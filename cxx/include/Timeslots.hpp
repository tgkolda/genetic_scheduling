#ifndef TIMESLOTS_H
#define TIMESLOTS_H

#include "Kokkos_Core.hpp"

#include<string>
#include<vector>

class Timeslots {
public:
  Timeslots() = default;
  Timeslots(const std::string& filename);
  Timeslots(const Timeslots&) = default;
  ~Timeslots() = default;
  Timeslots& operator=(const Timeslots&) = default;

  unsigned nlectures(unsigned i) const;
  KOKKOS_FUNCTION unsigned size() const;
private:
  std::vector<unsigned> nlectures_;
  unsigned size_;
};

#endif /* TIMESLOTS_H */