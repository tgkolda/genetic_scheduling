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

  void run_genetic(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);
  void print_best_schedule() const;
  void initialize_schedules(unsigned nschedules);
  double rate_schedules();
  void fix_schedules();
  void compute_weights();
  void breed_population(unsigned eliteSize);
  KOKKOS_FUNCTION void breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const;
  void mutate_population(double mutationRate);
  KOKKOS_FUNCTION unsigned nschedules() const;
  KOKKOS_FUNCTION unsigned nslots() const;
  KOKKOS_FUNCTION unsigned nrooms() const;
  KOKKOS_FUNCTION unsigned get_parent() const;
  void print_schedule(unsigned sc) const;
  void validate_schedules(Kokkos::View<unsigned***> schedules) const;
  double sort_on_ratings();
  void record(const std::string& filename) const;
  Kokkos::View<unsigned**,Kokkos::LayoutStride> get_best_schedule() const;

private:
  Rooms rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
  Kokkos::View<unsigned***> current_schedules_;
  Kokkos::View<unsigned***> next_schedules_;
  Kokkos::View<double*> ratings_;
  Kokkos::View<double*> weights_;
  Kokkos::View<unsigned*, Kokkos::HostSpace> best_indices_;
  Kokkos::View<unsigned**> theme_penalties_;
  std::default_random_engine rng_;
  Kokkos::Random_XorShift64_Pool<> pool_;
};

template<class View2D>
double Scheduler::rate(View2D schedule) const {
  unsigned order_penalty, oversubscribed_penalty, theme_penalty, priority_penalty;
  return mini_.rate_schedule(schedule, order_penalty, 
    oversubscribed_penalty, theme_penalty, priority_penalty);
}

#endif /* SCHEDULER_H */