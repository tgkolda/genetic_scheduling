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
  Minisymposium(const std::string& title, const std::vector<std::string>& talks);
  Minisymposium(const std::string& title, 
                unsigned tid, 
                const std::string& organizer, 
                const std::vector<std::string>& speakers,
                double average_citation_count,
                unsigned part);

  bool shares_participant(const Minisymposium& m) const;
  bool comes_before(const Minisymposium& m) const;
  bool shares_theme(const Minisymposium& m) const;
  KOKKOS_FUNCTION bool higher_priority(const Minisymposium& m) const;
  KOKKOS_FUNCTION unsigned tid() const;
  KOKKOS_FUNCTION unsigned priority() const;
  const std::string& short_title() const;
  const std::string& full_title() const;
  double average_citation_count() const;
  const std::vector<std::string>& talks() const;

  void set_priority(unsigned priority);
  KOKKOS_FUNCTION unsigned size() const;

private:
  std::string title_with_part_, title_without_part_;
  std::vector<std::string> talks_;
  unsigned tid_;
  double average_citation_count_;
  unsigned room_priority_;
  std::unordered_set<std::string> participants_;
  unsigned part_;
  unsigned size_;
};

#endif /* MINISYMPOSIUM_H */