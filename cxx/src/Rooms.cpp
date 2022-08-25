#include "Rooms.hpp"
#include "yaml-cpp/yaml.h"

Rooms::Rooms(const std::string& filename) {
  YAML::Node nodes = YAML::LoadFile(filename);
  d_data_ = Kokkos::View<Room*>("rooms", nodes.size());
  h_data_ = Kokkos::create_mirror_view(d_data_);

  unsigned i=0;
  for(auto node : nodes) {
    std::string name = node.first.as<std::string>();
    unsigned capacity = node.second.as<unsigned>();
    h_data_[i] = Room(name, capacity);
    i++;
  }
  Kokkos::deep_copy(d_data_, h_data_);
}

unsigned Rooms::size() const {
  return d_data_.size();
}