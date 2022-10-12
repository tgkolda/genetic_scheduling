#include "Scheduler.hpp"
#include "Utility.hpp"
#include "Kokkos_StdAlgorithms.hpp"
#include <fstream>

Scheduler::Scheduler(const Minisymposia& mini, 
                     const Rooms& rooms, 
                     unsigned ntimeslots) :
  mini_(mini),
  rooms_(rooms),
  ntimeslots_(ntimeslots), 
  pool_(5374857)
{
  
}

void Scheduler::run_genetic(unsigned popSize, 
                            unsigned eliteSize, 
                            double mutationRate, 
                            unsigned generations)
{
  initialize_schedules(popSize);
  fix_schedules();
  for(unsigned g=0; g<generations; g++) {
    std::cout << "generation " << g << ":\n";

    double best_rating = rate_schedules(eliteSize);
    if(best_rating == 1.0) break;
    print_best_schedule();
    compute_weights();
    breed_population(eliteSize);
    mutate_population(mutationRate);
    //validate_schedules(next_schedules_); // This is just for debugging
    std::swap(current_schedules_, next_schedules_);
    fix_schedules();
  }
}

void Scheduler::print_best_schedule() const {
  // Find the best schedule
  typedef Kokkos::MaxLoc<double,unsigned>::value_type maxloc_type;
  maxloc_type maxloc;
  Kokkos::parallel_reduce( "Finding best schedule", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned i, maxloc_type& lmaxloc) {
    if(ratings_[i] > lmaxloc.val) { 
      lmaxloc.val = ratings_[i]; 
      lmaxloc.loc = i; 
    }
  }, Kokkos::MaxLoc<double,unsigned>(maxloc));

  std::cout << "The best schedule has score " << maxloc.val << "\n";
  //print_schedule(maxloc.loc);
}

void Scheduler::initialize_schedules(unsigned nschedules) {
  unsigned nrooms = rooms_.size();
  unsigned nthemes = mini_.themes().size();
  // Resize the array of schedules
  current_schedules_ = Kokkos::View<unsigned***>("current schedules", nschedules, ntimeslots_, nrooms);
  next_schedules_ = Kokkos::View<unsigned***>("next schedules", nschedules, ntimeslots_, nrooms);
  ratings_ = Kokkos::View<double*>("ratings", nschedules);
  weights_ = Kokkos::View<double*>("weights", nschedules);
  best_indices_ = Kokkos::View<unsigned*, Kokkos::HostSpace>("best indices", nschedules);
  theme_penalties_ = Kokkos::View<unsigned**>("theme penalties", nschedules, nthemes);

  std::vector<unsigned> numbers(nrooms*ntimeslots_);
  for(size_t i=0; i<numbers.size(); i++) {
    numbers[i] = i;
  }

  auto h_schedules = Kokkos::create_mirror_view(current_schedules_);
  for(size_t sc=0; sc<nschedules; sc++) {
    // Initialize the schedule with a randomly permuted set of numbers
    std::shuffle(numbers.begin(), numbers.end(), rng_);

    size_t ind=0;
    for(unsigned sl=0; sl<ntimeslots_; sl++) {
      for(unsigned r=0; r<nrooms; r++) {
        h_schedules(sc, sl, r) = numbers[ind];
        ind++;
      }
    }
  }
  Kokkos::deep_copy(current_schedules_, h_schedules);
}

