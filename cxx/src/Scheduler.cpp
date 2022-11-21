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

KOKKOS_FUNCTION
unsigned Scheduler::nslots() const {
  return ntimeslots_;
}

KOKKOS_FUNCTION
unsigned Scheduler::nrooms() const {
  return rooms_.size();
}
