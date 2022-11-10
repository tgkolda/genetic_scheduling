#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include "Schedule.hpp"
#include "Kokkos_Random.hpp"
#include <random>
#include <vector>
#include <QTableWidget>

class Scheduler {
public:
  typedef Kokkos::View<unsigned***> ViewType;

  Scheduler(const Minisymposia& mini, const Rooms& rooms, unsigned ntimeslots);
  ViewType make_initial_population(unsigned nschedules) const;

  template<class View2D>
  KOKKOS_INLINE_FUNCTION double rate(View2D schedule) const;

  KOKKOS_FUNCTION bool out_of_bounds(unsigned i) const;
  void fix_schedules();
  KOKKOS_FUNCTION unsigned nslots() const;
  KOKKOS_FUNCTION unsigned nrooms() const;
  void record(const std::string& filename) const;

private:
  Rooms rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
};

template<class View2D>
double Scheduler::rate(View2D schedule) const {
  unsigned order_penalty, oversubscribed_penalty, theme_penalty, priority_penalty;
  return mini_.rate_schedule(schedule, order_penalty, 
    oversubscribed_penalty, theme_penalty, priority_penalty);
}

#endif /* SCHEDULER_H */