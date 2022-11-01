add_library(scheduler Lectures.cpp
                      Mapper.cpp
                      Minisymposia.cpp 
                      Minisymposium.cpp
                      Room.cpp
                      Rooms.cpp
                      Schedule.cpp
                      Scheduler.cpp)
target_link_libraries(scheduler ${YAML_CPP_LIBRARIES} Kokkos::kokkos Qt5::Widgets)

add_executable(mini-assignments mini-assignments-driver.cpp)
target_link_libraries(mini-assignments scheduler Qt5::Core)