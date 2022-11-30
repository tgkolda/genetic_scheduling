#include "Timeslots.hpp"
#include "yaml-cpp/yaml.h"

Timeslots::Timeslots(const std::string& filename) {
  YAML::Node nodes = YAML::LoadFile(filename);
  size_ = nodes.size();
  nlectures_.resize(size_);

  for(auto node : nodes) {
    unsigned timeslot = node.first.as<unsigned>();
    unsigned capacity = node.second.as<unsigned>();
    nlectures_[timeslot-1] = capacity;
  }
}

unsigned Timeslots::nlectures(unsigned i) const {
  return nlectures_[i];
}

unsigned Timeslots::size() const {
  return size_;
}