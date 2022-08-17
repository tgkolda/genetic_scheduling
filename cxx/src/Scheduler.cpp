#include "Scheduler.hpp"
#include "Utility.hpp"
#include "Kokkos_StdAlgorithms.hpp"

Scheduler::Scheduler(const Minisymposia& mini, 
                     const std::vector<Room>& rooms, 
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
  std::vector<unsigned> best_indices(popSize);
  initialize_schedules(popSize);

  for(unsigned g=0; g<generations; g++) {
    rate_schedules(best_indices, eliteSize);
    compute_weights();
    breed_population(best_indices, eliteSize);
    mutate_population(mutationRate);
    std::swap(current_schedules_, next_schedules_);

    std::cout << "generation " << g << ":\n";
    for(int i=0; i<eliteSize; i++) {
      std::cout << i << " " << best_indices[i] << " " << ratings_[best_indices[i]] << "\n";
    }
  }
}

void Scheduler::initialize_schedules(unsigned nschedules) {
  unsigned nrooms = rooms_.size();
  // Resize the array of schedules
  Kokkos::resize(current_schedules_, nschedules, ntimeslots_, nrooms);
  Kokkos::resize(next_schedules_, nschedules, ntimeslots_, nrooms);
  Kokkos::resize(ratings_, nschedules);
  Kokkos::resize(weights_, nschedules);

  std::vector<unsigned> numbers(nrooms*ntimeslots_);
  for(size_t i=0; i<numbers.size(); i++) {
    numbers[i] = i;
  }

  for(size_t sc=0; sc<nschedules; sc++) {
    // Initialize the schedule with a randomly permuted set of numbers
    std::shuffle(numbers.begin(), numbers.end(), rng_);

    size_t ind=0;
    for(unsigned sl=0; sl<ntimeslots_; sl++) {
      for(unsigned r=0; r<nrooms; r++) {
        current_schedules_(sc, sl, r) = numbers[ind];
        ind++;
      }
    }
  }
}

void Scheduler::rate_schedules(std::vector<unsigned>& best_indices, unsigned eliteSize) {
  unsigned nmini = mini_.size();
  unsigned nthemes = mini_.themes().size();
  unsigned max_penalty = mini_.get_max_penalty();
  unsigned max_theme_penalty = mini_.get_max_theme_penalty();
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
              // If we can swap the events to avoid an issue, do that
              if(sl1 == sl2) {
                penalty++;
              }
              else {
                std::swap(current_schedules_(sc,sl1,r1), current_schedules_(sc,sl2,r2));
              }
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
    std::vector<unsigned> theme_penalties(nthemes);
    for(unsigned sl=0; sl<nslots(); sl++) {
      theme_penalties.assign(nthemes, 0);
      for(unsigned r=0; r<nrooms(); r++) {
        unsigned mini_index = current_schedules_(sc,sl,r);
        if(mini_index >= nmini) continue;
        unsigned tid = mini_[mini_index].tid();
        theme_penalties[tid]++;
      }
      for(auto p : theme_penalties) {
        if(p > 1) {
          theme_penalty += pow(p-1, 2);
        }
      }
    }
    penalty += (double)theme_penalty / max_theme_penalty;
    ratings_[sc] = 1 - penalty / max_penalty;
  });

  // Block until all ratings are computed since the next step uses them
  Kokkos::fence();

  // Find the indices of the most promising schedules
  std::iota(best_indices.begin(), best_indices.end(), 0); // fill with 0,1,2,...
  std::nth_element(best_indices.begin(), best_indices.begin()+eliteSize, best_indices.end(),
                  [=](int i,int j) {return ratings_[i]>ratings_[j];});
}

void Scheduler::compute_weights() {
  // Find the worst schedule's score
  double worst_score = *Kokkos::Experimental::min_element(Kokkos::DefaultExecutionSpace(), ratings_);
  Kokkos::parallel_for("compute weights", nschedules(), KOKKOS_CLASS_LAMBDA(int i) {
    weights_[i] = ratings_[i] - worst_score;
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

void Scheduler::breed_population(std::vector<unsigned>& best_indices, unsigned eliteSize) {
  // Copy over the elite items to the new population
  Kokkos::parallel_for("Copy elites", eliteSize, KOKKOS_CLASS_LAMBDA (unsigned i) {
    unsigned index = best_indices[i];
    auto src = Kokkos::subview(current_schedules_, index, Kokkos::ALL(), Kokkos::ALL());
    auto dst = Kokkos::subview(next_schedules_, index, Kokkos::ALL(), Kokkos::ALL());
    Kokkos::deep_copy(dst, src);
  });

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
    std::swap(start_index, end_index);
  }
  pool_.free_state(gen);

  // Copy those timeslots to the child
  std::pair<size_t, size_t> mom_indices(start_index, end_index);
  std::pair<size_t, size_t> kid_indices(0, end_index-start_index);
  auto mom_genes = Kokkos::subview(mom, Kokkos::ALL(), mom_indices);
  auto child_genes = Kokkos::subview(child, Kokkos::ALL(), kid_indices);
  Kokkos::deep_copy(child_genes, mom_genes);

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
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned r=0; r<nrooms(); r++) {
        auto gen = pool_.get_state();
        if(gen.drand() < mutationRate) {
          // Swap the element with something else
          unsigned r2 = r, sl2 = sl;
          if(gen.rand(2) == 0) { // Flip a coin to see if we swap rooms or slots
            while(sl2 == sl) {
              sl2 = gen.rand(nslots());
            }
          }
          else {
            while(r2 == r) {
              r2 = gen.rand(nrooms());
            }
          }
          pool_.free_state(gen);
          std::swap(next_schedules_(sc,sl,r), next_schedules_(sc,sl2,r2));
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

unsigned Scheduler::nschedules() const {
  return current_schedules_.extent(0);
}

unsigned Scheduler::nslots() const {
  return current_schedules_.extent(1);
}

unsigned Scheduler::nrooms() const {
  return current_schedules_.extent(2);
}

unsigned Scheduler::get_parent() const {
  // Get a random number between 0 and 1
  auto gen = pool_.get_state();
  double r = gen.drand();
  double sum = 0.0;
  unsigned parent = -1;
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