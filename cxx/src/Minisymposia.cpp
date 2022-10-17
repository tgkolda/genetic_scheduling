#include "yaml-cpp/yaml.h"
#include "Minisymposia.hpp"

Minisymposia::Minisymposia(const std::string& filename, unsigned nrooms, unsigned nslots) {
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
    double citations = 0;
    if(node.second["average citation count"])
      citations = node.second["average citation count"].as<double>();
    std::string organizer;
    if(node.second["organizer"])
      organizer = node.second["organizer"].as<std::string>();
    std::vector<std::string> speakers = node.second["speakers"].as<std::vector<std::string>>();

    // Add the theme to themes if it's not already there
    unsigned tid = std::find (themes_.begin(), themes_.end(), theme) - themes_.begin();
    if(tid >= themes_.size()) {
      themes_.push_back(theme);
    }

    printf("%s has %lf citations\n", title.c_str(), citations);
    h_data_[i] = Minisymposium(title, tid, organizer, speakers, citations, part);
    i++;
  }

  // Copy the data to device
  Kokkos::deep_copy(d_data_, h_data_);

  set_overlapping_participants();
  set_prerequisites();
  set_overlapping_themes(nrooms, nslots);
  set_priorities(nslots);
  set_priority_penalty_bounds(nslots);

  // Copy the data to device
  Kokkos::deep_copy(d_data_, h_data_);
}

KOKKOS_FUNCTION unsigned Minisymposia::size() const {
  return d_data_.extent(0);
}

KOKKOS_FUNCTION
const Minisymposium& Minisymposia::operator[](unsigned i) const {
  assert(i < size());
  return d_data_[i];
}

const Minisymposium& Minisymposia::get(unsigned i) const {
  assert(i < size());
  return h_data_[i];
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

KOKKOS_FUNCTION
double Minisymposia::map_theme_penalty(unsigned nproblems) const {
  return (nproblems - min_theme_penalty_) / double(max_theme_penalty_ - min_theme_penalty_);
}

KOKKOS_FUNCTION
double Minisymposia::map_priority_penalty(unsigned nproblems) const {
  return (nproblems - min_priority_penalty_) / double(max_priority_penalty_ - min_priority_penalty_);
}

void Minisymposia::set_overlapping_themes(unsigned nrooms, unsigned nslots) {
  using Kokkos::parallel_for;
  using Kokkos::parallel_reduce;
  using Kokkos::DefaultHostExecutionSpace;
  using Kokkos::RangePolicy;

  size_t nmini = size();
  same_themes_ = Kokkos::View<bool**>("overlapping themes", nmini, nmini);
  auto h_same_themes = Kokkos::create_mirror_view(same_themes_);

  size_t nthemes = themes_.size();
  Kokkos::View<unsigned*, Kokkos::HostSpace> theme_penalties("theme penalties", nthemes);
  for(unsigned i=0; i<nmini; i++) {
    unsigned tid = h_data_[i].tid();
    theme_penalties[tid]++;
    for(unsigned j=0; j<nmini; j++) {
      if(i == j) continue;
      if(h_data_[i].shares_theme(h_data_[j])) {
        h_same_themes(i,j) = true;
      }
    }
  }
  Kokkos::deep_copy(same_themes_, h_same_themes);
  
  for(unsigned i=0; i<nthemes; i++) {
    for(int p = theme_penalties[i]; p > 1; p -= nrooms) {
      max_theme_penalty_ += pow(Kokkos::min(unsigned(p),nrooms)-1, 2);
    }
  }

  for(unsigned i=0; i<nthemes; i++) {
    unsigned nperslot = theme_penalties[i] / nslots;
    unsigned remainder = theme_penalties[i] % nslots;
    if(nperslot > 0) {
      min_theme_penalty_ += remainder * pow(nperslot,2) + (nslots - remainder) * pow(nperslot-1,2);
    }
  }

  printf("Theme penalty bounds: %i %i\n", min_theme_penalty_, max_theme_penalty_);
}

void Minisymposia::set_priorities(unsigned nslots) {
  // Get the citations
  std::vector<double> citation_list(size());
  for(unsigned i=0; i<size(); i++) {
    citation_list[i] = h_data_[i].average_citation_count();
  }

  // Sort the citations from most to least popular
  std::sort(citation_list.begin(), citation_list.end(), std::greater<double>());

  // Get the cutoffs
  unsigned nrooms_needed = ceil(double(size())/nslots);
  std::vector<double> cutoffs(nrooms_needed-1);
  for(unsigned i=0; i<nrooms_needed; i++) {
    cutoffs[i] = citation_list[(i+1)*nslots];
  }

  // Map all the priorities to the range [0, nrooms_needed)
  // with 0 being highest priority
  for(unsigned i=0; i<size(); i++) {
    double citations = h_data_[i].average_citation_count();
    unsigned j;
    for(j=0; j<cutoffs.size(); j++) {
      if(citations >= cutoffs[j]) {
        break;
      }
    }
    h_data_[i].set_priority(j);
  }
}

void Minisymposia::set_priority_penalty_bounds(unsigned nslots) {
  // Get the priorities
  std::vector<unsigned> priority_list(size());
  for(unsigned i=0; i<size(); i++) {
    priority_list[i] = h_data_[i].priority();
  }

  // Sort the priorities
  std::sort(priority_list.begin(), priority_list.end());

  // Compute the best schedule penalty
  unsigned slot=0;
  unsigned room_index=0;
  for(unsigned i=0; i<size(); i++) {
    if(priority_list[i] < room_index) {
      min_priority_penalty_ += pow(room_index - priority_list[i], 2);
    }
    slot++;
    if(slot >= nslots) {
      slot = 0;
      room_index++;
    }
  }

  // Compute the worst schedule penalty
  slot=0;
  room_index=0;
  for(unsigned i=size(); i > 0; i--) {
    if(priority_list[i] < room_index) {
      max_priority_penalty_ += pow(room_index - priority_list[i-1], 2);
    }
    slot++;
    if(slot >= nslots) {
      slot = 0;
      room_index++;
    }
  }

  printf("Priority penalty bounds: %i %i\n", min_priority_penalty_, max_priority_penalty_);
}

const std::string& Minisymposia::get_title(unsigned i) const {
  return h_data_[i].title();
}

const std::string& Minisymposia::get_theme(unsigned i) const {
  auto id = h_data_[i].tid();
  return themes_[id];
}

unsigned Minisymposia::get_priority(unsigned i) const {
  return h_data_[i].priority();
}