#ifndef THEME_H
#define THEME_H

#include "Kokkos_Core.hpp"
#include <string>
#include <unordered_map>

enum Similarity {
  IDENTICAL = 2,
  SIMILAR = 1,
  DIFFERENT = 0
};

class Theme {
public:
  Theme& operator=(unsigned id);

  KOKKOS_FUNCTION bool operator==(const Theme& theme) const;

  const std::string& name() const;

  unsigned stem() const;

  Similarity compare(const Theme& theme) const;

  static void read(const std::string& filename);

private:
  unsigned id_;
  static std::unordered_map<unsigned,std::string> theme_map_;
};

std::ostream& operator<<(std::ostream& os, const Theme& theme);

#endif /* THEME_H */