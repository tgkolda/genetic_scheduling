#ifndef GENETIC_H
#define GENETIC_H

#include "Utility.hpp"
#include "Kokkos_Random.hpp"
#include "Kokkos_Sort.hpp"
#include "Kokkos_StdAlgorithms.hpp"
#include <random>

template<class Runner>
class Genetic {
public:
  Genetic(Runner& runner);
  auto run(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations);

  // These items should be private but have to be public because #GPUs
  void rate_population();
  void compute_weights();
  void breed_population(unsigned eliteSize);
  void mutate_population(double mutationRate);
private:
  void sort();
  KOKKOS_INLINE_FUNCTION auto get_population_member(unsigned i, bool current=true) const;
  void make_initial_population(unsigned popSize);
  KOKKOS_INLINE_FUNCTION unsigned get_parent() const;
  KOKKOS_INLINE_FUNCTION void breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const;

  Runner runner_;
  typename Runner::ViewType current_population_;
  typename Runner::ViewType next_population_;
  Kokkos::View<double*> ratings_;
  Kokkos::View<double*> weights_;
  Kokkos::View<unsigned*> permutation_;
  std::default_random_engine rng_;
  Kokkos::Random_XorShift64_Pool<> pool_;
};

template<class Runner>
Genetic<Runner>::Genetic(Runner& runner) : runner_(runner), pool_(5374857) { }

template<class Runner>
auto Genetic<Runner>::run(unsigned popSize, unsigned eliteSize, double mutationRate, unsigned generations) {
  // Allocate space for the Kokkos Views
  ratings_ = Kokkos::View<double*>("ratings", popSize);
  weights_ = Kokkos::View<double*>("weights", popSize);

  make_initial_population(popSize);

  for(unsigned g=0; g<generations; g++) {
    std::cout << "generation " << g << ": ";

    rate_population();
    sort();
    compute_weights();
    breed_population(eliteSize);
    mutate_population(mutationRate);
    std::swap(current_population_, next_population_);
  }

  std::cout << "generation " << generations << ": ";
  rate_population();
  sort();

  unsigned best_pop_subscript;
  Kokkos::deep_copy(best_pop_subscript, Kokkos::subview(permutation_, popSize-1));
  auto h_population = Kokkos::create_mirror_view(current_population_);
  Kokkos::deep_copy(h_population, current_population_);
  if constexpr(h_population.rank == 2) {
    return Kokkos::subview(h_population, best_pop_subscript, Kokkos::ALL());
  }
  else {
    return Kokkos::subview(h_population, best_pop_subscript, Kokkos::ALL(), Kokkos::ALL());
  }
}

template<class Runner>
auto Genetic<Runner>::get_population_member(unsigned i, bool current) const {
  if constexpr(current_population_.rank == 2) {
    if(current) {
      return Kokkos::subview(current_population_, i, Kokkos::ALL());
    }
    return Kokkos::subview(next_population_, i, Kokkos::ALL());
  }
  else {
    if(current) {
      return Kokkos::subview(current_population_, i, Kokkos::ALL(), Kokkos::ALL());
    }
    return Kokkos::subview(next_population_, i, Kokkos::ALL(), Kokkos::ALL());
  }
}

template<class Runner>
void Genetic<Runner>::make_initial_population(unsigned popSize) {
  // Allocate memory for the population
  current_population_ = runner_.make_initial_population(popSize);
  next_population_ = runner_.make_initial_population(popSize);

  // Calculate how much data is in a single population member
  unsigned nentries = current_population_.extent(1);
  if(current_population_.rank == 3) {
    nentries *= current_population_.extent(2);
  }

  // Create a vector of integers [0, popSize)
  // We will permute this to get our population members
  std::vector<unsigned> ints(nentries);
  for(unsigned i=0; i<nentries; i++) {
    ints[i] = i;
  }

  // Get a host mirror of the device data
  auto h_current_population = Kokkos::create_mirror_view(current_population_);

  // Populate the host mirror
  for(unsigned i=0; i<popSize; i++) {
    // Randomly permute the integers
    std::shuffle(ints.begin(), ints.end(), rng_);

    // Assign them to the host mirror
    if constexpr(current_population_.rank == 2) {
      for(unsigned j=0; j<current_population_.extent(1); j++) {
        h_current_population(i,j) = ints[j];
      }
    }
    else {
      unsigned val = 0;
      for(unsigned j=0; j<current_population_.extent(1); j++) {
        for(unsigned k=0; k<current_population_.extent(2); k++) {
          h_current_population(i,j,k) = val;
          val++;
        }
      }
    }
  }

  // Copy data to device
  Kokkos::deep_copy(current_population_, h_current_population);
}

