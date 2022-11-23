#include "Genetic.hpp"
#include "Scheduler.hpp"
#include <iostream>
#include <QApplication>

int main(int argc, char* argv[]) {
  int ret_code;
//  QApplication app(argc, argv);
  Kokkos::initialize(argc, argv);
  {
    const unsigned nslots = 12;

    // Read the themes from yaml
    Theme::read("../../data/SIAM-CSE23/codes.yaml");

    // Read the citations from yaml
    Speaker::read("../../data/SIAM-CSE23/citations.yaml");

    // Read the rooms from yaml
    Rooms rooms("../../data/SIAM-CSE23/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/SIAM-CSE23/minisymposia.yaml", rooms, nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    Genetic<Scheduler> g(s);
    auto best_schedule = g.run(1000, 200, 0.01, 1000);
    s.record("schedule.md", best_schedule);

    // Create a table to display the schedule
//    Schedule sched(s.get_best_schedule(), &rooms, &mini);

//    ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}