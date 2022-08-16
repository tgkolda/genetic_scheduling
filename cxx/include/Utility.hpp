#ifndef UTILITY_H
#define UTILITY_H

namespace genetic {

template<class ViewType, class ValType>
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

} // namespace genetic

#endif /* UTILITY_H */