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
  Kokkos::resize(current_schedules_, nschedules, nrooms, ntimeslots_);
  Kokkos::resize(next_schedules_, nschedules, nrooms, ntimeslots_);
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
    for(unsigned r=0; r<nrooms; r++) {
      for(unsigned sl=0; sl<ntimeslots_; sl++) {
        current_schedules_(sc,r,sl) = numbers[ind];
        ind++;
      }
    }
  }
}

void Scheduler::rate_schedules(std::vector<unsigned>& best_indices, unsigned eliteSize) {
  unsigned nmini = mini_.size();
  unsigned max_penalty = mini_.get_max_penalty();
  for(unsigned sc=0; sc<current_schedules_.extent(0); sc++) {
    unsigned penalty = 0;
    ratings_[sc] = 0.0;
    // Compute the penalty related to multi-part minisymposia being out of order
    for(unsigned sl1=0; sl1<current_schedules_.extent(2); sl1++) {
      for(unsigned r1=0; r1<current_schedules_.extent(1); r1++) {
        if(current_schedules_(sc,r1,sl1) >= nmini) continue;
        for(unsigned sl2=sl1; sl2<current_schedules_.extent(2); sl2++) {
          for(unsigned r2=0; r2<current_schedules_.extent(1); r2++) {
            if(current_schedules_(sc,r2,sl2) >= nmini) continue;
            if(mini_.breaks_ordering(current_schedules_(sc,r1,sl1), current_schedules_(sc,r2,sl2))) {
              // If we can swap the events to avoid an issue, do that
              if(sl1 == sl2) {
                penalty++;
              }
              else {
                std::swap(current_schedules_(sc,r1,sl1), current_schedules_(sc,r2,sl2));
              }
            }
          }
        }
      }
    }
    // Compute the penalty related to oversubscribed participants
    for(unsigned sl=0; sl<current_schedules_.extent(2); sl++) {
      for(unsigned r1=0; r1<current_schedules_.extent(1); r1++) {
        if(current_schedules_(sc,r1,sl) >= nmini) continue;
        for(unsigned r2=r1+1; r2<current_schedules_.extent(1); r2++) {
          if(current_schedules_(sc,r2,sl) >= nmini) continue;
          if(mini_.overlaps_participants(current_schedules_(sc,r1,sl), current_schedules_(sc,r2,sl))) {
            penalty++;
          }
        }
      }
    }
    ratings_[sc] = 1 - (double)penalty / max_penalty;
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
  for(unsigned i=eliteSize; i<current_schedules_.extent(0); i++) {
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