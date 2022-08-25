#ifndef ROOMS_H
#define ROOMS_H

#include "Room.hpp"
#include "Kokkos_Core.hpp"
#include <string>

class Rooms {
public:
  Rooms(const std::string& filename);
  unsigned size() const;
private:
  Kokkos::View<Room*> d_data_;
  Kokkos::View<Room*>::HostMirror h_data_;
};

#endif /* ROOMS_H */