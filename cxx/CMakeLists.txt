cmake_minimum_required(VERSION 3.10)

# set the project name
project(Genetic)

find_package(yaml-cpp REQUIRED YES)

include_directories(include ${YAML_CPP_INCLUDE_DIR})
add_subdirectory(src)