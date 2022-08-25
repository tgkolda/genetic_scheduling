#ifndef ROOM_H
#define ROOM_H

#include <string>

class Room {
public:
  Room() = default;
  Room(const std::string& name, unsigned capacity);
private:
  std::string name_;
  unsigned capacity_;
};

#endif /* ROOM_H */