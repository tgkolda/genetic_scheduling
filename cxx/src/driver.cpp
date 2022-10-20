#include "Schedule.hpp"
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
    Rooms rooms("../../data/rooms.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/minisymposia_improved_themes.yaml", rooms.size(), nslots);
 
    // Run the genetic algorithm
    Scheduler s(mini, rooms, nslots);
    s.run_genetic(10000, 2000, 0.01, 10000);
    s.record("schedule.md");

    // Create a table to display the schedule
    Schedule sched(s.get_best_schedule(), &rooms, &mini);

    ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}