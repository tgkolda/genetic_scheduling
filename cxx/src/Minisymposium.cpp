#include "Minisymposium.hpp"

Minisymposium::Minisymposium(const std::string& title, 
                             const std::string& theme, 
                             const std::string& organizer, 
                             const std::vector<std::string>& speakers,
                             unsigned part) :
  title_(title),
  theme_(theme),
  part_(part)
{
  if(!organizer.empty())
    participants_.insert(organizer);
  for(const auto& speaker : speakers)
    participants_.insert(speaker);
}

std::ostream& operator<<(std::ostream& os, const Minisymposium& mini) {
  os << mini.title_;
  return os;
}