template<class Runner>
void Genetic<Runner>::rate_population() {
  unsigned popSize = current_population_.extent(0);
  Kokkos::parallel_for("rate population", popSize, KOKKOS_CLASS_LAMBDA(int i) {
    auto member = get_population_member(i);
    ratings_(i) = runner_.rate(member);
  });

  // Block until the GPU work is complete
  Kokkos::fence();
}

template<class Runner>
void Genetic<Runner>::compute_weights() {
  unsigned popSize = ratings_.extent(0);

  // Block until the GPU work is complete
  Kokkos::fence();

  // Subtract the lowest score from all weights
  Kokkos::parallel_for("compute weights", popSize, KOKKOS_CLASS_LAMBDA(int i) {
    weights_[i] = ratings_[i] - ratings_[0];
  });

  // Block until all weights are computed since the next step uses them
  Kokkos::fence();

  // Compute the sum of all weights
  double weight_sum;
  Kokkos::parallel_reduce("Weight sum", popSize, KOKKOS_CLASS_LAMBDA (unsigned i, double& lsum ) {
    lsum += weights_[i];
  },weight_sum);

  // Block until the sum is computed since the next step uses it
  Kokkos::fence();

  // Normalize the weights
  Kokkos::parallel_for("Normalize weights", popSize, KOKKOS_CLASS_LAMBDA (unsigned i) {
    weights_[i] /= weight_sum;
//    printf("%i %lf %lf %i\n", i, weights_[i], ratings_[i], permutation_[i]);
  });

  // Block until the weights are normalized since the next step uses them
  Kokkos::fence();
}

template<class Runner>
void Genetic<Runner>::breed_population(unsigned eliteSize) {
  unsigned popSize = current_population_.extent(0);
  unsigned breed_index_cutoff = popSize - eliteSize; // not inclusive

  Kokkos::parallel_for("Breeding", popSize, KOKKOS_CLASS_LAMBDA (unsigned i) {
    // Breed to obtain these indices
    if(i < breed_index_cutoff) {
      // Get the parents
      unsigned pid1 = get_parent();
      unsigned pid2 = pid1;
      while(pid2 == pid1) { // Make sure the parents are different
        pid2 = get_parent();
      }
//      printf("breeding current_population(%i) and (%i) to obtain next_population(%i)\n", pid1, pid2, i);
      breed(pid1, pid2, i);
    }
    // Copy over the elite items to the new population
    else {
      unsigned elite_index = permutation_(i);
//      printf("copying current_population(%i) with rating %lf to next_population(%i)\n", elite_index, ratings_(i), i);
      for(unsigned j=0; j<current_population_.extent(1); j++) {
        if constexpr(current_population_.rank == 2) {
          next_population_(i,j) = current_population_(elite_index, j);
        }
        else {
          for(unsigned k=0; k<current_population_.extent(2); k++) {
            next_population_(i,j,k) = current_population_(elite_index, j, k);
          }
        }
      }
    }
  });

  // Block until the breeding is complete since the next step uses the results
  Kokkos::fence();
}

template<class Runner>
unsigned Genetic<Runner>:: get_parent() const {
  // Get a random number between 0 and 1
  auto gen = pool_.get_state();
  double r = gen.drand();
  pool_.free_state(gen);

  // Determine which population member has been randomly chosen
  double sum = 0.0;
  unsigned parent = 0;
  for(unsigned i=0; i<weights_.extent(0); i++) {
    sum += weights_[i];
    if(r < sum) {
      parent = i;
      break;
    }
  }
  return permutation_(parent);
}

