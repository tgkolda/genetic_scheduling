#include "Minisymposia.hpp"

void Minisymposia::add(const std::string& title, 
                        const std::string& theme, 
                        const std::string& organizer, 
                        const std::vector<std::string>& speakers,
                        unsigned part) 
{
  // Add the theme to themes if it's not already there
  unsigned tid = std::find (themes_.begin(), themes_.end(), theme) - themes_.begin();
  if(tid >= themes_.size()) {
    themes_.push_back(theme);
  }
  temp_data_.push_back(Minisymposium(title, tid, organizer, speakers, part));
}

void Minisymposia::fill_complete() {
  Kokkos::resize(data_, temp_data_.size());
  for(int i=0; i<temp_data_.size(); i++) {
    data_[i] = temp_data_[i];
  }
  temp_data_.clear();

  set_overlapping_participants();
  set_prerequisites();
  set_overlapping_themes();
}

KOKKOS_FUNCTION unsigned Minisymposia::size() const {
  return data_.size();
}

KOKKOS_FUNCTION
const Minisymposium& Minisymposia::operator[](unsigned i) const {
  assert(i < size());
  return data_[i];
}

KOKKOS_FUNCTION
bool Minisymposia::overlaps_participants(unsigned m1, unsigned m2) const {
  return same_participants_(m1, m2);
}

KOKKOS_FUNCTION
bool Minisymposia::breaks_ordering(unsigned m1, unsigned m2) const {
  return is_prereq_(m2, m1);
}

unsigned Minisymposia::get_max_penalty() const {
  return max_penalty_;
}

unsigned Minisymposia::get_max_theme_penalty() const {
  return max_theme_penalty_;
}

const std::vector<std::string>& Minisymposia::themes() const {
  return themes_;
}

void Minisymposia::set_overlapping_participants() {
  size_t nmini = data_.size();
  Kokkos::resize(same_participants_, nmini, nmini);

  for(int i=0; i<nmini; i++) {
    for(int j=i+1; j<nmini; j++) {
      if(data_[i].shares_participant(data_[j])) {
        same_participants_(i,j) = true;
        same_participants_(j,i) = true;
        max_penalty_++;
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
        max_penalty_++;
      }
    }
  }  
}

void Minisymposia::set_overlapping_themes() {
  size_t nmini = data_.size();
  Kokkos::resize(same_themes_, nmini, nmini);

  size_t nthemes = themes_.size();
  std::vector<unsigned> theme_penalties(nthemes);
  for(int i=0; i<nmini; i++) {
    unsigned tid = data_[i].tid();
    theme_penalties[tid]++;
    for(int j=i+1; j<nmini; j++) {
      if(data_[i].shares_theme(data_[j])) {
        same_themes_(i,j) = true;
        same_themes_(j,i) = true;
      }
    }
  }
  for(auto p : theme_penalties) {
    if(p > 1) {
      max_theme_penalty_ += pow(p-1, 2);
    }
  }
}
