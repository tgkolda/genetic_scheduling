#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include "Schedule.hpp"
#include "Kokkos_Random.hpp"
#include <fstream>
#include <random>
#include <vector>
#include <QTableWidget>

class Scheduler {
public:
  typedef Kokkos::View<unsigned***> ViewType;

  Scheduler(const Minisymposia& mini, const Rooms& rooms, unsigned ntimeslots);
  ViewType make_initial_population(unsigned nschedules) const;

  template<class View2D>
  KOKKOS_INLINE_FUNCTION double rate(View2D schedule) const;

  template<class View2D>
  inline void record(const std::string& filename, View2D schedule) const;

  KOKKOS_FUNCTION bool out_of_bounds(unsigned i) const;
  void fix_schedules();
  KOKKOS_FUNCTION unsigned nslots() const;
  KOKKOS_FUNCTION unsigned nrooms() const;
  void record(const std::string& filename) const;

private:
  Rooms rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
};

template<class View2D>
double Scheduler::rate(View2D schedule) const {
  unsigned order_penalty, oversubscribed_penalty, priority_penalty;
  double theme_penalty;
  double result = mini_.rate_schedule(schedule, order_penalty, oversubscribed_penalty, 
                                      theme_penalty, priority_penalty);
  return result;
}

template<class View2D>
void Scheduler::record(const std::string& filename, View2D schedule) const {
  unsigned nmini = mini_.size();
  auto class_codes = mini_.class_codes();

  std::ofstream fout(filename);
  fout << "# Conference schedule\n\n";

  for(unsigned slot=0; slot<nslots(); slot++) {
    fout << "|Slot " << slot << "|   |   |   |\n|---|---|---|---|\n";
    for(unsigned room=0; room<nrooms(); room++) {
      unsigned mid = schedule(slot, room);
      if(mid < nmini) {
        fout << "|" << mini_.get(mid).full_title() << "|" << class_codes(mid,0) << " " 
             << class_codes(mid, 1) << " " << class_codes(mid, 2) << "|" << mini_.get(mid).priority() 
             << "|" << rooms_.name(room) << "|\n";
      }
    }
    fout << "\n";
  }
}

#endif /* SCHEDULER_H */