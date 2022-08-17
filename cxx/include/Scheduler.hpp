#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Room.hpp"
#include "Kokkos_Random.hpp"
#include <random>
#include <vector>

class Scheduler {
public:
  Scheduler(const Minisymposia& mini, const std::vector<Room>& rooms, unsigned ntimeslots);
  void run_genetic(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);
private:
  void initialize_schedules(unsigned nschedules);
  void rate_schedules(std::vector<unsigned>& best_indices, unsigned eliteSize);
  void compute_weights();
  void breed_population(std::vector<unsigned>& best_indices, unsigned eliteSize);
  void breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const;
  void mutate_population(double mutationRate);
  unsigned nschedules() const;
  unsigned nslots() const;
  unsigned nrooms() const;
  unsigned get_parent() const;

  std::vector<Room> rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
  Kokkos::View<unsigned***> current_schedules_;
  Kokkos::View<unsigned***> next_schedules_;
  Kokkos::View<double*> ratings_;
  Kokkos::View<double*> weights_;
  std::default_random_engine rng_;
  Kokkos::Random_XorShift64_Pool<> pool_;
};

#endif /* SCHEDULER_H */