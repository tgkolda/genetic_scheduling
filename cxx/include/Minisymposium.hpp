#ifndef MINISYMPOSIUM_H
#define MINISYMPOSIUM_H

#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

class Minisymposium {
public:
  Minisymposium(const std::string& title, 
                const std::string& theme, 
                const std::string& organizer, 
                const std::vector<std::string>& speakers,
                unsigned part);

  friend std::ostream& operator<<(std::ostream& os, const Minisymposium& mini);
  
private:
  std::string title_;
  std::string theme_;
  std::unordered_set<std::string> participants_;
  unsigned part_;
};

#endif /* MINISYMPOSIUM_H */