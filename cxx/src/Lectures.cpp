#include "Lectures.hpp"
#include "yaml-cpp/yaml.h"

Lectures::Lectures(const std::string& filename) {
  // Read the lectures from yaml on the host
  YAML::Node nodes = YAML::LoadFile(filename);

  unsigned n = nodes.size();
  titles_.resize(n);
  speakers_.resize(n);
  class_codes_ = Kokkos::View<unsigned*[3]>("classification codes", n);
  auto h_codes = Kokkos::create_mirror_view(class_codes_);

  unsigned i=0;
  for(auto node : nodes) {
    titles_[i] = node.first.as<std::string>();
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