#ifndef MINISYMPOSIA_H
#define MINISYMPOSIA_H

#include "Minisymposium.hpp"
#include <Kokkos_Core.hpp>
#include <ostream>
#include <vector>

class Minisymposia {
public:
  void add(const Minisymposium& mini);
  void fill_complete();
  unsigned size() const;

  bool overlaps_participants(unsigned m1, unsigned m2) const;
  bool breaks_ordering(unsigned m1, unsigned m2) const;
  unsigned get_max_penalty() const;

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  void set_overlapping_participants();
  void set_prerequisites();
  void set_overlapping_themes();

  std::vector<Minisymposium> data_;
  Kokkos::View<bool**> same_participants_;
  Kokkos::View<bool**> is_prereq_;
  Kokkos::View<bool**> same_themes_;
  unsigned max_penalty_{0};
};

#endif /* MINISYMPOSIA_H */