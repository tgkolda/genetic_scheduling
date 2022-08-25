#include "Scheduler.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    // Read the minisymposia from yaml
    Minisymposia mini("../../data/minisymposia_predicted_theme.yaml");

    // Read the rooms from yaml
    Rooms rooms("../../data/rooms.yaml");
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, 13);
    s.run_genetic(10000, 2000, 0.0, 1000);
  }
  Kokkos::finalize();
  return 0;
}