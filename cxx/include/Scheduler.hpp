#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include "Kokkos_Random.hpp"
#include <random>
#include <vector>

class Scheduler {
public:
  Scheduler(const Minisymposia& mini, const Rooms& rooms, unsigned ntimeslots);
  void run_genetic(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);
  void print_best_schedule() const;
  void initialize_schedules(unsigned nschedules);
  void rate_schedules(unsigned eliteSize);
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
  void sort_on_ratings();
  void record(const std::string& filename) const;

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

#endif /* SCHEDULER_H */