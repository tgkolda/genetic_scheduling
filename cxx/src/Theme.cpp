#include "Theme.hpp"
#include "yaml-cpp/yaml.h"
#include <exception>

std::unordered_map<unsigned,std::string> Theme::theme_map_{};

Theme& Theme::operator=(unsigned id) {
  id_ = id;
  return *this;
}

bool Theme::operator==(const Theme& theme) const {
  return id_ == theme.id_;
}

const std::string& Theme::name() const {
  try {
    return theme_map_.at(id_);
  }
  catch (std::exception& e) {
    // If the number ends in 99, do something special
    if(id_ % 100 == 99) {
      return theme_map_[stem()];
    }
    theme_map_[id_] = "UNKNOWN THEME: " + std::to_string(id_);
    return theme_map_[id_];
  }
}

unsigned Theme::stem() const {
  // Chop off the last two digits
  return (id_/100)*100;
}

Similarity Theme::compare(const Theme& theme) const {
  if (*this == theme) {
    return IDENTICAL;
  }
  else if(stem() == theme.stem()) {
    return SIMILAR;
  }
  return DIFFERENT;
}

void Theme::read(const std::string& filename) {
  // Read the themes from yaml
  YAML::Node nodes = YAML::LoadFile(filename);

  for(auto node : nodes) {
    unsigned id = node.first.as<unsigned>();
    std::string str = node.second.as<std::string>();
    theme_map_[id] = str;
  }
}

std::ostream& operator<<(std::ostream& os, const Theme& theme) {
  os << theme.name();
  return os;
}

unsigned compute_topic_score(unsigned lid, unsigned mid, 
  Kokkos::View<Theme*[3]>::HostMirror lecture_codes,
  Kokkos::View<Theme*[3]>::HostMirror mini_codes) 
{
  unsigned result = 0;
  for(unsigned i=0; i<3; i++) {
    for(unsigned j=0; j<3; j++) {
      result += lecture_codes(lid,i).compare(mini_codes(mid, j));
    }
  }
  return result;
}

unsigned compute_topic_score(unsigned lid1, unsigned lid2, 
  Kokkos::View<Theme*[3]>::HostMirror lecture_codes)
{
  unsigned result = 0;
  for(unsigned i=0; i<3; i++) {
    for(unsigned j=0; j<3; j++) {
      result += lecture_codes(lid1,i).compare(lecture_codes(lid2, j));
    }
  }
  return result;
}