double Scheduler::rate_schedules(unsigned eliteSize) {
  unsigned nmini = mini_.size();
  unsigned nthemes = mini_.themes().size();
  unsigned max_penalty = mini_.get_max_penalty();
  Kokkos::parallel_for("rate schedules", nschedules(), KOKKOS_CLASS_LAMBDA(unsigned sc) {
    double penalty = 0.0;
    // Compute the penalty related to multi-part minisymposia being out of order
    for(unsigned sl1=0; sl1<nslots(); sl1++) {
      for(unsigned r1=0; r1<nrooms(); r1++) {
        if(current_schedules_(sc,sl1,r1) >= nmini) continue;
        for(unsigned sl2=sl1; sl2<nslots(); sl2++) {
          for(unsigned r2=0; r2<nrooms(); r2++) {
            if(current_schedules_(sc,sl2,r2) >= nmini) continue;
            if(mini_.breaks_ordering(current_schedules_(sc,sl1,r1), current_schedules_(sc,sl2,r2))) {
              penalty++;
            }
          }
        }
      }
    }
    // Compute the penalty related to oversubscribed participants
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned r1=0; r1<nrooms(); r1++) {
        if(current_schedules_(sc,sl,r1) >= nmini) continue;
        for(unsigned r2=r1+1; r2<nrooms(); r2++) {
          if(current_schedules_(sc,sl,r2) >= nmini) continue;
          if(mini_.overlaps_participants(current_schedules_(sc,sl,r1), current_schedules_(sc,sl,r2))) {
            penalty++;
          }
        }
      }
    }
    // Compute the penalty related to theme overlap
    unsigned theme_penalty = 0;
    auto my_theme_penalties = Kokkos::subview(theme_penalties_, sc, Kokkos::ALL());
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned tid=0; tid<nthemes; tid++) {
        my_theme_penalties[tid] = 0;
      }
      for(unsigned r=0; r<nrooms(); r++) {
        unsigned mini_index = current_schedules_(sc,sl,r);
        if(mini_index >= nmini) continue;
        unsigned tid = mini_[mini_index].tid();
        my_theme_penalties[tid]++;
      }
      for(unsigned tid=0; tid<nthemes; tid++) {
        auto p = my_theme_penalties[tid];
        if(p > 1) {
          theme_penalty += pow(p-1, 2);
        }
      }
    }
    // Compute the penalty related to priority
    unsigned priority_penalty = 0;
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned r=0; r<nrooms(); r++) {
        unsigned mini_index = current_schedules_(sc,sl,r);
        if(mini_index >= nmini) continue;
        unsigned priority = mini_[mini_index].priority();
        if(priority < r) {
          priority_penalty += pow(r-priority, 2);
        }
      }
    }
    if(sc == 0) printf("Theme penalty: %i\nPriority penalty: %i\n", theme_penalty, priority_penalty);
    penalty += mini_.map_theme_penalty(theme_penalty) / 2.0;
    penalty += mini_.map_priority_penalty(priority_penalty) / 2.0;
    ratings_[sc] = 1 - penalty / max_penalty;
  });

  // Block until all ratings are computed since the next step uses them
  Kokkos::fence();

  // Sort the indices based on the ratings
  return sort_on_ratings();
}

void Scheduler::fix_schedules() {
  unsigned nmini = mini_.size();
  Kokkos::parallel_for("fix schedules", nschedules(), KOKKOS_CLASS_LAMBDA(unsigned sc) {
    // If we can put multi-part minisymposia in order, do that
    for(unsigned sl1=0; sl1<nslots(); sl1++) {
      for(unsigned r1=0; r1<nrooms(); r1++) {
        if(current_schedules_(sc,sl1,r1) >= nmini) continue;
        for(unsigned sl2=sl1+1; sl2<nslots(); sl2++) {
          for(unsigned r2=0; r2<nrooms(); r2++) {
            if(current_schedules_(sc,sl2,r2) >= nmini) continue;
            if(mini_.breaks_ordering(current_schedules_(sc,sl1,r1), current_schedules_(sc,sl2,r2))) {
              // swap the values
              auto temp = current_schedules_(sc,sl1,r1);
              current_schedules_(sc,sl1,r1) = current_schedules_(sc,sl2,r2);
              current_schedules_(sc,sl2,r2) = temp;
            }
          }
        }
      }
    }

    // Sort the minisymposia in each slot based on the room priority
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned i=1; i<nrooms(); i++) {
        for(unsigned j=0; j<nrooms()-i; j++) {
          auto m1 = current_schedules_(sc,sl,j);
          auto m2 = current_schedules_(sc,sl,j+1);
          if(m2 >= nmini) continue;
          if(m1 >= nmini || mini_[m2].higher_priority(mini_[m1])) {
              // swap the values
              auto temp = current_schedules_(sc,sl,j);
              current_schedules_(sc,sl,j) = current_schedules_(sc,sl,j+1);
              current_schedules_(sc,sl,j+1) = temp;
          }
        }
      }
    }
  });
}

void Scheduler::compute_weights() {
  // Find the worst schedule's score
  auto worst_score = Kokkos::Experimental::min_element(Kokkos::DefaultExecutionSpace(), ratings_);
  Kokkos::parallel_for("compute weights", nschedules(), KOKKOS_CLASS_LAMBDA(int i) {
    weights_[i] = ratings_[i] - *worst_score;
  });

  // Block until all weights are computed since the next step uses them
  Kokkos::fence();

  // Compute the sum of all weights
  double weight_sum;
  Kokkos::parallel_reduce("Weight sum", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned i, double& lsum ) {
    lsum += weights_[i];
  },weight_sum);

  // Block until the sum is computed since the next step uses it
  Kokkos::fence();

  // Normalize the weights
  Kokkos::parallel_for("Normalize weights", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned i) {
    weights_[i] / weight_sum;
  });

  // Block until the weights are normalized since the next step uses them
  Kokkos::fence();
}

