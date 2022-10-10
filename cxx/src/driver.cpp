#include "Scheduler.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    const unsigned nslots = 13;

    // Read the rooms from yaml
    Rooms rooms("../../data/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/minisymposia_improved_themes.yaml", rooms.size(), nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    s.run_genetic(10000, 2000, 0.01, 10000);
    s.record("schedule.md");
  }
  Kokkos::finalize();
  return 0;
}