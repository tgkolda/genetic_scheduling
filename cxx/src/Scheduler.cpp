#include "Scheduler.hpp"
#include "Utility.hpp"
#include "Kokkos_StdAlgorithms.hpp"
#include <fstream>

Scheduler::Scheduler(const Minisymposia& mini) :
  mini_(mini)
{
  
}

Scheduler::ViewType Scheduler::make_initial_population(unsigned nschedules) const {
  return ViewType("schedules", nschedules, nslots(), nrooms());
}

bool Scheduler::out_of_bounds(unsigned i) const {
  return i >= mini_.size();
}

KOKKOS_FUNCTION
unsigned Scheduler::nslots() const {
  return mini_.timeslots().size();
}

KOKKOS_FUNCTION
unsigned Scheduler::nrooms() const {
  return mini_.rooms().size();
}
