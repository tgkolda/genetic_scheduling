#include "Minisymposia.hpp"
#include "yaml-cpp/yaml.h"
#include <iostream>

int main() {
  YAML::Node nodes = YAML::LoadFile("../../data/minisymposia_predicted_theme.yaml");
  Minisymposia mini;

  for(auto node : nodes) {
    std::string title = node.first.as<std::string>();
    std::string theme = node.second["predicted_theme"].as<std::string>();
    unsigned part = 1;
    if(node.second["part"])
      part = node.second["part"].as<unsigned>();
    std::string organizer;
    if(node.second["organizer"])
      organizer = node.second["organizer"].as<std::string>();
    std::vector<std::string> speakers = node.second["speakers"].as<std::vector<std::string>>();

    mini.add(Minisymposium(title, theme, organizer, speakers, part));
  }

  std::cout << mini << "\n";

  return 0;
}