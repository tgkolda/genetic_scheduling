#ifndef MINISYMPOSIA_H
#define MINISYMPOSIA_H

#include "Minisymposium.hpp"
#include <Kokkos_Core.hpp>
#include <ostream>
#include <set>
#include <vector>

class Minisymposia {
public:
  Minisymposia(const std::string& filename);
  Minisymposia(const std::string& filename, unsigned nrooms, unsigned nslots);
  Minisymposia(const Minisymposia&) = default;
  ~Minisymposia() = default;
  Minisymposia& operator=(const Minisymposia&) = delete;
  

  KOKKOS_FUNCTION unsigned size() const;
  KOKKOS_FUNCTION const Minisymposium& operator[](unsigned i) const;
  const Minisymposium& get(unsigned i) const;

  KOKKOS_FUNCTION bool overlaps_participants(unsigned m1, unsigned m2) const;
  KOKKOS_FUNCTION bool breaks_ordering(unsigned m1, unsigned m2) const;
  unsigned get_max_penalty() const;
  const std::vector<std::string>& themes() const;
  void set_overlapping_participants();
  void set_prerequisites();
  KOKKOS_FUNCTION double map_theme_penalty(unsigned nproblems) const;
  KOKKOS_FUNCTION double map_priority_penalty(unsigned nproblems) const;
  void set_overlapping_themes(unsigned nrooms, unsigned nslots);
  void set_priorities(unsigned nslots);
  void set_priority_penalty_bounds(unsigned nslots);
  const std::string& get_theme(unsigned i) const;
  KOKKOS_FUNCTION unsigned class_codes(unsigned mid, unsigned cid) const;
  Kokkos::View<unsigned*[3]>::HostMirror class_codes() const;

  template<class ViewType1, class ViewType2>
  KOKKOS_INLINE_FUNCTION double rate_schedule(ViewType1 schedule, ViewType2 theme_penalties,
    unsigned& order_penalty, unsigned& oversubscribed_penalty, unsigned& theme_penalty,
    unsigned& priority_penalty) const;
  
  template<class ViewType>
  inline std::string rate_schedule(ViewType schedule) const;

  friend std::ostream& operator<<(std::ostream& os, const Minisymposia& mini);
private:
  std::vector<std::string> themes_;
  Kokkos::View<unsigned*[3]> class_codes_;
  Kokkos::View<Minisymposium*> d_data_;
  Kokkos::View<Minisymposium*>::HostMirror h_data_;
  Kokkos::View<bool**> same_participants_;
  Kokkos::View<bool**> is_prereq_;
  Kokkos::View<bool**> same_themes_;
  unsigned nthemes_;
  unsigned max_penalty_{1};
  unsigned min_theme_penalty_{0};
  unsigned max_theme_penalty_{0};
  unsigned min_priority_penalty_{0};
  unsigned max_priority_penalty_{0};
};

template<class ViewType1, class ViewType2>
KOKKOS_INLINE_FUNCTION 
double Minisymposia::rate_schedule(ViewType1 schedule, ViewType2 theme_penalties, 
  unsigned& order_penalty, unsigned& oversubscribed_penalty, unsigned& theme_penalty, 
  unsigned& priority_penalty) const
{
  unsigned nrooms = schedule.extent(1);
  unsigned nslots = schedule.extent(0);
  unsigned nmini = size();
  // Compute the penalty related to multi-part minisymposia being out of order
  order_penalty = 0;
  for(unsigned sl1=0; sl1<nslots; sl1++) {
    for(unsigned r1=0; r1<nrooms; r1++) {
      if(schedule(sl1,r1) >= nmini) continue;
      for(unsigned sl2=sl1; sl2<nslots; sl2++) {
        for(unsigned r2=0; r2<nrooms; r2++) {
          if(schedule(sl2,r2) >= nmini) continue;
          if(breaks_ordering(schedule(sl1,r1), schedule(sl2,r2))) {
            //printf("Entries %i and %i break the ordering\n", schedule(sl1, r1), schedule(sl2, r2));
            order_penalty++;
          }
        }
      }
    }
  }
  // Compute the penalty related to oversubscribed participants
  oversubscribed_penalty = 0;
  for(unsigned sl=0; sl<nslots; sl++) {
    for(unsigned r1=0; r1<nrooms; r1++) {
      if(schedule(sl,r1) >= nmini) continue;
      for(unsigned r2=r1+1; r2<nrooms; r2++) {
        if(schedule(sl,r2) >= nmini) continue;
        if(overlaps_participants(schedule(sl,r1), schedule(sl,r2))) {
          oversubscribed_penalty++;
          //printf("Entries %i and %i oversubscribe a participant\n", schedule(sl, r1), schedule(sl, r2));
        }
      }
    }
  }
  // Compute the penalty related to theme overlap
  theme_penalty = 0;
  for(unsigned sl=0; sl<nslots; sl++) {
    for(unsigned tid=0; tid<nthemes_; tid++) {
      theme_penalties[tid] = 0;
    }
    for(unsigned r=0; r<nrooms; r++) {
      unsigned mini_index = schedule(sl,r);
      if(mini_index >= nmini) continue;
      unsigned tid = d_data_[mini_index].tid();
      theme_penalties[tid]++;
    }
    for(unsigned tid=0; tid<nthemes_; tid++) {
      auto p = theme_penalties[tid];
      if(p > 1) {
        theme_penalty += pow(p-1, 2);
      }
    }
  }
  // Compute the penalty related to priority
  priority_penalty = 0;
  for(unsigned sl=0; sl<nslots; sl++) {
    for(unsigned r=0; r<nrooms; r++) {
      unsigned mini_index = schedule(sl,r);
      if(mini_index >= nmini) continue;
      unsigned priority = d_data_[mini_index].priority();
      if(priority < r) {
        priority_penalty += pow(r-priority, 2);
      }
    }
  }
  double penalty = order_penalty + oversubscribed_penalty;
  penalty += map_theme_penalty(theme_penalty) / 2.0;
  penalty += map_priority_penalty(priority_penalty) / 2.0;
  return 1 - penalty / max_penalty_;
}

template<class ViewType>
inline std::string Minisymposia::rate_schedule(ViewType schedule) const {
  Kokkos::View<unsigned*> theme_penalties("theme penalties", nthemes_);
  Kokkos::View<unsigned*> d_penalties("penalties", 4);
  double score;
  Kokkos::parallel_reduce("computing score", 1, KOKKOS_CLASS_LAMBDA (unsigned i, double& lscore) {
    lscore = rate_schedule(schedule, theme_penalties, d_penalties(0), d_penalties(1), 
      d_penalties(2), d_penalties(3));
  }, score);

  // Copy penalties back to device
  auto h_penalties = Kokkos::create_mirror_view(d_penalties);
  Kokkos::deep_copy(h_penalties, d_penalties);

  std::ostringstream oss;

  oss << "This schedule has score " << score << "\nThe order penalty is " << h_penalties(0)
      << "\nThe oversubscribed penalty is " << h_penalties(1) << "\nThe theme penalty is "
      << h_penalties(2) << " in [ " << min_theme_penalty_ << ", " << max_theme_penalty_
      << "]\nThe priority penalty is " << h_penalties(3) << " in [ " << min_priority_penalty_
      << ", " << max_priority_penalty_ << "]";
  return oss.str();
}

#endif /* MINISYMPOSIA_H */