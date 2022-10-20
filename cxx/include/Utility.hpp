#ifndef UTILITY_H
#define UTILITY_H

#include "Kokkos_Core.hpp"

namespace genetic {

template<class ViewType, class ValType>
KOKKOS_FUNCTION
bool contains(ViewType view, ValType val) {
  for(unsigned i=0; i<view.extent(0); i++) {
    for(unsigned j=0; j<view.extent(1); j++) {
      if(view(i,j) == val) {
        return true;
      }
    }
  }
  return false;
}

template<class ViewType, class ValType>
KOKKOS_FUNCTION
bool find(ViewType view, ValType val, Kokkos::pair<size_t, size_t>& index) {
  for(unsigned i=0; i<view.extent(0); i++) {
    for(unsigned j=0; j<view.extent(1); j++) {
      if(view(i,j) == val) {
        index.first = i;
        index.second = j;
        return true;
      }
    }
  }
  return false;
}

} // namespace genetic

#endif /* UTILITY_H */