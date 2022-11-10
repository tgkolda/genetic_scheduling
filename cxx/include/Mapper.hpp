#ifndef MAPPER_H
#define MAPPER_H

#include "Lectures.hpp"
#include "Minisymposia.hpp"
#include "Kokkos_Random.hpp"
#include <fstream>
#include <random>

class Mapper {
public:
  typedef Kokkos::View<unsigned**> ViewType;

  Mapper(const Lectures& lectures, const Minisymposia& minisymposia, unsigned nExtraMini=5);
  ViewType make_initial_population(unsigned popSize);

  template<class View1D>
  KOKKOS_INLINE_FUNCTION double rate(View1D mapping) const;

  KOKKOS_FUNCTION bool out_of_bounds(unsigned i) const;

  template<class View1D>
  inline void greedy(View1D solution) const;

  template<class View1D>
  inline void record(const std::string& filename, View1D mapping) const;
  void smush();
private:
  void sort();

  template<class View1D>
  KOKKOS_INLINE_FUNCTION
  unsigned count_full_minisymposia(View1D mapping) const;

  template<class View1D>
  KOKKOS_INLINE_FUNCTION
  double topic_cohesion_score(View1D mapping) const;

  Lectures lectures_;
  Minisymposia minisymposia_;
  unsigned nExtraMini_;
};

template<class View1D>
double Mapper::rate(View1D mapping) const {
  constexpr double fullness_weight = 1.0;
  constexpr double cohesion_weight = 5.0;

  unsigned nfull = count_full_minisymposia(mapping);
  double cohesion = topic_cohesion_score(mapping);
  return fullness_weight*nfull + cohesion_weight*cohesion;
}

template<class View1D>
KOKKOS_INLINE_FUNCTION
unsigned Mapper::count_full_minisymposia(View1D mapping) const {
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

template<class View1D>
KOKKOS_INLINE_FUNCTION
double Mapper::topic_cohesion_score(View1D mapping) const {
  unsigned nmini = minisymposia_.size();
  unsigned nlectures = lectures_.size();
  unsigned ngenes = mapping.extent(0);
  double score = 0;

  for(unsigned i=0; i<nmini; i++) {
    if(mapping(i) < nlectures) {
      score += lectures_.topic_cohesion_score(minisymposia_, i, mapping(i));
    }
  }
  for(unsigned i=nmini; i+4<ngenes; i+=5) {
    for(unsigned j=0; j<5; j++) {
      if(mapping(i+j) >= nlectures) continue;
      for(unsigned k=j+1; k<5; k++) {
        if(mapping(i+k) < nlectures) {
          // 12 is (5-1)!
          score += lectures_.topic_cohesion_score(mapping(i+j), mapping(i+k)) / 12.0;
        }
      }
    }
  }
  return score;
}

template<class View1D>
void Mapper::record(const std::string& filename, View1D mapping) const {
  unsigned nmini = minisymposia_.size();
  unsigned nlectures = lectures_.size();
  unsigned ngenes = mapping.extent(0);

  std::ofstream fout(filename);
  fout << "# Minisymposia\n\n"
       << "|Minisymposium|Lecture 1|Lecture 2|Lecture 3|Lecture 4|Lecture 5|\n"
       << "|---|---|---|---|---|---|\n";

  auto mini_codes = minisymposia_.class_codes();
  auto lect_codes = lectures_.class_codes();
  for(unsigned m=0; m<nmini; m++) {
    fout << "|" << minisymposia_.get(m).full_title() << " " << mini_codes(m,0)
         << " " << mini_codes(m,1) << " " << mini_codes(m,2);
    auto talks = minisymposia_.get(m).talks();

    unsigned i;
    for(i=0; i<talks.size(); i++) {
      fout << "|" << talks[i];
    }
    unsigned lid = mapping(m);
    if(lid < nlectures) {
      fout << "|" << lectures_.title(lid) << " " << lect_codes(lid,0)
           << " " << lect_codes(lid,1) << " " << lect_codes(lid,2);
      i++;
    }
    for(;i<5; i++) {
      fout << "| ";
    }
    fout << "|\n";
  }
  for(unsigned m=nmini, i=0; m<ngenes; m+=5, i++) {
    fout << "|Contributed Lectures " << i+1;
    for(unsigned j=0; j<5; j++) {
      unsigned lid = mapping(m+j);
      if(lid < nlectures) {
        fout << "|" << lectures_.title(lid) << " " << lect_codes(lid,0)
             << " " << lect_codes(lid,1) << " " << lect_codes(lid,2);
      }
      else {
        fout << "| ";
      }
    }
    fout << "|\n";
  }
}

template<class View1D>
void Mapper::greedy(View1D solution) const {
  unsigned nlectures = lectures_.size();
  unsigned nmini = minisymposia_.size();
  unsigned ngenes = solution.extent(0);
  std::vector<unsigned> unused_indices(nlectures);
  std::iota(unused_indices.begin(), unused_indices.end(), 0);

  for(unsigned i=0; i<ngenes; i++) {
    solution[i] = ngenes;
  }

  auto lecture_codes = lectures_.class_codes();
  auto mini_codes = minisymposia_.class_codes();

  auto ntopics_in_common = [](unsigned lid, unsigned mid, 
                              Kokkos::View<Theme*[3]>::HostMirror lecture_codes,
                              Kokkos::View<Theme*[3]>::HostMirror mini_codes) {
    unsigned result = 0;
    for(unsigned i=0; i<3; i++) {
      for(unsigned j=0; j<3; j++) {
        if(lecture_codes(lid, i) == mini_codes(mid, j)) {
          result++;
        }
      }
    }
    return result;
  };

  // For each minisymposium, try to find a lecture that matches all themes
  for(unsigned ndesired=3; ndesired > 0; ndesired--) {
    for(unsigned i=0; i<nmini; i++) {
      if(solution[i] < nlectures) continue;
      for(unsigned j=0; j<unused_indices.size(); j++) {
        unsigned lid = unused_indices[j];
        if(ntopics_in_common(lid, i, lecture_codes, mini_codes) == ndesired) {
          solution[i] = lid;
          unused_indices.erase(unused_indices.begin()+j);
          break;
        }
      }
    }
  }

  // Put the remaining lectures in "contributed lectures"
  for(unsigned i=0; i<unused_indices.size(); i++) {
    solution[nmini+i] = unused_indices[i];
  }

  // Fill in the rest with empty space
  for(unsigned i=0, empty_val=nlectures; i<ngenes; i++) {
    if(solution[i] == ngenes) {
      solution[i] = empty_val;
      empty_val++;
    }
  }

  for(unsigned i=0; i<ngenes; i++) {
      printf("%i: %i\n", i, solution[i]);
  }
}

#endif /* MAPPER_H */