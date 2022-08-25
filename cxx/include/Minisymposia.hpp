#ifndef MINISYMPOSIA_H
#define MINISYMPOSIA_H

#include "Minisymposium.hpp"
#include <Kokkos_Core.hpp>
#include <ostream>
#include <set>
#include <vector>

class Minisymposia {
public:
  void add(const std::string& title, 
           const std::string& theme, 
           const std::string& organizer, 
           const std::vector<std::string>& speakers,
           unsigned part);
  void fill_complete();
  KOKKOS_FUNCTION unsigned size() const;
  KOKKOS_FUNCTION const Minisymposium& operator[](unsigned i) const;

  KOKKOS_FUNCTION bool overlaps_participants(unsigned m1, unsigned m2) const;
  KOKKOS_FUNCTION bool breaks_ordering(unsigned m1, unsigned m2) const;
  unsigned get_max_penalty() const;
  unsigned get_max_theme_penalty() const;
  const std::vector<std::string>& themes() const;

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  void set_overlapping_participants();
  void set_prerequisites();
  void set_overlapping_themes();

  std::vector<Minisymposium> temp_data_;
  std::vector<std::string> themes_;
  Kokkos::View<Minisymposium*> data_;
  Kokkos::View<bool**> same_participants_;
  Kokkos::View<bool**> is_prereq_;
  Kokkos::View<bool**> same_themes_;
  unsigned max_penalty_{1};
  unsigned max_theme_penalty_{0};
};

#endif /* MINISYMPOSIA_H */