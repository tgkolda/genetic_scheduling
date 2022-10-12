add_library(scheduler Minisymposia.cpp 
                      Minisymposium.cpp
                      Room.cpp
                      Rooms.cpp
                      Schedule.cpp
                      Scheduler.cpp)
target_link_libraries(scheduler ${YAML_CPP_LIBRARIES} Kokkos::kokkos Qt5::Widgets)

add_executable(driver driver.cpp)
target_link_libraries(driver scheduler Qt5::Core)