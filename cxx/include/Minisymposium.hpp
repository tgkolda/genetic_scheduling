#ifndef MINISYMPOSIUM_H
#define MINISYMPOSIUM_H

#include "Speaker.hpp"
#include <Kokkos_Core.hpp>
#include <ostream>
#include <string>
#include <unordered_set>
#include <vector>

class Minisymposium {
public:
  Minisymposium() = default;
  Minisymposium(unsigned id,
                const std::string& title,
                const std::vector<std::string>& talks, 
                const std::vector<Speaker>& organizers, 
                const std::vector<Speaker>& speakers,
                const std::string& room);
  Minisymposium(const Minisymposium&) = default;
  ~Minisymposium() = default;
  Minisymposium& operator=(const Minisymposium&) = default;

  bool shares_participant(const Minisymposium& m) const;
  bool comes_before(const Minisymposium& m) const;
  KOKKOS_FUNCTION unsigned priority() const;
  const std::string& short_title() const;
  const std::string& full_title() const;
  const std::string& room() const;
  unsigned id() const;
  KOKKOS_FUNCTION unsigned room_id() const;
  double total_citation_count() const;
  const std::vector<std::string>& talks() const;

  void set_priority(unsigned priority);
  void set_room_id(unsigned id);
  KOKKOS_FUNCTION unsigned size() const;

private:
  std::string title_with_part_, title_without_part_, room_;
  std::vector<std::string> talks_;
  unsigned id_;
  double total_citation_count_;
  unsigned room_priority_;
  std::vector<Speaker> participants_;
  unsigned part_;
  unsigned size_;
  static std::unordered_map<std::string, unsigned> roman_numeral_map_;
  unsigned room_id_;
};

#endif /* MINISYMPOSIUM_H */