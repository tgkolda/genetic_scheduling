#ifndef MINISYMPOSIUM_H
#define MINISYMPOSIUM_H

#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

class Minisymposium {
public:
  Minisymposium(const std::string& title, 
                unsigned tid, 
                const std::string& organizer, 
                const std::vector<std::string>& speakers,
                unsigned part);

  bool shares_participant(const Minisymposium& m) const;
  bool comes_before(const Minisymposium& m) const;
  bool shares_theme(const Minisymposium& m) const;
  unsigned tid() const;
  const std::string& title() const;

  friend std::ostream& operator<<(std::ostream& os, const Minisymposium& mini);
  
private:
  std::string title_;
  unsigned tid_;
  std::unordered_set<std::string> participants_;
  unsigned part_;
};

#endif /* MINISYMPOSIUM_H */