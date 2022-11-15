#include "Minisymposium.hpp"
#include <algorithm>

Minisymposium::Minisymposium(const std::string& title, 
                             const std::vector<std::string>& talks,
                             const std::vector<Speaker>& organizers, 
                             const std::vector<Speaker>& speakers) :
  title_with_part_(title),
  talks_(talks),
  size_(speakers.size())
{
  // Strip the part from the name
  size_t found = title.find(" - Part");
  if(found != std::string::npos) {
    title_without_part_ = title.substr(0, found);
  }
  else {
    title_without_part_ = title;
  }

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