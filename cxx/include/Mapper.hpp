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
  KOKKOS_INLINE_FUNCTION double rate(View1D mapping, bool verbose=false) const;

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
  const unsigned nlect_per_mini_{5};
};

template<class View1D>
double Mapper::rate(View1D mapping, bool verbose) const {
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

  unsigned subs = 0;
  for(unsigned i=0; i<nmini; i++) {
    unsigned nlect_in_mini = minisymposia_[i].size();
    for(unsigned j=nlect_in_mini; j<nlect_per_mini_; j++) {
      if(mapping(subs) < nlectures) {
        nlect_in_mini++;
      }
      subs++;
    }
    score += pow(nlect_in_mini, 2);
  }

  for(; subs+nlect_per_mini_-1<ngenes; subs+=nlect_per_mini_) {
    unsigned nlectures_in_mini = 0;
    for(unsigned j=0; j<nlect_per_mini_; j++) {
      if(mapping(subs+j) < nlectures) {
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

  unsigned subs = 0;
  for(unsigned i=0; i<nmini; i++) {
    unsigned nlect_in_mini = minisymposia_[i].size();
    for(unsigned j=nlect_in_mini; j<nlect_per_mini_; j++) {
      if(mapping(subs) < nlectures) {
        score += lectures_.topic_cohesion_score(minisymposia_, i, mapping(subs));
      }
      subs++;
    }
  }

  for(; subs+nlect_per_mini_-1<ngenes; subs+=nlect_per_mini_) {
    for(unsigned j=0; j<nlect_per_mini_; j++) {
      for(unsigned k=j+1; k<nlect_per_mini_; k++) {
        if(mapping(subs+j) < nlectures) {
          score += lectures_.topic_cohesion_score(mapping(subs+j), mapping(subs+k));
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
  fout << "# Minisymposia\n\n|Minisymposium|";

  for(unsigned i=0; i<nlect_per_mini_; i++) {
    fout << "Lecture " << i+1 << "|";
  }
  fout << "\n|---|";
  for(unsigned i=0; i<nlect_per_mini_; i++) {
    fout << "---|";
  }
  fout << "\n";

  auto mini_codes = minisymposia_.class_codes();
  auto lect_codes = lectures_.class_codes();
  unsigned subs = 0;
  for(unsigned i=0; i<nmini; i++) {
    unsigned nlect_in_mini = minisymposia_.get(i).size();
    fout << "|" << minisymposia_.get(i).id() << " " << minisymposia_.get(i).full_title() 
         << " " << mini_codes(i,0) << " " << mini_codes(i,1) << " " << mini_codes(i,2);
    auto talks = minisymposia_.get(i).talks();
    for(unsigned j=0; j<talks.size(); j++) {
      fout << "|" << talks[j];
    }
    for(unsigned j=talks.size(); j<nlect_per_mini_; j++) {
      unsigned lid = mapping(subs);
      if(lid < nlectures) {
        fout << "|" << lectures_.id(lid) << " " << lectures_.title(lid) << " " << lect_codes(lid,0)
             << " " << lect_codes(lid,1) << " " << lect_codes(lid,2);
      }
      else {
        fout << "| ";
      }
      subs++;
    }
    fout << "|\n";
  }

  for(unsigned i=0; subs+nlect_per_mini_-1<ngenes; subs+=nlect_per_mini_, i++) {
    fout << "|Contributed Lectures " << i+1;
    for(unsigned j=0; j<nlect_per_mini_; j++) {
      unsigned lid = mapping(subs+j);
      if(lid < nlectures) {
        fout << "|" << lectures_.id(lid) << " " << lectures_.title(lid) << " " << lect_codes(lid,0)
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

  // Try to match everything, then take what you can get
  while(!unused_indices.empty()) {
    double best_score = 0;
    unsigned best_index = 0;
    unsigned best_index2 = 0;
    unsigned best_subs = 0;
    unsigned subs = 0;
    // For each minisymposium, try to find a lecture that matches the themes
    for(unsigned i=0; i<nmini; i++) {
      unsigned nlect_in_mini = minisymposia_.get(i).size();
      for(unsigned j=nlect_in_mini; j<nlect_per_mini_; j++, subs++) {
        if(solution[subs] >= nlectures) {
          for(unsigned k=0; k<unused_indices.size(); k++) {
            unsigned lid = unused_indices[k];
            double score = compute_topic_score(lid, i, lecture_codes, mini_codes);
            score += 1 - nlect_in_mini / nlect_per_mini_;
            if(score > best_score) {
              best_score = score;
              best_index = k;
              best_index2 = k;
              best_subs = subs;
            }
          }
        }
        else {
          nlect_in_mini++;
        }
      }
    }

    // For the contributed lecture presentations, try to find a lecture that matches the themes
    for(; subs+nlect_per_mini_-1<ngenes; subs+=nlect_per_mini_) {
      // If this CP is empty, look for two lectures with the same theme
      if(solution[subs] >= nlectures) {
        bool found = false;
        for(unsigned i=0; !found && i<unused_indices.size(); i++) {
          unsigned lid1 = unused_indices[i];
          for(unsigned j=i+1; j<unused_indices.size(); j++) {
            unsigned lid2 = unused_indices[j];
            double score = compute_topic_score(lid1, lid2, lecture_codes);
            if(score > best_score) {
              best_score = score;
              best_index = i;
              best_index2 = j;
              best_subs = subs;
            }
          }
        }
      }
      // If this CP is not empty, look for another lecture with the same theme
      else {
        unsigned nlect_in_mini = 2;
        for(unsigned i=2; i<nlect_per_mini_; i++) {
          if(solution[subs+i] < nlectures) {
            nlect_in_mini++;
            continue;
          }
          for(unsigned j=0; j < unused_indices.size(); j++) {
            unsigned lid = unused_indices[j];
            double score = 0;
            for(unsigned k=0; k<i; k++) {
              score += compute_topic_score(solution[subs+k], lid, lecture_codes);
            }
            score /= i;
            score += 1 - nlect_in_mini / nlect_per_mini_;
            if(score > best_score) {
              best_score = score;
              best_index = j;
              best_index2 = j;
              best_subs = subs+i;
            }
          }
        }
      }
    }

    // Assign the best option
    if(best_index == best_index2) {
      solution[best_subs] = unused_indices[best_index];
      unused_indices.erase(unused_indices.begin()+best_index);
    }
    else {
      solution[best_subs] = unused_indices[best_index];
      solution[best_subs+1] = unused_indices[best_index2];
      unused_indices.erase(unused_indices.begin()+best_index2);
      unused_indices.erase(unused_indices.begin()+best_index);
    }
  }

  // Fill in the rest
  for(unsigned i=0, empty_val=nlectures; i < ngenes; i++) {
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