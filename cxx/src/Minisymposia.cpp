#include "Minisymposia.hpp"

void Minisymposia::add(const Minisymposium& mini) {
  data_.push_back(mini);
}

std::ostream& operator<<(std::ostream& os, const Minisymposia& mini) {
  for(const auto& m : mini.data_) {
    os << m << "\n";
  }
  return os;
}