#include "Scheduler.hpp"
#include "Utility.hpp"

Scheduler::Scheduler(const Minisymposia& mini, 
                     const std::vector<Room>& rooms, 
                     unsigned ntimeslots) :
  mini_(mini),
  rooms_(rooms),
  ntimeslots_(ntimeslots)
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
  ratings_.resize(nschedules);
  weights_.resize(nschedules);

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
  std::vector<unsigned> theme_penalties(nthemes);
  unsigned max_penalty = mini_.get_max_penalty();
  unsigned max_theme_penalty = mini_.get_max_theme_penalty();
  for(unsigned sc=0; sc<nschedules(); sc++) {
    double penalty = 0.0;
    ratings_[sc] = 0.0;
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
  }

  // Find the indices of the most promising schedules
  std::iota(best_indices.begin(), best_indices.end(), 0); // fill with 0,1,2,...
  std::nth_element(best_indices.begin(), best_indices.begin()+eliteSize, best_indices.end(),
                  [=](int i,int j) {return ratings_[i]>ratings_[j];});
}

void Scheduler::compute_weights() {
  // Find the worst schedule's score
  double worst_score = *std::min_element(ratings_.begin(), ratings_.end());
  std::transform(ratings_.begin(), ratings_.end(), weights_.begin(), [=] (double d) {
    return d - worst_score;
  });
}

void Scheduler::breed_population(std::vector<unsigned>& best_indices, unsigned eliteSize) {
  // Copy over the elite items to the new population
  for(unsigned i=0; i<eliteSize; i++) {
    unsigned index = best_indices[i];
    auto src = Kokkos::subview(current_schedules_, index, Kokkos::ALL(), Kokkos::ALL());
    auto dst = Kokkos::subview(next_schedules_, index, Kokkos::ALL(), Kokkos::ALL());
    Kokkos::deep_copy(dst, src);
  }

  // Breed to obtain the rest
  std::discrete_distribution<unsigned> distribution(weights_.begin(), weights_.end());
  for(unsigned i=eliteSize; i<nschedules(); i++) {
    // Get the parents
    unsigned pid1 = distribution(rng_);
    unsigned pid2 = pid1;
    while(pid2 == pid1) { // Make sure the parents are different
      pid2 = distribution(rng_);
    }
    breed(pid1, pid2, i);
  }
}

void Scheduler::breed(unsigned mom_index, unsigned dad_index, unsigned child_index) {
  using genetic::contains;

  auto mom = Kokkos::subview(current_schedules_, mom_index, Kokkos::ALL(), Kokkos::ALL());
  auto dad = Kokkos::subview(current_schedules_, dad_index, Kokkos::ALL(), Kokkos::ALL());
  auto child = Kokkos::subview(next_schedules_, child_index, Kokkos::ALL(), Kokkos::ALL());

  // Determine which timeslots are carried over from the mom
  unsigned nslots = current_schedules_.extent(2);
  std::uniform_int_distribution<unsigned> distribution(0,nslots);
  unsigned start_index = distribution(rng_);
  unsigned end_index = start_index;
  while(end_index == start_index ||
        std::abs((int)end_index - (int)start_index) == nslots) { // Make sure at least some are carried over, but not all
    end_index = distribution(rng_);
  }
  if(end_index < start_index) {
    std::swap(start_index, end_index);
  }

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
  std::uniform_real_distribution<double> mutate_dist(0.0,1.0);
  std::uniform_int_distribution<unsigned> room_dist(0,nrooms());
  std::uniform_int_distribution<unsigned> slot_dist(0,nslots());
  std::uniform_int_distribution<unsigned> coin_flip(0,2);

  for(unsigned sc=0; sc<nschedules(); sc++) {
    for(unsigned sl=0; sl<nslots(); sl++) {
      for(unsigned r=0; r<nrooms(); r++) {
        if(mutate_dist(rng_) < mutationRate) {
          // Swap the element with something else
          unsigned r2 = r, sl2 = sl;
          if(coin_flip(rng_) == 0) {
            while(sl2 == sl) {
              sl2 = slot_dist(rng_);
            }
          }
          else {
            while(r2 == r) {
              r2 = room_dist(rng_);
            }
          }
          std::swap(next_schedules_(sc,sl,r), next_schedules_(sc,sl2,r2));
        }
      }
    }
  }
}

unsigned Scheduler::nschedules() const {
  return current_schedules_.extent(0);
}

unsigned Scheduler::nslots() {
  return current_schedules_.extent(1);
}

unsigned Scheduler::nrooms() const {
  return current_schedules_.extent(2);
}
