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

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  void set_overlapping_participants();
  void set_prerequisites();
  void set_overlapping_themes();

  std::vector<Minisymposium> data_;
  Kokkos::View<bool**> same_participants_;
  Kokkos::View<bool**> is_prereq_;
  Kokkos::View<bool**> same_themes_;
};

#endif /* MINISYMPOSIA_H */