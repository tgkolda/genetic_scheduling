#include "Genetic.hpp"
#include "Scheduler.hpp"
#include <iostream>
#include <QApplication>

int main(int argc, char* argv[]) {
  int ret_code;
  QApplication app(argc, argv);
  Kokkos::initialize(argc, argv);
  {
    const unsigned nslots = 12;

    // Read the rooms from yaml
    Rooms rooms("../../data/SIAM-CSE23/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/SIAM-CSE23/minisymposia.yaml", rooms.size(), nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    Genetic<Scheduler> g(s);
    g.run(10000, 2000, 0.01, 10000);
//    s.record("schedule.md");

    // Create a table to display the schedule
//    Schedule sched(s.get_best_schedule(), &rooms, &mini);

//    ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}