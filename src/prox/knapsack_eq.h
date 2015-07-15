#ifndef SDCA_PROX_KNAPSACK_EQ_H
#define SDCA_PROX_KNAPSACK_EQ_H

#include <algorithm>
#include <iterator>
#include <numeric>

#include "proxdef.h"

/*
 * Based on the Algorithm 3.1 in
 * Kiwiel, K. C. "Variable fixing algorithms for the continuous
 * quadratic knapsack problem."
 * Journal of Optimization Theory and Applications 136.3 (2008): 445-458.
 */

namespace sdca {

template <typename ForwardIterator,
          typename Summator = std_sum<ForwardIterator>>
thresholds<ForwardIterator>
thresholds_knapsack_eq(
    ForwardIterator first,
    ForwardIterator last,
    const typename std::iterator_traits<ForwardIterator>::value_type lo = 0,
    const typename std::iterator_traits<ForwardIterator>::value_type hi = 1,
    const typename std::iterator_traits<ForwardIterator>::value_type rhs = 1,
    Summator sum = Summator()
    ) {
  using Type = typename std::iterator_traits<ForwardIterator>::value_type;

  // Initialization
  const auto num_elements = std::distance(first, last);
  Type t = (sum(first, last, static_cast<Type>(0)) - rhs) /
    static_cast<Type>(num_elements);

  ForwardIterator m_first = first;
  ForwardIterator m_last = last;
  for (auto iter = num_elements; iter > 0; --iter) {
    // Feasibility check
    Type tt = lo + t;
    auto lo_it = std::partition(m_first, m_last, [=](const Type &x){
      return x > tt; });
    Type infeas_lo = + static_cast<Type>(std::distance(lo_it, m_last))
      * tt - sum(lo_it, m_last, static_cast<Type>(0));

    tt = hi + t;
    auto hi_it = std::partition(m_first, lo_it, [=](const Type &x){
      return x > tt; });
    Type infeas_hi = - static_cast<Type>(std::distance(m_first, hi_it))
      * tt + sum(m_first, hi_it, static_cast<Type>(0));

    // Variable fixing (using the incremental multiplier formula (23))
    if (infeas_lo > infeas_hi) {
      m_last = lo_it;
      tt = +infeas_lo;
    } else if (infeas_lo < infeas_hi) {
      m_first = hi_it;
      tt = -infeas_hi;
    } else {
      m_first = hi_it;
      m_last = lo_it;
      break;
    }
    auto size = std::distance(m_first, m_last);
    if (size) {
      t += tt / static_cast<Type>(size);
    } else {
      break;
    }
  }

  return make_thresholds(t, lo, hi, m_first, m_last);
}

template <typename ForwardIterator,
          typename Summator = std_sum<ForwardIterator>>
inline
void
project_knapsack_eq(
    ForwardIterator first,
    ForwardIterator last,
    const typename std::iterator_traits<ForwardIterator>::value_type lo = 0,
    const typename std::iterator_traits<ForwardIterator>::value_type hi = 1,
    const typename std::iterator_traits<ForwardIterator>::value_type rhs = 1,
    Summator sum = Summator()
    ) {
  project(first, last,
          thresholds_knapsack_eq<ForwardIterator, Summator>, lo, hi, rhs, sum);
}

template <typename ForwardIterator,
          typename Summator = std_sum<ForwardIterator>>
inline
void
project_knapsack_eq(
    ForwardIterator first,
    ForwardIterator last,
    ForwardIterator aux_first,
    ForwardIterator aux_last,
    const typename std::iterator_traits<ForwardIterator>::value_type lo = 0,
    const typename std::iterator_traits<ForwardIterator>::value_type hi = 1,
    const typename std::iterator_traits<ForwardIterator>::value_type rhs = 1,
    Summator sum = Summator()
    ) {
  project(first, last, aux_first, aux_last,
          thresholds_knapsack_eq<ForwardIterator, Summator>, lo, hi, rhs, sum);
}

template <typename ForwardIterator,
          typename Summator = std_sum<ForwardIterator>>
inline
void
project_knapsack_eq(
    const typename std::iterator_traits<ForwardIterator>::difference_type dim,
    ForwardIterator first,
    ForwardIterator last,
    ForwardIterator aux_first,
    ForwardIterator aux_last,
    const typename std::iterator_traits<ForwardIterator>::value_type lo = 0,
    const typename std::iterator_traits<ForwardIterator>::value_type hi = 1,
    const typename std::iterator_traits<ForwardIterator>::value_type rhs = 1,
    Summator sum = Summator()
    ) {
  project(dim, first, last, aux_first, aux_last,
          thresholds_knapsack_eq<ForwardIterator, Summator>, lo, hi, rhs, sum);
}

}

#endif