void Scheduler::breed_population(unsigned eliteSize) {
  // Copy over the elite items to the new population
  for(unsigned i=0; i<eliteSize; i++) {
    unsigned index = best_indices_[i];
    auto src = Kokkos::subview(current_schedules_, index, Kokkos::ALL(), Kokkos::ALL());
    auto dst = Kokkos::subview(next_schedules_, i, Kokkos::ALL(), Kokkos::ALL());
    Kokkos::deep_copy(dst, src);
  }

  // Breed to obtain the rest
  Kokkos::parallel_for("Breeding", Kokkos::RangePolicy<>(eliteSize, nschedules()), KOKKOS_CLASS_LAMBDA (unsigned i) {
    // Get the parents
    unsigned pid1 = get_parent();
    unsigned pid2 = pid1;
    while(pid2 == pid1) { // Make sure the parents are different
      pid2 = get_parent();
    }
    breed(pid1, pid2, i);
  });

  // Block until the breeding is complete since the next step uses the results
  Kokkos::fence();
}

KOKKOS_FUNCTION
void Scheduler::breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const {
  using genetic::contains;

  auto mom = Kokkos::subview(current_schedules_, mom_index, Kokkos::ALL(), Kokkos::ALL());
  auto dad = Kokkos::subview(current_schedules_, dad_index, Kokkos::ALL(), Kokkos::ALL());
  auto child = Kokkos::subview(next_schedules_, child_index, Kokkos::ALL(), Kokkos::ALL());

  // Determine which timeslots are carried over from the mom
  unsigned nslots = current_schedules_.extent(2);
  auto gen = pool_.get_state();
  unsigned start_index = gen.rand(nslots);
  unsigned end_index = start_index;
  while(end_index == start_index ||
        std::abs((int)end_index - (int)start_index) == nslots) { // Make sure at least some are carried over, but not all
    end_index = gen.rand(nslots);
  }
  if(end_index < start_index) {
    auto temp = start_index;
    start_index = end_index;
    end_index = temp;
  }
  pool_.free_state(gen);

  // Copy those timeslots to the child
  Kokkos::pair<size_t, size_t> mom_indices(start_index, end_index);
  Kokkos::pair<size_t, size_t> kid_indices(0, end_index-start_index);
  auto mom_genes = Kokkos::subview(mom, Kokkos::ALL(), mom_indices);
  auto child_genes = Kokkos::subview(child, Kokkos::ALL(), kid_indices);
  for(unsigned i=0; i<mom_genes.extent(0); i++) {
    for(unsigned j=0; j<mom_genes.extent(1); j++) {
      child_genes(i, j) = mom_genes(i, j);
    }
  }

  // Fill in the gaps with dad's info
  kid_indices.first = end_index - start_index;
  kid_indices.second = nslots;
  child_genes = Kokkos::subview(child, Kokkos::ALL(), kid_indices);
  unsigned dr=0, ds=0;
  for(unsigned cr=0; cr<child_genes.extent(0); cr++) {
    for(unsigned cs=0; cs<child_genes.extent(1); cs++) {
      while(contains(mom_genes, dad(dr, ds))) {
        ds++;
        if(ds >= dad.extent(1)) {
          ds = 0;
          dr++;
        }
      }
      child_genes(cr, cs) = dad(dr, ds);
      ds++;
      if(ds >= dad.extent(1)) {
        ds = 0;
        dr++;
      }
    }
  }
}

void Scheduler::mutate_population(double mutationRate) {
  Kokkos::parallel_for("Mutations", nschedules(), KOKKOS_CLASS_LAMBDA(unsigned sc) {
    // Don't mutate the best schedule
    if (sc == 0) return;
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned r=0; r<nrooms(); r++) {
        auto gen = pool_.get_state();
        if(gen.drand() < mutationRate) {
          // Swap the element with another slot
          unsigned sl2 = sl;
          while(sl2 == sl) {
            sl2 = gen.rand(nslots());
          }
          pool_.free_state(gen);
          auto temp = next_schedules_(sc,sl,r);
          next_schedules_(sc,sl,r) = next_schedules_(sc,sl2,r);
          next_schedules_(sc,sl2,r) = temp;
        }
        else {
          pool_.free_state(gen);
        }
      }
    }
  });

  // Block until the mutations are complete since the next step uses the results
  Kokkos::fence();
}

KOKKOS_FUNCTION
unsigned Scheduler::nschedules() const {
  return current_schedules_.extent(0);
}

KOKKOS_FUNCTION
unsigned Scheduler::nslots() const {
  return current_schedules_.extent(1);
}

KOKKOS_FUNCTION
unsigned Scheduler::nrooms() const {
  return current_schedules_.extent(2);
}

