#ifndef ROOMS_H
#define ROOMS_H

#include "Room.hpp"
#include "Kokkos_Core.hpp"
#include <string>

class Rooms {
public:
  Rooms(const std::string& filename);
  KOKKOS_FUNCTION unsigned size() const;
  const std::string& name(unsigned i) const;
private:
  Kokkos::View<Room*> d_data_;
  Kokkos::View<Room*>::HostMirror h_data_;
};

#endif /* ROOMS_H */