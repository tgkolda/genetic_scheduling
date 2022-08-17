#include "Scheduler.hpp"
#include "yaml-cpp/yaml.h"
#include <iostream>

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    // Read the minisymposia from yaml
    YAML::Node mini_nodes = YAML::LoadFile("../../data/minisymposia_predicted_theme.yaml");
    Minisymposia mini;

    for(auto node : mini_nodes) {
      std::string title = node.first.as<std::string>();
      std::string theme = node.second["predicted_theme"].as<std::string>();
      unsigned part = 1;
      if(node.second["part"])
        part = node.second["part"].as<unsigned>();
      std::string organizer;
      if(node.second["organizer"])
        organizer = node.second["organizer"].as<std::string>();
      std::vector<std::string> speakers = node.second["speakers"].as<std::vector<std::string>>();

      mini.add(title, theme, organizer, speakers, part);
    }
    mini.fill_complete();

    // Read the rooms from yaml
    // Read the minisymposia from yaml
    YAML::Node room_nodes = YAML::LoadFile("../../data/rooms.yaml");
    std::vector<Room> rooms;

    for(auto node : room_nodes) {
      std::string name = node.first.as<std::string>();
      unsigned capacity = node.second.as<unsigned>();
      rooms.push_back(Room(name, capacity));
    }

    // Run the genetic algorithm
    Scheduler s(mini, rooms, 13);
    s.run_genetic(1000, 200, 0.01, 1000);
  }
  Kokkos::finalize();
  return 0;
}