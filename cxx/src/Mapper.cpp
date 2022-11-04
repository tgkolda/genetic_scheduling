#include "Mapper.hpp"
#include "Utility.hpp"

Mapper::Mapper(const Lectures& lectures, const Minisymposia& minisymposia) :
  lectures_(lectures), minisymposia_(minisymposia) { }

Mapper::ViewType Mapper::make_initial_population(unsigned popSize) {
  unsigned nlectures = lectures_.size();
  unsigned nmini = minisymposia_.size();
  unsigned ngenes = nmini + 5*10; // Only allow 10 extra minisymposia
  return Kokkos::View<unsigned**>("mappings", popSize, ngenes);
}

void Mapper::smush() {
/*
  unsigned nmappings = current_mappings_.extent(0);
  unsigned nlectures = lectures_.size();
  unsigned nmini = minisymposia_.size();
  unsigned ngenes = current_mappings_.extent(1);
  Kokkos::parallel_for("Smashing!", nmappings, KOKKOS_CLASS_LAMBDA(unsigned m) {
    // Find a minisymposium that needs to move up in the schedule
    unsigned empty_index = ngenes;
    for(unsigned i=nmini; i+4<ngenes; i+=5) {
      // Determine whether this minisymposium includes any talks
      bool location_empty = true;
      for(unsigned j=0; j<5; j++) {
        if(current_mappings_(m,i+j) < nmini) {
          location_empty = false;
          break;
        }
      }
      if(!location_empty && empty_index < ngenes) {
        for(unsigned j=0; j<5; j++) {
          unsigned temp = current_mappings_(m,i+j);
          current_mappings_(m,i+j) = current_mappings_(m,empty_index+j);
          current_mappings_(m,empty_index+j) = temp;
        }
        i = empty_index;
        empty_index = ngenes;
      }
      if(location_empty && empty_index == ngenes) {
        empty_index = i;
      }
    }
  });

  // Block until the smashing is complete since the next step uses the results
  Kokkos::fence();
*/
}

bool Mapper::out_of_bounds(unsigned i) const {
  return i >= lectures_.size();
}