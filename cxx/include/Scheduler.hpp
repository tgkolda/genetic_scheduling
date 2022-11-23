#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Minisymposia.hpp"
#include "Rooms.hpp"
#include "Schedule.hpp"
#include "Utility.hpp"
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
  KOKKOS_INLINE_FUNCTION double rate(View2D schedule, bool verbose=false) const;

  template<class View2D>
  KOKKOS_INLINE_FUNCTION void fix_order(View2D schedule, bool verbose=false) const;

  template<class View2D>
  inline void record(const std::string& filename, View2D schedule) const;

  KOKKOS_FUNCTION bool out_of_bounds(unsigned i) const;
  KOKKOS_FUNCTION unsigned nslots() const;
  KOKKOS_FUNCTION unsigned nrooms() const;
  void record(const std::string& filename) const;

private:
  Rooms rooms_;
  Minisymposia mini_;
  unsigned ntimeslots_;
};

template<class View2D>
double Scheduler::rate(View2D schedule, bool verbose) const {
  fix_order(schedule, verbose);

  unsigned order_penalty, oversubscribed_penalty, priority_penalty;
  double theme_penalty;
  double result = mini_.rate_schedule(schedule, order_penalty, oversubscribed_penalty, 
                                      theme_penalty, priority_penalty);
  return result;
}

template<class View2D>
void Scheduler::fix_order(View2D schedule, bool verbose) const {
  unsigned nmini = mini_.size();

  // If we can put multi-part minisymposia in order, do that
  for(unsigned sl1=0; sl1<nslots(); sl1++) {
    for(unsigned r1=0; r1<nrooms(); r1++) {
      if(schedule(sl1,r1) >= nmini) continue;
      for(unsigned sl2=sl1+1; sl2<nslots(); sl2++) {
        for(unsigned r2=0; r2<nrooms(); r2++) {
          if(schedule(sl2,r2) >= nmini) continue;
          if(mini_.breaks_ordering(schedule(sl1,r1), schedule(sl2,r2))) {
            // swap the values
            genetic::swap(schedule(sl1, r1), schedule(sl2, r2));
          }
        }
      }
    }
  }

  // Sort the minisymposia in each slot based on the room priority
  // Assign minisymposia to the correct rooms if possible
  for(unsigned sl=0; sl<nslots(); sl++) {
    for(unsigned i=0; i<nrooms(); i++) {
      auto m1 = schedule(sl,i);
      unsigned min_index = i;
      unsigned min_value = unsigned(-1);
      if(m1 < nmini) {
        if(mini_[m1].room_id() == i) {
          if(verbose) {
            printf("Minisymposium %i is already in the correct room\n", m1);
          }
          continue;
        }
        min_value = mini_[m1].priority();
      }
      for(unsigned j=i+1; j<nrooms(); j++) {
        auto m2 = schedule(sl,j);
        if(m2 >= nmini) continue;
        // If this item is supposed to be in room i, put it there
        if(mini_[m2].room_id() == i) {
          if(verbose) {
            printf("assigning %i at position %i to room %i as requested\n", m2, j, i);
          }
          min_index = j;
          min_value = 0;
          break;
        }
        if(mini_[m2].priority() < min_value) {
          min_index = j;
          min_value = mini_[m2].priority();
        }
      }
      if(min_index != i) {
        genetic::swap(schedule(sl,i), schedule(sl, min_index));
      }
      if(verbose) {
        auto mid = schedule(sl,i);
        if(mid < nmini) {
          unsigned priority = mini_[mid].priority();
          unsigned rid = mini_[mid].room_id();
          printf("schedule(%i,%i) = %i with priority %i and room id %i\n", sl, i, mid, priority, rid);
        }
        else {
          printf("schedule(%i,%i) = %i\n", sl, i, mid);
        }
      }
    }
  }
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