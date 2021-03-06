#ifndef SDCA_SOLVE_L2_HINGE_TOPK_LOSS_H
#define SDCA_SOLVE_L2_HINGE_TOPK_LOSS_H

#include "objective_base.h"
#include "prox/topk_simplex.h"
#include "prox/topk_simplex_biased.h"
#include "solvedef.h"

namespace sdca {

template <typename Data,
          typename Result,
          typename Summation>
struct l2_hinge_topk : public objective_base<Data, Result, Summation> {
  typedef objective_base<Data, Result, Summation> base;
  const difference_type k;
  const Result c;

  l2_hinge_topk(
      const size_type __k,
      const Result __c,
      const Summation __sum
    ) :
      base::objective_base(__c / static_cast<Result>(__k), __sum),
      k(static_cast<difference_type>(__k)),
      c(__c)
  {}

  inline std::string to_string() const {
    std::ostringstream str;
    str << "l2_hinge_topk (k = " << k << ", C = " << c << ", gamma = 0)";
    return str.str();
  }

  void update_variables(
      const blas_int num_classes,
      const Data norm2,
      Data* variables,
      Data* scores
      ) const {
    Data *first(variables + 1), *last(variables + num_classes), a(1 / norm2);
    Result rhs(c), rho(1);

    // 1. Prepare a vector to project in 'variables'.
    sdca_blas_axpby(num_classes, a, scores, -1, variables);
    a -= variables[0];
    std::for_each(first, last, [=](Data &x){ x += a; });

    // 2. Proximal step (project 'variables', use 'scores' as scratch space)
    prox_topk_simplex_biased(first, last,
      scores + 1, scores + num_classes, k, rhs, rho, this->sum);

    // 3. Recover the updated variables
    *variables = static_cast<Data>(std::min(rhs,
      this->sum(first, last, static_cast<Result>(0)) ));
    std::for_each(first, last, [](Data &x){ x = -x; });
  }

  inline Result
  primal_loss(
      const blas_int num_classes,
      Data* scores
    ) const {
    Data *first(scores + 1), *last(scores + num_classes), a(1 - scores[0]);
    std::for_each(first, last, [=](Data &x){ x += a; });

    // Find k largest elements
    std::nth_element(first, first + k - 1, last, std::greater<Data>());

    // max{0, sum_k_largest} (division by k happens later)
    return std::max(static_cast<Result>(0),
      this->sum(first, first + k, static_cast<Result>(0)));
  }
};

template <typename Data,
          typename Result,
          typename Summation>
struct l2_hinge_topk_smooth : public objective_base<Data, Result, Summation> {
  typedef objective_base<Data, Result, Summation> base;
  const difference_type k;
  const Result c;
  const Result gamma;
  const Result c_div_gamma;
  const Result gamma_div_c;
  const Result gamma_div_2c;

  l2_hinge_topk_smooth(
      const size_type __k,
      const Result __c,
      const Result __gamma,
      const Summation __sum
    ) :
      base::objective_base(__c / __gamma, __sum),
      k(static_cast<difference_type>(__k)),
      c(__c),
      gamma(__gamma),
      c_div_gamma(__c / __gamma),
      gamma_div_c(__gamma / __c),
      gamma_div_2c(__gamma / (2 * __c))
  {}

  inline std::string to_string() const {
    std::ostringstream str;
    str << "l2_hinge_topk (k = " << k << ", C = " << c << ", "
      "gamma = " << gamma << ")";
    return str.str();
  }

  void update_variables(
      const blas_int num_classes,
      const Data norm2,
      Data* variables,
      Data* scores
      ) const {
    Data *first(variables + 1), *last(variables + num_classes);
    Result rhs(c), rho(static_cast<Result>(norm2) /
      (static_cast<Result>(norm2) + gamma_div_c));

    // 1. Prepare a vector to project in 'variables'.
    Data a(static_cast<Data>(rho) / norm2);
    sdca_blas_axpby(num_classes, a, scores, -static_cast<Data>(rho), variables);
    a -= variables[0];
    std::for_each(first, last, [=](Data &x){ x += a; });

    // 2. Proximal step (project 'variables', use 'scores' as scratch space)
    prox_topk_simplex_biased(first, last,
      scores + 1, scores + num_classes, k, rhs, rho, this->sum);

    // 3. Recover the updated variables
    *variables = static_cast<Data>(std::min(rhs,
      this->sum(first, last, static_cast<Result>(0)) ));
    std::for_each(first, last, [](Data &x){ x = -x; });
  }

  inline Result
  dual_loss(
      const blas_int num_classes,
      const Data* variables
    ) const {
    return static_cast<Result>(variables[0])
      - gamma_div_2c * static_cast<Result>(
      sdca_blas_dot(num_classes - 1, variables + 1, variables + 1));
  }

  inline Result
  primal_loss(
      const blas_int num_classes,
      Data* scores
    ) const {
    Data *first(scores + 1), *last(scores + num_classes), a(1 - scores[0]);
    std::for_each(first, last, [=](Data &x){ x += a; });

    // loss = 1/gamma (<p,h> - 1/2 <p,p>), p =prox_{k,gamma}(h), h = c + a
    auto t = thresholds_topk_simplex(first, last, k, gamma, this->sum);
    Result ph = dot_prox(t, first, last, this->sum);
    Result pp = dot_prox_prox(t, first, last, this->sum);

    // (division by gamma happens later)
    return ph - static_cast<Result>(0.5) * pp;
  }
};

}

#endif
