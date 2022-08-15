#include "Minisymposia.hpp"

void Minisymposia::add(const Minisymposium& mini) {
  data_.push_back(mini);
}

void Minisymposia::fill_complete() {
  set_overlapping_participants();
  set_prerequisites();
  set_overlapping_themes();
}

std::ostream& operator<<(std::ostream& os, const Minisymposia& mini) {
  for(const auto& m : mini.data_) {
    os << m << "\n";
  }
  return os;
}

void Minisymposia::set_overlapping_participants() {
  size_t nmini = data_.size();
  Kokkos::resize(same_participants_, nmini, nmini);

  for(int i=0; i<nmini; i++) {
    for(int j=i+1; j<nmini; j++) {
      if(data_[i].shares_participant(data_[j])) {
        same_participants_(i,j) = true;
        same_participants_(j,i) = true;
      }
    }
  }
}

void Minisymposia::set_prerequisites() {
  size_t nmini = data_.size();
  Kokkos::resize(is_prereq_, nmini, nmini);

  for(int i=0; i<nmini; i++) {
    for(int j=0; j<nmini; j++) {
      if(i == j) continue;
      if(data_[i].comes_before(data_[j])) {
        is_prereq_(i,j) = true;
      }
    }
  }  
}

void Minisymposia::set_overlapping_themes() {
  size_t nmini = data_.size();
  Kokkos::resize(same_themes_, nmini, nmini);

  for(int i=0; i<nmini; i++) {
    for(int j=i+1; j<nmini; j++) {
      if(data_[i].shares_theme(data_[j])) {
        same_themes_(i,j) = true;
        same_themes_(j,i) = true;
      }
    }
  }
}