#include "yaml-cpp/yaml.h"
#include "Minisymposia.hpp"

Minisymposia::Minisymposia(const std::string& filename) {
  // Read the minisymposia from yaml on the host
  YAML::Node nodes = YAML::LoadFile(filename);

  unsigned n = nodes.size();
  class_codes_ = Kokkos::View<Theme*[3]>("classification codes", n);
  auto h_codes = Kokkos::create_mirror_view(class_codes_);
  d_data_ = Kokkos::View<Minisymposium*>("minisymposia", n);
  h_data_ = Kokkos::create_mirror_view(d_data_);

  unsigned i=0;
  for(auto node : nodes) {
    std::string title = node.first.as<std::string>();
    std::vector<unsigned> codes = node.second["class codes"].as<std::vector<unsigned>>();
    std::vector<std::string> talks = node.second["talks"].as<std::vector<std::string>>();
    std::vector<std::string> organizer_names;
    if(node.second["organizers"])
      organizer_names = node.second["organizers"].as<std::vector<std::string>>();
    std::vector<std::string> speaker_names;
    if(node.second["speakers"])
      speaker_names = node.second["speakers"].as<std::vector<std::string>>();
    assert(codes.size() == 3);
    for(unsigned j=0; j<3; j++) {
      h_codes(i,j) = codes[j];
    }

    std::vector<Speaker> organizers(organizer_names.size());
    for(unsigned j=0; j<organizer_names.size(); j++) {
      organizers[j] = Speaker(organizer_names[j]);
    }
    std::vector<Speaker> speakers(speaker_names.size());
    for(unsigned j=0; j<speaker_names.size(); j++) {
      speakers[j] = Speaker(speaker_names[j]);
    }

    h_data_[i] = Minisymposium(title, talks, organizers, speakers);
    std::cout << "Minisymposium " << i << " is " << title << " and its themes are "
              << h_codes(i,0) << ", " << h_codes(i,1) << ", " << h_codes(i,2) << " and its speakers are";
    for(unsigned j=0; j<speaker_names.size(); j++) {
      std::cout << " " << speaker_names[j];
    }
    std::cout << "\n";

    i++;
  }

  // Copy the data to device
  Kokkos::deep_copy(d_data_, h_data_);
  Kokkos::deep_copy(class_codes_, h_codes);
}

Minisymposia::Minisymposia(const std::string& filename, unsigned nrooms, unsigned nslots) :
  Minisymposia(filename)
{
  set_overlapping_participants();
  set_prerequisites();
  set_priorities(nslots);
  set_priority_penalty_bounds(nslots);
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
  printf("set_overlapping_participants max_penalty: %i\n", max_penalty_);
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
  printf("set_prerequisites max_penalty: %i\n", max_penalty_);
}

KOKKOS_FUNCTION
double Minisymposia::map_theme_penalty(unsigned nproblems) const {
  return (nproblems - min_theme_penalty_) / double(max_theme_penalty_ - min_theme_penalty_);
}

KOKKOS_FUNCTION
double Minisymposia::map_priority_penalty(unsigned nproblems) const {
  return (nproblems - min_priority_penalty_) / double(max_priority_penalty_ - min_priority_penalty_);
}

void Minisymposia::set_priorities(unsigned nslots) {
  // Get the citations
  std::vector<double> citation_list(size());
  for(unsigned i=0; i<size(); i++) {
    citation_list[i] = h_data_[i].total_citation_count();
  }

  // Sort the citations from most to least popular
  std::sort(citation_list.begin(), citation_list.end(), std::greater<double>());

  // Get the cutoffs
  unsigned nrooms_needed = ceil(double(size())/nslots);
  std::vector<double> cutoffs(nrooms_needed-1);
  for(unsigned i=0; i<nrooms_needed-1; i++) {
    cutoffs[i] = citation_list[(i+1)*nslots];
  }

  // Map all the priorities to the range [0, nrooms_needed)
  // with 0 being highest priority
  for(unsigned i=0; i<size(); i++) {
    double citations = h_data_[i].total_citation_count();
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
    if(priority_list[i-1] < room_index) {
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

const Theme& Minisymposia::class_codes(unsigned mid, unsigned cid) const {
  return class_codes_(mid, cid);
}

Kokkos::View<Theme*[3]>::HostMirror Minisymposia::class_codes() const {
  auto h_class_codes = Kokkos::create_mirror_view(class_codes_);
  Kokkos::deep_copy(h_class_codes, class_codes_);
  return h_class_codes;
}