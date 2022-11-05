#include "Genetic.hpp"
#include "Mapper.hpp"
#include <QApplication>

int main(int argc, char* argv[]) {
  int ret_code;
  //QApplication app(argc, argv);
  Kokkos::initialize(argc, argv);
  {
    // Read the lectures from yaml
    Lectures lectures("../../data/SIAM-CSE23/lectures.yaml");

    // Read the minisymposia from yaml
    Minisymposia mini("../../data/SIAM-CSE23/minisymposia.yaml");
 
    // Run the genetic algorithm
    Mapper m(lectures, mini);
    Genetic<Mapper> g(m);
    auto best_schedule = g.run(10000, 2000, 0.01, 10000);
    m.record("lecture-assignments.md", best_schedule);

    // Create a table to display the schedule
  //  Schedule sched(s.get_best_schedule(), &rooms, &mini);

    //ret_code = app.exec();
  }
  Kokkos::finalize();
  return ret_code;
}