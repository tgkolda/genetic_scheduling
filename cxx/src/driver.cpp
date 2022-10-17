#include "Schedule.hpp"
#include "Scheduler.hpp"
#include <iostream>
#include <QApplication>

int main(int argc, char* argv[]) {
  int ret_code;
  QApplication app(argc, argv);
  Kokkos::initialize(argc, argv);
  {
    const unsigned nslots = 13;

    // Read the rooms from yaml
    Rooms rooms("../../data/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/minisymposia_improved_themes.yaml", rooms.size(), nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    s.run_genetic(1000, 200, 0.01, 10);
    s.record("schedule.md");

    // Create a table to display the schedule
    Schedule sched(rooms.size(), nslots, &rooms, &mini);

    // Populate the schedule
    s.populate(sched);

    ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}