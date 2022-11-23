#include "Lectures.hpp"
#include "yaml-cpp/yaml.h"

Lectures::Lectures(const std::string& filename) {
  // Read the lectures from yaml on the host
  YAML::Node nodes = YAML::LoadFile(filename);

  unsigned n = nodes.size();
  titles_.resize(n);
  speakers_.resize(n);
  ids_.resize(n);
  class_codes_ = Kokkos::View<Theme*[3]>("classification codes", n);
  auto h_codes = Kokkos::create_mirror_view(class_codes_);

  unsigned i=0;
  for(auto node : nodes) {
    titles_[i] = node.first.as<std::string>();
    ids_[i] = node.second["id"].as<unsigned>();
    speakers_[i] = node.second["speaker"].as<std::string>();
    std::vector<unsigned> codes = node.second["class codes"].as<std::vector<unsigned>>();
    assert(codes.size() == 3);
    for(unsigned j=0; j<3; j++) {
      h_codes(i,j) = codes[j];
    }

    printf("Lecture %i is %s\n", i, titles_[i].c_str());
    i++;
  }

  // Copy the data to device
  Kokkos::deep_copy(class_codes_, h_codes);
}

unsigned Lectures::size() const {
  return class_codes_.extent(0);
}

unsigned Lectures::topic_cohesion_score(unsigned first, unsigned second) const {
  unsigned score = 0;

  for(unsigned i=0; i<3; i++) {
    for(unsigned j=0; j<3; j++) {
      if(class_codes_(first,i) == class_codes_(second,j)) {
        score++;
      }
    }
  }

  return pow(score,2);
}

KOKKOS_FUNCTION unsigned Lectures::topic_cohesion_score(const Minisymposia& mini, unsigned mid, unsigned lid) const {
  unsigned score = 0;

  for(unsigned i=0; i<3; i++) {
    for(unsigned j=0; j<3; j++) {
      if(class_codes_(lid,i) == mini.class_codes(mid,j)) {
        score++;
      }
    }
  }

  return pow(score,2);  
}

const std::string& Lectures::title(unsigned index) const {
  return titles_[index];
}

unsigned Lectures::id(unsigned index) const {
  return ids_[index];
}

Kokkos::View<Theme*[3]>::HostMirror Lectures::class_codes() const {
  auto h_codes = Kokkos::create_mirror_view(class_codes_);
  Kokkos::deep_copy(h_codes, class_codes_);
  return h_codes;
}