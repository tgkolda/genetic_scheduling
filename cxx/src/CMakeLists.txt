add_library(scheduler Minisymposia.cpp 
                      Minisymposium.cpp)
target_link_libraries(scheduler ${YAML_CPP_LIBRARIES} Kokkos::kokkos)

add_executable(driver driver.cpp)
target_link_libraries(driver scheduler)