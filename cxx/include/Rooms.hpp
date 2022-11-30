#ifndef ROOMS_H
#define ROOMS_H

#include "Room.hpp"
#include "Kokkos_Core.hpp"
#include <string>

class Rooms {
public:
  Rooms() = default;
  Rooms(const std::string& filename);
  
  KOKKOS_FUNCTION unsigned size() const;
  const std::string& name(unsigned i) const;
  unsigned get_id(const std::string& name) const;
private:
  std::vector<Room> data_;
  unsigned size_;
};

#endif /* ROOMS_H */