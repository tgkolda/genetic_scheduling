#include "yaml-cpp/yaml.h"
#include "Minisymposia.hpp"

Minisymposia::Minisymposia(const std::string& filename) {
  // Read the minisymposia from yaml on the host
  YAML::Node nodes = YAML::LoadFile(filename);

  unsigned n = nodes.size();
  d_data_ = Kokkos::View<Minisymposium*>("minisymposia", n);
  h_data_ = Kokkos::create_mirror_view(d_data_);

  unsigned i=0;
  for(auto node : nodes) {
    std::string title = node.first.as<std::string>();
    std::string theme = node.second["predicted_theme"].as<std::string>();
    unsigned part = 1;
    if(node.second["part"])
      part = node.second["part"].as<unsigned>();
    std::string organizer;
    if(node.second["organizer"])
      organizer = node.second["organizer"].as<std::string>();
    std::vector<std::string> speakers = node.second["speakers"].as<std::vector<std::string>>();

    // Add the theme to themes if it's not already there
    unsigned tid = std::find (themes_.begin(), themes_.end(), theme) - themes_.begin();
    if(tid >= themes_.size()) {
      themes_.push_back(theme);
    }

    h_data_[i] = Minisymposium(title, tid, organizer, speakers, part);
    i++;
  }
  
  // Copy the data to device
  Kokkos::deep_copy(d_data_, h_data_);

  set_overlapping_participants();
  set_prerequisites();
  set_overlapping_themes();

}

KOKKOS_FUNCTION unsigned Minisymposia::size() const {
  return d_data_.extent(0);
}

KOKKOS_FUNCTION
const Minisymposium& Minisymposia::operator[](unsigned i) const {
  assert(i < size());
  return d_data_[i];
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
  using Kokkos::parallel_reduce;
  using Kokkos::DefaultHostExecutionSpace;
  using Kokkos::RangePolicy;

  size_t nmini = size();
  same_participants_ = Kokkos::View<bool**>("overlapping participants", nmini, nmini);
  auto h_same_participants = Kokkos::create_mirror_view(same_participants_);

  unsigned overlap_penalty = 0;
  RangePolicy<DefaultHostExecutionSpace> rp(DefaultHostExecutionSpace(), 0, nmini);
  parallel_reduce("set overlapping participants", rp, [=] (unsigned i, unsigned& lpenalty ) {
    for(int j=0; j<nmini; j++) {
      if(i == j) continue;
      if(h_data_[i].shares_participant(h_data_[j])) {
        h_same_participants(i,j) = true;
        lpenalty++;
      }
    }
  }, overlap_penalty);
  Kokkos::deep_copy(same_participants_, h_same_participants);
  max_penalty_ += overlap_penalty;
}

void Minisymposia::set_prerequisites() {
  using Kokkos::parallel_reduce;
  using Kokkos::DefaultHostExecutionSpace;
  using Kokkos::RangePolicy;

  size_t nmini = size();
  is_prereq_ = Kokkos::View<bool**>("prerequisites", nmini, nmini);
  auto h_is_prereq = Kokkos::create_mirror_view(is_prereq_);

  unsigned prereq_penalty = 0;
  RangePolicy<DefaultHostExecutionSpace> rp(DefaultHostExecutionSpace(), 0, nmini);
  parallel_reduce("set prerequisites", rp, [=] (unsigned i, unsigned& lpenalty ) {
    for(int j=0; j<nmini; j++) {
      if(i == j) continue;
      if(h_data_[i].comes_before(h_data_[j])) {
        h_is_prereq(i,j) = true;
        lpenalty++;
      }
    }
  }, prereq_penalty);
  Kokkos::deep_copy(is_prereq_, h_is_prereq);
  max_penalty_ += prereq_penalty; 
}

void Minisymposia::set_overlapping_themes() {
  using Kokkos::parallel_for;
  using Kokkos::parallel_reduce;
  using Kokkos::DefaultHostExecutionSpace;
  using Kokkos::RangePolicy;

  size_t nmini = size();
  same_themes_ = Kokkos::View<bool**>("overlapping themes", nmini, nmini);
  auto h_same_themes = Kokkos::create_mirror_view(same_themes_);

  size_t nthemes = themes_.size();
  Kokkos::View<unsigned*, Kokkos::HostSpace, Kokkos::MemoryTraits<Kokkos::Atomic>> theme_penalties("theme penalties", nthemes);
  RangePolicy<DefaultHostExecutionSpace> rp(DefaultHostExecutionSpace(), 0, nmini);
  parallel_for("set overlapping themes", rp, [=] (unsigned i) {
    unsigned tid = h_data_[i].tid();
    theme_penalties[tid]++;
    for(unsigned j=0; j<nmini; j++) {
      if(i == j) continue;
      if(h_data_[i].shares_theme(h_data_[j])) {
        h_same_themes(i,j) = true;
      }
    }
  });
  Kokkos::deep_copy(same_themes_, h_same_themes);
  
  RangePolicy<DefaultHostExecutionSpace> rp2(DefaultHostExecutionSpace(), 0, nthemes);
  parallel_reduce("compute theme penalty", rp2, [=] (unsigned i, unsigned& lpenalty) {
    auto p = theme_penalties[i];
    if(p > unsigned(1)) {
      lpenalty += pow(p-unsigned(1), 2);
    }
  }, max_theme_penalty_);
}
