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

std::ostream& operator<<(std::ostream& os, const Minisymposium& mini) {
  os << mini.title_;
  return os;
}