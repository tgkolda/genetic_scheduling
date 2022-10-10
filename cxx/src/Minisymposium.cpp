#include "Minisymposium.hpp"
#include <algorithm>

Minisymposium::Minisymposium(const std::string& title, 
                             unsigned tid, 
                             const std::string& organizer, 
                             const std::vector<std::string>& speakers,
                             double average_citation_count,
                             unsigned part) :
  title_(title),
  tid_(tid),
  average_citation_count_(average_citation_count),
  part_(part)

{
  // Strip the part from the name
  size_t found = title.find(" - Part");
  if(found != std::string::npos) {
    title_ = title.substr(0, found);
  }

  // Add the participants
  if(!organizer.empty())
    participants_.insert(organizer);
  for(const auto& speaker : speakers)
    participants_.insert(speaker);
}

bool Minisymposium::shares_participant(const Minisymposium& m) const {
  std::vector<std::string> intersection(participants_.size());
  auto it = std::set_intersection(participants_.begin(), participants_.end(),
                                  m.participants_.begin(), m.participants_.end(),
                                  intersection.begin());
  return it != intersection.begin();
}

bool Minisymposium::comes_before(const Minisymposium& m) const {
  return title_ == m.title_ && part_ < m.part_;
}

bool Minisymposium::shares_theme(const Minisymposium& m) const {
  return tid_ == m.tid_;
}

KOKKOS_FUNCTION
bool Minisymposium::higher_priority(const Minisymposium& m) const {
  return room_priority_ < m.room_priority_;
}

KOKKOS_FUNCTION
unsigned Minisymposium::tid() const {
  return tid_;
}

KOKKOS_FUNCTION
unsigned Minisymposium::priority() const {
  return room_priority_;
}

const std::string& Minisymposium::title() const {
  return title_;
}

double Minisymposium::average_citation_count() const {
  return average_citation_count_;
}

void Minisymposium::set_priority(unsigned priority) {
  room_priority_ = priority;
}