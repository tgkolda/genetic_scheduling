#include "Scheduler.hpp"

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
  initialize_schedules(popSize);

  for(unsigned g=0; g<generations; g++) {
    rate_schedules();
    for(int i=0; i<ratings_.extent(0); i++) {
      std::cout << i << " " << ratings_(i) << "\n";
    }
    break;
  }
}

void Scheduler::initialize_schedules(unsigned nschedules) {
  unsigned nrooms = rooms_.size();
  // Resize the array of schedules
  Kokkos::resize(schedules_, nschedules, nrooms, ntimeslots_);
  Kokkos::resize(ratings_, nschedules);

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
        schedules_(sc,r,sl) = numbers[ind];
        ind++;
      }
    }
  }
}

void Scheduler::rate_schedules() {
  unsigned nmini = mini_.size();
  unsigned max_penalty = mini_.get_max_penalty();
  for(unsigned sc=0; sc<schedules_.extent(0); sc++) {
    unsigned penalty = 0;
    ratings_(sc) = 0.0;
    // Compute the penalty related to multi-part minisymposia being out of order
    for(unsigned sl1=0; sl1<schedules_.extent(2); sl1++) {
      for(unsigned r1=0; r1<schedules_.extent(1); r1++) {
        if(schedules_(sc,r1,sl1) >= nmini) continue;
        for(unsigned sl2=sl1; sl2<schedules_.extent(2); sl2++) {
          for(unsigned r2=0; r2<schedules_.extent(1); r2++) {
            if(schedules_(sc,r2,sl2) >= nmini) continue;
            if(mini_.breaks_ordering(schedules_(sc,r1,sl1), schedules_(sc,r2,sl2))) {
              // If we can swap the events to avoid an issue, do that
              if(sl1 == sl2) {
                penalty++;
              }
              else {
                std::swap(schedules_(sc,r1,sl1), schedules_(sc,r2,sl2));
              }
            }
          }
        }
      }
    }
    // Compute the penalty related to oversubscribed participants
    for(unsigned sl=0; sl<schedules_.extent(2); sl++) {
      for(unsigned r1=0; r1<schedules_.extent(1); r1++) {
        if(schedules_(sc,r1,sl) >= nmini) continue;
        for(unsigned r2=r1+1; r2<schedules_.extent(1); r2++) {
          if(schedules_(sc,r2,sl) >= nmini) continue;
          if(mini_.overlaps_participants(schedules_(sc,r1,sl), schedules_(sc,r2,sl))) {
            penalty++;
          }
        }
      }
    }
    ratings_(sc) = 1 - (double)penalty / max_penalty;
  }
}