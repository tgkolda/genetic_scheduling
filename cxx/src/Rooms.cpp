#include "Rooms.hpp"
#include "yaml-cpp/yaml.h"

Rooms::Rooms(const std::string& filename) {
  YAML::Node nodes = YAML::LoadFile(filename);
  size_ = nodes.size();
  data_.reserve(size_);

  for(auto node : nodes) {
    std::string name = node.first.as<std::string>();
    unsigned capacity = node.second.as<unsigned>();
    data_.push_back(Room(name, capacity));
  }
}

unsigned Rooms::size() const {
  return size_;
}

const std::string& Rooms::name(unsigned i) const {
  return data_[i].name();
}

unsigned Rooms::get_id(const std::string& name) const{
  auto it = find (data_.begin(), data_.end(), name);
  return std::distance(data_.begin(), it);
}