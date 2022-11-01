#ifndef MAPPER_H
#define MAPPER_H

#include "Lectures.hpp"
#include "Minisymposia.hpp"
#include "Kokkos_Random.hpp"
#include <random>

class Mapper {
public:
  Mapper(const Lectures& lectures, const Minisymposia& minisymposia);
  void run_genetic(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);
  void record(const std::string& filename) const;

  void rate_mappings();
  void compute_weights();
  void breed_population(unsigned eliteSize);
  void mutate_population(double mutationRate);
  void smush();
private:
  void make_initial_population(unsigned popSize);
  void sort();

  template<class ViewType>
  KOKKOS_INLINE_FUNCTION
  unsigned count_full_minisymposia(ViewType mapping) const;

  KOKKOS_FUNCTION unsigned get_parent() const;
  KOKKOS_FUNCTION void breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const;

  Kokkos::View<unsigned**> current_mappings_;
  Kokkos::View<unsigned**> next_mappings_;
  Kokkos::View<double*> ratings_;
  Kokkos::View<double*> weights_;
  Kokkos::View<unsigned*> d_best_indices_;
  Kokkos::View<unsigned*, Kokkos::HostSpace> h_best_indices_;
  Lectures lectures_;
  Minisymposia minisymposia_;
  std::default_random_engine rng_;
  Kokkos::Random_XorShift64_Pool<> pool_;
};

template<class ViewType>
KOKKOS_INLINE_FUNCTION
unsigned Mapper::count_full_minisymposia(ViewType mapping) const {
  unsigned nmini = minisymposia_.size();
  unsigned nlectures = lectures_.size();
  unsigned ngenes = mapping.extent(0);
  unsigned score = 0;
  for(unsigned i=0; i<nmini; i++) {
    if(mapping(i) < nlectures) {
      score += 25;
    }
    else {
      score += 16;
    }
  }
  for(unsigned i=nmini; i+4<ngenes; i+=5) {
    unsigned nlectures_in_mini = 0;
    for(unsigned j=0; j<5; j++) {
      if(mapping(i+j) < nlectures) {
        nlectures_in_mini++;
      }
    }
    score += pow(nlectures_in_mini, 2);
  }

  return score;
}

#endif /* MAPPER_H */