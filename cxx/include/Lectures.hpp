#ifndef LECTURES_H
#define LECTURES_H

#include <Kokkos_Core.hpp>

class Lectures {
public:
  Lectures(const std::string& filename);
  KOKKOS_FUNCTION unsigned size() const;
private:
  std::vector<std::string> titles_;
  std::vector<std::string> speakers_;
  Kokkos::View<unsigned*[3]> class_codes_;
};

#endif /* LECTURES_H */