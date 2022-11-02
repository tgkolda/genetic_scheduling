#include "Mapper.hpp"
#include "Utility.hpp"
#include <fstream>

Mapper::Mapper(const Lectures& lectures, const Minisymposia& minisymposia) :
  lectures_(lectures), minisymposia_(minisymposia), pool_(5374857) { }

void Mapper::run_genetic(unsigned popSize, unsigned eliteSize, 
                         double mutationRate, unsigned generations) 
{
  make_initial_population(popSize);

  for(unsigned g=0; g<generations; g++) {
    std::cout << "generation " << g << ":\n";

    smush();
    rate_mappings();
    compute_weights();
    breed_population(eliteSize);
    mutate_population(mutationRate);
    std::swap(current_mappings_, next_mappings_);
  }
}

void Mapper::record(const std::string& filename) const {
  // Find the best mapping
  unsigned popSize = current_mappings_.extent(0);
  unsigned ngenes = current_mappings_.extent(1);
  unsigned nlectures = lectures_.size();
  typedef Kokkos::MaxLoc<double,unsigned>::value_type maxloc_type;
  maxloc_type maxloc;
  Kokkos::parallel_reduce( "Finding best mapping", popSize, KOKKOS_CLASS_LAMBDA (unsigned i, maxloc_type& lmaxloc) {
    if(ratings_[i] > lmaxloc.val) { 
      lmaxloc.val = ratings_[i]; 
      lmaxloc.loc = i; 
    }
  }, Kokkos::MaxLoc<double,unsigned>(maxloc));

  unsigned nmini = minisymposia_.size();
  unsigned ind = maxloc.loc;
  auto h_current_mappings = Kokkos::create_mirror_view(current_mappings_);
  Kokkos::deep_copy(h_current_mappings, current_mappings_);

  std::ofstream fout(filename);
  fout << "# Minisymposia with score " << maxloc.val << "\n\n"
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
    unsigned lid = h_current_mappings(ind,m);
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
      unsigned lid = h_current_mappings(ind,m+j);
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

void Mapper::make_initial_population(unsigned popSize) {
  unsigned nlectures = lectures_.size();
  unsigned nmini = minisymposia_.size();
  unsigned ngenes = nmini + 5*10; // Only allow 10 extra minisymposia
  current_mappings_ = Kokkos::View<unsigned**>("current mappings", popSize, ngenes);
  next_mappings_ = Kokkos::View<unsigned**>("next mappings", popSize, ngenes);
  ratings_ = Kokkos::View<double*>("ratings", popSize);
  weights_ = Kokkos::View<double*>("weights", popSize);
  d_best_indices_ = Kokkos::View<unsigned*>("best indices", popSize);
  h_best_indices_ = Kokkos::create_mirror_view(d_best_indices_);

  std::vector<unsigned> numbers(ngenes);
  for(size_t i=0; i<numbers.size(); i++) {
    numbers[i] = i;
  }

  auto h_mappings = Kokkos::create_mirror_view(current_mappings_);
  for(size_t p=0; p<popSize; p++) {
    // Initialize with a randomly permuted set of numbers
    std::shuffle(numbers.begin(), numbers.end(), rng_);

    for(unsigned g=0; g<ngenes; g++) {
      h_mappings(p,g) = numbers[g];
    }
  }
  Kokkos::deep_copy(current_mappings_, h_mappings);
}

void Mapper::rate_mappings() {
  constexpr double fullness_weight = 2.0;
  constexpr double cohesion_weight = 1.0;
  unsigned popSize = current_mappings_.extent(0);
  Kokkos::parallel_for("rate mappings", popSize, KOKKOS_CLASS_LAMBDA(unsigned m) {
    auto mapping = Kokkos::subview(current_mappings_, m, Kokkos::ALL());
    unsigned nfull = count_full_minisymposia(mapping);
    double cohesion = topic_cohesion_score(mapping);
    ratings_[m] = fullness_weight*nfull + cohesion_weight*cohesion;
    if(m == 0) {
      printf("score: %lf\n", ratings_[0]);
    }
  });

  // Block until all ratings are computed since the next step uses them
  Kokkos::fence();

  sort();
}

void Mapper::sort() {
  size_t n = h_best_indices_.extent(0);
  auto h_ratings = Kokkos::create_mirror_view(ratings_);
  Kokkos::deep_copy(h_ratings, ratings_);

  // Find the indices of the most promising mappings
  for(unsigned i=0; i<n; i++) {
    h_best_indices_[i] = i;
  }

  for(unsigned i=0; i<n; i++) {
    for(unsigned j=0; j < n-i-1; j++) {
      if(h_ratings[j] < h_ratings[j+1]) {
        std::swap(h_best_indices_[j], h_best_indices_[j+1]);
        std::swap(h_ratings[j], h_ratings[j+1]);
      }
    }
  }

  Kokkos::deep_copy(d_best_indices_, h_best_indices_);
  Kokkos::deep_copy(ratings_, h_ratings);
}

void Mapper::compute_weights() {
  unsigned n = ratings_.extent(0);
  // Subtract the worst score
  Kokkos::parallel_for("compute weights", n, KOKKOS_CLASS_LAMBDA(int i) {
    weights_[i] = ratings_[i] - ratings_[n-1];
  });

  // Block until all weights are computed since the next step uses them
  Kokkos::fence();

  // Compute the sum of all weights
  double weight_sum;
  Kokkos::parallel_reduce("Weight sum", n, KOKKOS_CLASS_LAMBDA (unsigned i, double& lsum ) {
    lsum += weights_[i];
  },weight_sum);

  // Block until the sum is computed since the next step uses it
  Kokkos::fence();

  // Normalize the weights
  Kokkos::parallel_for("Normalize weights", n, KOKKOS_CLASS_LAMBDA (unsigned i) {
    weights_[i] /= weight_sum;
  });

  // Block until the weights are normalized since the next step uses them
  Kokkos::fence();
}

void Mapper::breed_population(unsigned eliteSize) {
  unsigned n = h_best_indices_.extent(0);
  // Copy over the elite items to the new population
  for(unsigned i=0; i<eliteSize; i++) {
    unsigned index = h_best_indices_[i];
    auto src = Kokkos::subview(current_mappings_, index, Kokkos::ALL());
    auto dst = Kokkos::subview(next_mappings_, i, Kokkos::ALL());
    Kokkos::deep_copy(dst, src);
  }

  // Breed to obtain the rest
  Kokkos::parallel_for("Breeding", Kokkos::RangePolicy<>(eliteSize, n), KOKKOS_CLASS_LAMBDA (unsigned i) {
    // Get the parents
    unsigned pid1 = get_parent();
    unsigned pid2 = pid1;
    while(pid2 == pid1) { // Make sure the parents are different
      pid2 = get_parent();
    }
    breed(pid1, pid2, i);
  });

  // Block until the breeding is complete since the next step uses the results
  Kokkos::fence();
}

unsigned Mapper::get_parent() const {
  unsigned n = weights_.extent(0);
  // Get a random number between 0 and 1
  auto gen = pool_.get_state();
  double r = gen.drand();
  pool_.free_state(gen);
  double sum = 0.0;
  unsigned parent = unsigned(-1);
  for(unsigned sc=0; sc<n; sc++) {
    sum += weights_[sc];
    if(r < sum) {
      parent = sc;
      break;
    }
  }
  return d_best_indices_[parent];
}

void Mapper::breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const {
  using genetic::find;

  auto mom = Kokkos::subview(current_mappings_, mom_index, Kokkos::ALL());
  auto dad = Kokkos::subview(current_mappings_, dad_index, Kokkos::ALL());
  auto child = Kokkos::subview(next_mappings_, child_index, Kokkos::ALL());

  // Determine which timeslots are carried over from the mom
  unsigned ngenes = mom.extent(0);
  auto gen = pool_.get_state();
  unsigned end_index = gen.rand(ngenes);
  pool_.free_state(gen);

  // Copy those timeslots to the child
  Kokkos::pair<size_t, size_t> mom_indices(0, end_index);
  auto mom_genes = Kokkos::subview(mom, mom_indices);
  for(unsigned i=0; i<mom_genes.extent(0); i++) {
    child(i) = mom(i);
  }

  // Fill in the gaps with dad's info
  for(unsigned i=end_index; i<ngenes; i++) {
    // Determine whether the dad's value is already in child
    unsigned current_index = i, new_index;
    while(find(mom_genes, dad(current_index), new_index)) {
      current_index = new_index;
    }
    child(i) = dad(current_index);
  }
}

void Mapper::mutate_population(double mutationRate) {
  unsigned n = next_mappings_.extent(1);
  Kokkos::parallel_for("Mutations", n, KOKKOS_CLASS_LAMBDA(unsigned sc) {
    // Don't mutate the best mapping
    if (sc == 0) return;
    for(unsigned i=0; i<n; i++) {
      auto gen = pool_.get_state();
      if(gen.drand() < mutationRate) {
        // Swap the element with another one
        unsigned j = i;
        while(i == j) {
          j = gen.rand(n);
        }
        pool_.free_state(gen);
        auto temp = next_mappings_(sc,i);
        next_mappings_(sc,i) = next_mappings_(sc,j);
        next_mappings_(sc,j) = temp;
      }
      else {
        pool_.free_state(gen);
      }
    }
  });

  // Block until the mutations are complete since the next step uses the results
  Kokkos::fence();
}

void Mapper::smush() {
  unsigned nmappings = current_mappings_.extent(0);
  unsigned nlectures = lectures_.size();
  unsigned nmini = minisymposia_.size();
  unsigned ngenes = current_mappings_.extent(1);
  Kokkos::parallel_for("Smashing!", nmappings, KOKKOS_CLASS_LAMBDA(unsigned m) {
    // Find a minisymposium that needs to move up in the schedule
    unsigned empty_index = ngenes;
    for(unsigned i=nmini; i+4<ngenes; i+=5) {
      // Determine whether this minisymposium includes any talks
      bool location_empty = true;
      for(unsigned j=0; j<5; j++) {
        if(current_mappings_(m,i+j) < nmini) {
          location_empty = false;
          break;
        }
      }
      if(!location_empty && empty_index < ngenes) {
        for(unsigned j=0; j<5; j++) {
          unsigned temp = current_mappings_(m,i+j);
          current_mappings_(m,i+j) = current_mappings_(m,empty_index+j);
          current_mappings_(m,empty_index+j) = temp;
        }
        i = empty_index;
        empty_index = ngenes;
      }
      if(location_empty && empty_index == ngenes) {
        empty_index = i;
      }
    }
  });

  // Block until the smashing is complete since the next step uses the results
  Kokkos::fence();
}