#ifndef MINISYMPOSIUM_H
#define MINISYMPOSIUM_H

#include <Kokkos_Core.hpp>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

class Minisymposium {
public:
  Minisymposium() = default;
  Minisymposium(const std::string& title, 
                unsigned tid, 
                const std::string& organizer, 
                const std::vector<std::string>& speakers,
                unsigned part);

  bool shares_participant(const Minisymposium& m) const;
  bool comes_before(const Minisymposium& m) const;
  bool shares_theme(const Minisymposium& m) const;
  KOKKOS_FUNCTION unsigned tid() const;
  const std::string& title() const;

private:
  std::string title_;
  unsigned tid_;
  std::unordered_set<std::string> participants_;
  unsigned part_;
};

#endif /* MINISYMPOSIUM_H */