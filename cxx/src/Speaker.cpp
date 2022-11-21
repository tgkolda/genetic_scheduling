#include "Speaker.hpp"
#include "yaml-cpp/yaml.h"

std::unordered_map<std::string, unsigned> Speaker::citation_map_{};

Speaker::Speaker(const std::string& name) : name_(name) { 
  
}

bool Speaker::operator==(const Speaker& speaker) const {
  return name_ == speaker.name_;
}

bool Speaker::operator<(const Speaker& speaker) const {
  return name_ < speaker.name_;
}

bool Speaker::empty() const {
  return name_.empty();
}

const std::string& Speaker::name() const {
  return name_;
}

unsigned Speaker::citations() const {
  // unordered_map.contains is a C++20 feature
  if(citation_map_.find(name_) != citation_map_.end()) {
    return citation_map_[name_];
  }
  return 0;
}

void Speaker::read(const std::string& filename) {
  // Read the citations from yaml
  YAML::Node nodes = YAML::LoadFile(filename);

  for(auto node : nodes) {
    std::string name = node.first.as<std::string>();
    unsigned citations = node.second.as<unsigned>();
    citation_map_[name] = citations;
  }
}