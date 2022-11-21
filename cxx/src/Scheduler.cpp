#include "Scheduler.hpp"
#include "Utility.hpp"
#include "Kokkos_StdAlgorithms.hpp"
#include <fstream>

Scheduler::Scheduler(const Minisymposia& mini, 
                     const Rooms& rooms, 
                     unsigned ntimeslots) :
  mini_(mini),
  rooms_(rooms),
  ntimeslots_(ntimeslots)
{
  
}

Scheduler::ViewType Scheduler::make_initial_population(unsigned nschedules) const {
  return ViewType("schedules", nschedules, ntimeslots_, rooms_.size());
}

bool Scheduler::out_of_bounds(unsigned i) const {
  return i >= mini_.size();
}

void Scheduler::fix_schedules() {
/*
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
*/
}

KOKKOS_FUNCTION
unsigned Scheduler::nslots() const {
  return ntimeslots_;
}

KOKKOS_FUNCTION
unsigned Scheduler::nrooms() const {
  return rooms_.size();
}
