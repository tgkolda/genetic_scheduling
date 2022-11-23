#include "Room.hpp"

Room::Room(const std::string& name, unsigned capacity) :
  name_(name),
  capacity_(capacity)
{
  
}

const std::string& Room::name() const {
  return name_;
}

bool Room::operator==(const std::string& name) const {
  return name == name_;
}