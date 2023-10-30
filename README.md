# An automated conference scheduler
Scheduling large conferences (such as SIAM CSE) is quite difficult. There are many constraints which must be taken into account, such as
1. Speaker conflicts - No speaker or minisymposium organizer can be in two rooms at the same time.
2. Room equipment - Minitutorials and certain minisymposia require rooms with special setups.
3. Timeslot suitability - Some speakers and minisymposium organizers are only available during certain timeslots. Because timeslots are not always the same length, longer minisymposia cannot be scheduled in short timeslots.
4. Multipart sequence - Some minisymposia have multiple parts. Part I must come before Part II, which must come before Part III, and so forth.
5. Room size - More popular minisymposia should take place in larger rooms.
6. Topic - Minisymposia that address similar topics should be assigned to different timeslots, i.e. all machine learning minisymposia should not take place simultaneously.
7. Multipart adjacency - Multipart minisymposia should ideally occur in the same room during adjacent timeslots.

## Tools
This repository includes a variety of tools intended to help automatically schedule conferences.

### Estimating popularity of a given minisymposia
data/popularity.py data mines citation counts from Google Scholar's API. It only includes citations of publications within the last 5 years to give people in their early careers a fair chance.

### Estimating topic similarity of minisymposia
data/theme_clustering.py clusters minisymposia based on their abstracts. Also returns the most common words for each cluster so the user understands approximately what topic the cluster represents.

### Assigning lectures to minisymposia
Executable cxx/mini-assignments uses a genetic algorithm to assign contributed lectures to minisymposia.

### Assigning minisymposia to rooms and timeslots
Executable cxx/schedule-mini uses a genetic algoritm to generate a conference schedule. A Qt GUI is also provided to allow the user to tweak an existing schedule, being alerted to whether their changes have made the schedule unfeasible.

## Requirements
The C++ executables use the CMake build system and have the following dependencies
* Qt
* Kokkos
* yaml-cpp