template<class Runner>
void Genetic<Runner>::breed(unsigned mom_index, unsigned dad_index, unsigned child_index) const {
  using genetic::find;

  // Get the parent and child population members
  auto mom = get_population_member(mom_index);
  auto dad = get_population_member(dad_index);
  auto child = get_population_member(child_index, false);

  // Determine which genes are carried over from the mom
  unsigned ngenes = current_population_.extent(current_population_.rank-1);
  auto gen = pool_.get_state();
  unsigned end_index = gen.rand(ngenes);
  pool_.free_state(gen);

  // Copy those timeslots to the child
  if constexpr(current_population_.rank == 2) {
    for(unsigned i=0; i<end_index; i++) {
      child(i) = mom(i);
    }
  }
  else {
    for(unsigned i=0; i<child.extent(0); i++) {
      for(unsigned j=0; j<end_index; j++) {
        child(i, j) = mom(i, j);
      }
    }
  }

  // Fill in the gaps with dad's info
  Kokkos::pair<size_t, size_t> mom_indices(0, end_index);
  if constexpr(current_population_.rank == 2) {
    auto mom_genes = Kokkos::subview(mom, mom_indices);
    for(unsigned i=end_index; i<child.extent(0); i++) {
      // Determine whether the dad's value is already in child
      unsigned current_index = i, new_index;
      while(find(mom_genes, dad(current_index), new_index)) {
        current_index = new_index;
      }
      child(i) = dad(current_index);
    }
  }
  else {
    auto mom_genes = Kokkos::subview(mom, Kokkos::ALL(), mom_indices);
    for(unsigned r=0; r<child.extent(0); r++) {
      for(unsigned c=end_index; c<child.extent(1); c++) {
        // Determine whether the dad's value is already in child
        Kokkos::pair<size_t, size_t> current_index(r,c), new_index;
        while(find(mom_genes, dad(current_index.first, current_index.second), new_index)) {
          current_index = new_index;
        }
        child(r, c) = dad(current_index.first, current_index.second);
      }
    }
  }
}

template<class Runner>
void Genetic<Runner>::mutate_population(double mutationRate) {
  using genetic::swap;
  unsigned popSize = current_population_.extent(0);

  // The constexpr can't live inside the device lambda
  if constexpr(current_population_.rank == 2) {
    Kokkos::parallel_for("Mutations", popSize, KOKKOS_CLASS_LAMBDA(unsigned p) {
      // Don't mutate the best population member
      if (p == popSize-1) return;
      for(unsigned i=0; i<current_population_.extent(1); i++) {
        auto gen = pool_.get_state();
        if(gen.drand() < mutationRate) {
          // Swap the element with another
          unsigned i2 = i;
          while(i2 == i || runner_.out_of_bounds(current_population_(p,i2))) {
            i2 = gen.rand(current_population_.extent(1));
          }
          pool_.free_state(gen);
          swap(next_population_(p,i), next_population_(p,i2));
        }
        else {
          pool_.free_state(gen);
        }
      }
    });
  }
  else {
    Kokkos::parallel_for("Mutations", popSize, KOKKOS_CLASS_LAMBDA(unsigned p) {
      // Don't mutate the best population member
      if (p == popSize-1) return;
      for(unsigned i=0; i<current_population_.extent(1); i++) {
        for(unsigned j=0; j<current_population_.extent(2); j++) {
          auto gen = pool_.get_state();
          if(gen.drand() < mutationRate) {
            // Swap the element with another slot
            unsigned j2 = j;
            while(j2 == j || runner_.out_of_bounds(current_population_(p,i,j2))) {
              j2 = gen.rand(current_population_.extent(2));
            }
            pool_.free_state(gen);
            swap(next_population_(p,i,j), next_population_(p,i,j2));
          }
          else {
            pool_.free_state(gen);
          }
        }
      }
    });
  }

  // Block until the mutations are complete since the next step uses the results
  Kokkos::fence();
}

template<class Runner>
void Genetic<Runner>:: sort() {
  // Kokkos uses a bin sort which requires the minimum and maximum elements of the array
  using KeyViewType = Kokkos::View<double*>;
  using BinOp = Kokkos::BinOp1D<KeyViewType>;
  using Kokkos::Experimental::minmax_element;
  using Kokkos::Experimental::begin;
  double min, max;
  auto itpair = minmax_element(Kokkos::DefaultExecutionSpace(), ratings_);
  Kokkos::deep_copy(min, Kokkos::subview(ratings_, itpair.first-begin(ratings_)));
  Kokkos::deep_copy(max, Kokkos::subview(ratings_, itpair.second-begin(ratings_)));
  BinOp bin_op(ratings_.extent(0), min, max);
  Kokkos::BinSort<KeyViewType, BinOp> bin_sort(ratings_, bin_op);
  bin_sort.create_permute_vector();
  bin_sort.sort(ratings_);
  permutation_ = bin_sort.get_permute_vector();
  printf("%lf\n", max);
}

#endif /* GENETIC_H */