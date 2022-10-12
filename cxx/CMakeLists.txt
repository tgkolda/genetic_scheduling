cmake_minimum_required(VERSION 3.10)

# set the project name
project(Genetic)

find_package(yaml-cpp REQUIRED)
find_package(Kokkos REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

include_directories(include ${YAML_CPP_INCLUDE_DIR})
add_subdirectory(src)