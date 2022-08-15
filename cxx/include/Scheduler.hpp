#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Room.hpp"
#include <random>
#include <vector>

class Scheduler {
public:
  Scheduler(const Minisymposia& mini, const std::vector<Room>& rooms, unsigned ntimeslots);
  void run_genetic(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);
private:
  void initialize_schedules(unsigned nschedules);
  void rate_schedules(std::vector<unsigned>& best_indices, unsigned eliteSize);

  std::vector<Room> rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
  Kokkos::View<unsigned***> schedules_;
  Kokkos::View<double*> ratings_;
  std::default_random_engine rng_;
};

#endif /* SCHEDULER_H */