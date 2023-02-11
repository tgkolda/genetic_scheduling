# genetic_scheduling
A genetic scheduling code for SIAM CSE23

## Installation
This code depends on the following software; in parentheses are the exact specs this code has been tested with.
* kokkos `(kokkos@develop+cuda+cuda_constexpr+cuda_lambda+cuda_relocatable_device_code+wrapper~shared+openmp cuda_arch=75 std=17)`
* yaml-cpp `(yaml-cpp@0.6.3~ipo+pic+shared~tests build_type=RelWithDebInfo)`
* qt `(qt@5.15.5~dbus~debug~doc~examples~framework~gtk+gui+opengl~phonon+shared+sql+ssl+tools~webkit patches=2081e9c,51aeba5,75bcb42,7f34d48,84b0991,8cd4be9,9378afd)`
* cmake

The C++ code can be built with CMake.

## Considerations
* Speakers can not be in two minisymposia at the same time. We should attempt to prevent coauthors from being in two minisymposia at the same time, but that is less important.
* Minisymposia with the same keywords should not take place at the same time. A person who wants to attend a linear algebra presentation will likely want to attend other linear algebra presentations
* Some minisymposia have multiple sessions which cannot take place at the same time; these are denoted by the word 'part'.
* The rooms have dramatically different sizes. The minisymposia have an additional piece of metadata estimating their popularity so the rooms can be assigned accordingly. The popularity is estimated by querying citation counts from google scholar.
* Minitutorials are being treated as minisymposia, but they may require additional equipment in the rooms (such as tables/desks). 