KOKKOS_FUNCTION
unsigned Scheduler::get_parent() const {
  // Get a random number between 0 and 1
  auto gen = pool_.get_state();
  double r = gen.drand();
  double sum = 0.0;
  unsigned parent = unsigned(-1);
  for(unsigned sc=0; sc<nschedules(); sc++) {
    sum += weights_[sc];
    if(r < sum) {
      parent = sc;
      break;
    }
  }
  pool_.free_state(gen);
  return parent;
}

void Scheduler::print_schedule(unsigned sc) const {
  unsigned nmini = mini_.size();
  auto h_current_schedules = Kokkos::create_mirror_view(current_schedules_);
  Kokkos::deep_copy(h_current_schedules, current_schedules_);
  for(unsigned slot=0; slot<nslots(); slot++) {
    std::cout << "Slot " << slot << ":\n";
    for(unsigned room=0; room<nrooms(); room++) {
      unsigned mid = h_current_schedules(sc, slot, room);
      if(mid < nmini) {
        std::cout << mini_.get_title(mid) << " (" << mini_.get_theme(mid) << ")\n";
      }
    }
  }
}

void Scheduler::validate_schedules(Kokkos::View<unsigned***> schedules) const {
  Kokkos::parallel_for("validating", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned sc) {
    auto sched = Kokkos::subview(schedules, sc, Kokkos::ALL(), Kokkos::ALL());
    for(unsigned m=0; m < mini_.size(); m++) {
      if(!genetic::contains(sched, m)) {
        printf("ERROR! Schedule %i does not contain minisymposium %i\n", sc, m);
        break;
      }
    }
  });
  Kokkos::fence();
}

double Scheduler::sort_on_ratings() {
  size_t n = best_indices_.extent(0);
  auto h_ratings = Kokkos::create_mirror_view(ratings_);
  Kokkos::deep_copy(h_ratings, ratings_);

  // Find the indices of the most promising schedules
  for(unsigned i=0; i<n; i++) {
    best_indices_[i] = i;
  }

  for(unsigned i=0; i<n; i++) {
    for(unsigned j=0; j < n-i-1; j++) {
      if(h_ratings[j] < h_ratings[j+1]) {
        std::swap(best_indices_[j], best_indices_[j+1]);
        std::swap(h_ratings[j], h_ratings[j+1]);
      }
    }
  }

  return h_ratings[n-1];
}

void Scheduler::record(const std::string& filename) const {
  // Find the best schedule
  typedef Kokkos::MaxLoc<double,unsigned>::value_type maxloc_type;
  maxloc_type maxloc;
  Kokkos::parallel_reduce( "Finding best schedule", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned i, maxloc_type& lmaxloc) {
    if(ratings_[i] > lmaxloc.val) { 
      lmaxloc.val = ratings_[i]; 
      lmaxloc.loc = i; 
    }
  }, Kokkos::MaxLoc<double,unsigned>(maxloc));

  unsigned nmini = mini_.size();
  unsigned sc = maxloc.loc;
  auto h_current_schedules = Kokkos::create_mirror_view(current_schedules_);
  Kokkos::deep_copy(h_current_schedules, current_schedules_);

  std::ofstream fout(filename);
  fout << "# Conference schedule with score " << maxloc.val << "\n\n";

  for(unsigned slot=0; slot<nslots(); slot++) {
    fout << "|Slot " << slot << "|   |   |   |\n|---|---|---|---|\n";
    for(unsigned room=0; room<nrooms(); room++) {
      unsigned mid = h_current_schedules(sc, slot, room);
      if(mid < nmini) {
        fout << "|" << mini_.get_title(mid) << "|" << mini_.get_theme(mid) << "|" << mini_.get_priority(mid) << "|" << rooms_.name(room) << "|\n";
      }
    }
    fout << "\n";
  }
}

void Scheduler::populate(Schedule& table) const {
  // Find the best schedule
  typedef Kokkos::MaxLoc<double,unsigned>::value_type maxloc_type;
  maxloc_type maxloc;
  Kokkos::parallel_reduce( "Finding best schedule", nschedules(), KOKKOS_CLASS_LAMBDA (unsigned i, maxloc_type& lmaxloc) {
    if(ratings_[i] > lmaxloc.val) { 
      lmaxloc.val = ratings_[i]; 
      lmaxloc.loc = i; 
    }
  }, Kokkos::MaxLoc<double,unsigned>(maxloc));

  // Copy the schedule to host
  unsigned nmini = mini_.size();
  unsigned sc = maxloc.loc;
  auto h_current_schedules = Kokkos::create_mirror_view(current_schedules_);
  Kokkos::deep_copy(h_current_schedules, current_schedules_);

  // Populate schedule
  for(unsigned room=0; room<nrooms(); room++) {
    for(unsigned slot=0; slot<nslots(); slot++) {
      unsigned mid = h_current_schedules(sc, slot, room);
      table.setData(table.index(room, slot), QVariant(mid));
    }
  }
}