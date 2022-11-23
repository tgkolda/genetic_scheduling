#include "Minisymposium.hpp"
#include <algorithm>

std::unordered_map<std::string, unsigned> Minisymposium::roman_numeral_map_ = {
  {"I", 1},
  {"II", 2},
  {"III", 3},
  {"IV", 4},
  {"V", 5},
  {"VI", 6},
  {"VII", 7},
  {"VIII", 8},
  {"IX", 9},
  {"X", 10}
};

Minisymposium::Minisymposium(unsigned id,
                             const std::string& title, 
                             const std::vector<std::string>& talks,
                             const std::vector<Speaker>& organizers, 
                             const std::vector<Speaker>& speakers) :
  id_(id),
  title_with_part_(title),
  talks_(talks),
  size_(speakers.size())
{
  // Strip the part from the name
  size_t found = title.find(" - Part ");
  if(found != std::string::npos) {
    title_without_part_ = title.substr(0, found);
    size_t end_pos = title.find(" of ", found);
    size_t start_pos = found+8;
    size_t nchars = end_pos-start_pos;
    std::string part = title.substr(start_pos, nchars);
    part_ = roman_numeral_map_.at(part);
  }
  else {
    title_without_part_ = title;
  }

  // Compute the total citation count
  total_citation_count_ = 0;
  for(const auto& speaker: speakers) {
    total_citation_count_ += speaker.citations();
  }

  printf("%s: %lf\n", title.c_str(), total_citation_count_);

  // Add the participants
  for(const auto& speaker : speakers)
    participants_.push_back(speaker);
  for(const auto& organizer : organizers) {
    participants_.push_back(organizer);
  }

  // This needs to be sorted for set_intersection
  std::sort(participants_.begin(), participants_.end());

  // Remove duplicates
  auto it = std::unique (participants_.begin(), participants_.end());
  participants_.resize( std::distance(participants_.begin(),it) );
}

bool Minisymposium::shares_participant(const Minisymposium& m) const {
  std::vector<Speaker> intersection(participants_.size());
  auto it = std::set_intersection(participants_.begin(), participants_.end(),
                                  m.participants_.begin(), m.participants_.end(),
                                  intersection.begin());
  for(auto i=intersection.begin(); i != it; i++) {
    if(i->name() != "Presenters to be Announced") {
      return true;
    }
  }
  return false;
}

bool Minisymposium::comes_before(const Minisymposium& m) const {
  return title_without_part_ == m.title_without_part_ && part_ < m.part_;
}

KOKKOS_FUNCTION
unsigned Minisymposium::priority() const {
  return room_priority_;
}

const std::string& Minisymposium::short_title() const {
  return title_without_part_;
}

const std::string& Minisymposium::full_title() const {
  return title_with_part_;
}

unsigned Minisymposium::id() const {
  return id_;
}

double Minisymposium::total_citation_count() const {
  return total_citation_count_;
}

void Minisymposium::set_priority(unsigned priority) {
  room_priority_ = priority;
}

const std::vector<std::string>& Minisymposium::talks() const {
  return talks_;
}

unsigned Minisymposium::size() const {
  return size_;
}