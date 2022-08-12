#ifndef MINISYMPOSIA_H
#define MINISYMPOSIA_H

#include "Minisymposium.hpp"
#include <ostream>
#include <vector>

class Minisymposia {
public:
  void add(const Minisymposium& mini);

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  std::vector<Minisymposium> data_;
};

#endif /* MINISYMPOSIA_H */