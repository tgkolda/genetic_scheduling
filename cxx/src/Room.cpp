#include "Room.hpp"

Room::Room(const std::string& name, unsigned capacity) :
  name_(name),
  capacity_(capacity)
{
  
}

const std::string& Room::name() const {
  return name_;
}