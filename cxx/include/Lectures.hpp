#ifndef LECTURES_H
#define LECTURES_H

#include <Kokkos_Core.hpp>
#include "Minisymposia.hpp"

class Lectures {
public:
  Lectures(const std::string& filename);
  KOKKOS_FUNCTION unsigned size() const;
  KOKKOS_FUNCTION unsigned topic_cohesion_score(unsigned first, unsigned second) const;
  KOKKOS_FUNCTION unsigned topic_cohesion_score(const Minisymposia& mini, unsigned mid, unsigned lid) const;
  const std::string& title(unsigned index) const;
  Kokkos::View<Theme*[3]>::HostMirror class_codes() const;
private:
  std::vector<std::string> titles_;
  std::vector<std::string> speakers_;
  Kokkos::View<Theme*[3]> class_codes_;
};

#endif /* LECTURES_H */