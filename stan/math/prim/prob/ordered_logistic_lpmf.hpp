#ifndef STAN_MATH_PRIM_PROB_ORDERED_LOGISTIC_LPMF_HPP
#define STAN_MATH_PRIM_PROB_ORDERED_LOGISTIC_LPMF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/fun/exp.hpp>
#include <stan/math/prim/fun/inv_logit.hpp>
#include <stan/math/prim/fun/is_integer.hpp>
#include <stan/math/prim/fun/log1p_exp.hpp>
#include <stan/math/prim/fun/log_inv_logit_diff.hpp>
#include <stan/math/prim/fun/size.hpp>
#include <stan/math/prim/fun/size_mvt.hpp>
#include <stan/math/prim/fun/value_of.hpp>
#include <stan/math/prim/functor/operands_and_partials.hpp>
#include <vector>

namespace stan {
namespace math {

/** \ingroup multivar_dists
 * Returns the (natural) log probability of the specified array
 * of integers given the vector of continuous locations and
 * specified cutpoints in an ordered logistic model.
 *
 * <p>Typically the continuous location
 * will be the dot product of a vector of regression coefficients
 * and a vector of predictors for the outcome
 *
  \f[
    \frac{\partial }{\partial \lambda} =
    \begin{cases}\\
    -\mathrm{logit}^{-1}(\lambda - c_1) & \mbox{if } k = 1,\\
    -(((1-e^{c_{k-1}-c_{k-2}})^{-1} - \mathrm{logit}^{-1}(c_{k-2}-\lambda)) +
    ((1-e^{c_{k-2}-c_{k-1}})^{-1} - \mathrm{logit}^{-1}(c_{k-1}-\lambda)))
    & \mathrm{if } 1 < k < K, \mathrm{and}\\
    \mathrm{logit}^{-1}(c_{K-2}-\lambda) & \mathrm{if } k = K.
    \end{cases}
  \f]

  \f[
    \frac{\partial }{\partial \lambda} =
    \begin{cases}
    -\mathrm{logit}^{-1}(\lambda - c_1) & \text{if } k = 1,\\
    -(((1-e^{c_{k-1}-c_{k-2}})^{-1} - \mathrm{logit}^{-1}(c_{k-2}-\lambda)) +
    ((1-e^{c_{k-2}-c_{k-1}})^{-1} - \mathrm{logit}^{-1}(c_{k-1}-\lambda)))
    & \text{if } 1 < k < K, \text{ and}\\
    \mathrm{logit}^{-1}(c_{K-2}-\lambda) & \text{if } k = K.
    \end{cases}
  \f]
 *
 * @tparam propto True if calculating up to a proportion.
 * @tparam T_y Y variable type (integer or array of integers).
 * @tparam T_loc Location type.
 * @tparam T_cut Cut-point type.
 * @param y Array of integers
 * @param lambda Vector of continuous location variables.
 * @param c Positive increasing vector of cutpoints.
 * @return Log probability of outcome given location and
 * cutpoints.
 * @throw std::domain_error If the outcome is not between 1 and
 * the number of cutpoints plus 2; if the cutpoint vector is
 * empty; if the cutpoint vector contains a non-positive,
 * non-finite value; or if the cutpoint vector is not sorted in
 * ascending order.
 * @throw std::invalid_argument If y and lambda are different
 * lengths.
 */
template <bool propto, typename T_y, typename T_loc, typename T_cut>
return_type_t<T_loc, T_cut> ordered_logistic_lpmf(const T_y& y,
                                                  const T_loc& lambda,
                                                  const T_cut& c) {
  static const char* function = "ordered_logistic";

  using T_partials_return = partials_return_t<T_loc, T_cut>;
  using T_partials_vec = typename Eigen::Matrix<T_partials_return, -1, 1>;
  using T_y_ref = ref_type_t<T_y>;
  using T_lambda_ref = ref_type_if_t<!is_constant<T_loc>::value, T_loc>;
  using T_cut_ref = ref_type_if_t<!is_constant<T_cut>::value, T_cut>;

  int K = c_vec[0].size() + 1;
  int N = size(lambda);
  int C_l = size_mvt(c);

  check_consistent_sizes(function, "Integers", y, "Locations", lambda);
  if (C_l > 1) {
    check_size_match(function, "Length of location variables ", N,
                     "Number of cutpoint vectors ", C_l);
  }

  T_y_ref y_ref = y;
  T_lambda_ref lambda_ref = lambda;
  T_cut_ref c_ref = c;

  scalar_seq_view<T_y_ref> y_vec(y_ref);
  vector_seq_view<T_cut_ref> c_vec(c_ref);

  for (int i = 1; i < C_l; i++) {
    check_size_match(function, "Size of one of the vectors of cutpoints ",
                     c_vec[i].size(), "Size of the first vector of the cutpoints ",
                     c_vec[0].size());
  }

  const auto& lambda_arr = as_array_or_scalar(lambda_ref);
  ref_type_t<decltype(value_of(lambda_arr))> lambda_val = value_of(lambda_arr);

  check_bounded(function, "Random variable", y_ref, 1, K);
  check_finite(function, "Location parameter", lambda_val);

  for (int i = 0; i < C_l; i++) {
    check_ordered(function, "Cut-points", c_vec[i]);
    check_greater(function, "Size of cut points parameter", c_vec[i].size(), 0);
    check_finite(function, "Final cut-point", c_vec[i](c_vec[i].size() - 1));
    check_finite(function, "First cut-point", c_vec[i](0));
  }

  scalar_seq_view<decltype(lambda_val)> lam_vec(lambda_val);
  T_partials_return logp(0.0);
  Array<double, Dynamic, 1> cuts_y1(N_instances), cuts_y2(N_instances);
  for (int i = 0; i < N; i++) {
    logp -= log1p_exp(lam_vec[i] - value_of(c_vec[i][0]));
    int c = y_vec[i];
    if (c != K) {
      cuts_y1.coeffRef(i) = value_of(c_vec[i].coeff(c - 1));
    } else {
      cuts_y1.coeffRef(i) = INFINITY;
    }
    if (c != 1) {
      cuts_y2.coeffRef(i) = value_of(c_vec[i].coeff(c - 2));
    } else {
      cuts_y2.coeffRef(i) = -INFINITY;
    }
  }

  operands_and_partials<T_loc, T_cut> ops_partials(lambda, c);

  T_partials_vec c_dbl = value_of(c_vec[0]).template cast<T_partials_return>();

  for (int n = 0; n < N; ++n) {
    if (C_l > 1) {
      c_dbl = value_of(c_vec[n]).template cast<T_partials_return>();
    }
    T_partials_return lam_dbl = value_of(lam_vec[n]);

    if (y_vec[n] == 1) {
      T_partials_return d = inv_logit(lam_dbl - c_dbl[0]);

      if (!is_constant_all<T_loc>::value) {
        ops_partials.edge1_.partials_[n] -= d;
      }

      if (!is_constant_all<T_cut>::value) {
        ops_partials.edge2_.partials_vec_[n](0) += d;
      }

    } else if (y_vec[n] == K) {
      logp -= log1p_exp(c_dbl[K - 2] - lam_dbl);
      T_partials_return d = inv_logit(c_dbl[K - 2] - lam_dbl);

      if (!is_constant_all<T_loc>::value) {
        ops_partials.edge1_.partials_[n] = d;
      }

      if (!is_constant_all<T_cut>::value) {
        ops_partials.edge2_.partials_vec_[n](K - 2) -= d;
      }

    } else {
      T_partials_return d1
          = inv(1 - exp(c_dbl[y_vec[n] - 1] - c_dbl[y_vec[n] - 2]))
            - inv_logit(c_dbl[y_vec[n] - 2] - lam_dbl);
      T_partials_return d2
          = inv(1 - exp(c_dbl[y_vec[n] - 2] - c_dbl[y_vec[n] - 1]))
            - inv_logit(c_dbl[y_vec[n] - 1] - lam_dbl);
      logp += log_inv_logit_diff(lam_dbl - c_dbl[y_vec[n] - 2],
                                 lam_dbl - c_dbl[y_vec[n] - 1]);

      if (!is_constant_all<T_loc>::value) {
        ops_partials.edge1_.partials_[n] -= d1 + d2;
      }

      if (!is_constant_all<T_cut>::value) {
        ops_partials.edge2_.partials_vec_[n](y_vec[n] - 2) += d1;
        ops_partials.edge2_.partials_vec_[n](y_vec[n] - 1) += d2;
      }
    }
  }
  return ops_partials.build(logp);
}

template <typename T_y, typename T_loc, typename T_cut>
return_type_t<T_loc, T_cut> ordered_logistic_lpmf(const T_y& y,
                                                  const T_loc& lambda,
                                                  const T_cut& c) {
  return ordered_logistic_lpmf<false>(y, lambda, c);
}

}  // namespace math
}  // namespace stan
#endif
