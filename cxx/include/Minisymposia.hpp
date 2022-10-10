#ifndef MINISYMPOSIA_H
#define MINISYMPOSIA_H

#include "Minisymposium.hpp"
#include <Kokkos_Core.hpp>
#include <ostream>
#include <set>
#include <vector>

class Minisymposia {
public:
  Minisymposia(const std::string& filename, unsigned nrooms, unsigned nslots);
  Minisymposia(const Minisymposia&) = default;
  ~Minisymposia() = default;
  Minisymposia& operator=(const Minisymposia&) = delete;
  

  KOKKOS_FUNCTION unsigned size() const;
  KOKKOS_FUNCTION const Minisymposium& operator[](unsigned i) const;

  KOKKOS_FUNCTION bool overlaps_participants(unsigned m1, unsigned m2) const;
  KOKKOS_FUNCTION bool breaks_ordering(unsigned m1, unsigned m2) const;
  unsigned get_max_penalty() const;
  const std::vector<std::string>& themes() const;
  void set_overlapping_participants();
  void set_prerequisites();
  KOKKOS_FUNCTION double map_theme_penalty(unsigned nproblems) const;
  KOKKOS_FUNCTION double map_priority_penalty(unsigned nproblems) const;
  void set_overlapping_themes(unsigned nrooms, unsigned nslots);
  void set_priorities(unsigned nslots);
  void set_priority_penalty_bounds(unsigned nslots);
  const std::string& get_title(unsigned i) const;
  const std::string& get_theme(unsigned i) const;
  unsigned get_priority(unsigned i) const;

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  std::vector<std::string> themes_;
  Kokkos::View<Minisymposium*> d_data_;
  Kokkos::View<Minisymposium*>::HostMirror h_data_;
  Kokkos::View<bool**> same_participants_;
  Kokkos::View<bool**> is_prereq_;
  Kokkos::View<bool**> same_themes_;
  unsigned max_penalty_{1};
  unsigned min_theme_penalty_{0};
  unsigned max_theme_penalty_{0};
  unsigned min_priority_penalty_{0};
  unsigned max_priority_penalty_{0};
};

#endif /* MINISYMPOSIA_H */