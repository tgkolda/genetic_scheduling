#include "Minisymposium.hpp"
#include <algorithm>

Minisymposium::Minisymposium(const std::string& title, 
                             unsigned tid, 
                             const std::string& organizer, 
                             const std::vector<std::string>& speakers,
                             unsigned part) :
  title_(title),
  tid_(tid),
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

unsigned Minisymposium::tid() const {
  return tid_;
}

std::ostream& operator<<(std::ostream& os, const Minisymposium& mini) {
  os << mini.title_;
  return os;
}