#ifndef STAN_MATH_PRIM_SCAL_PROB_LOGNORMAL_CDF_HPP
#define STAN_MATH_PRIM_SCAL_PROB_LOGNORMAL_CDF_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/err.hpp>
#include <stan/math/prim/scal/fun/erfc.hpp>
#include <stan/math/prim/scal/fun/size_zero.hpp>
#include <stan/math/prim/scal/fun/constants.hpp>
#include <stan/math/prim/scal/fun/value_of.hpp>
#include <cmath>

namespace stan {
namespace math {

template <typename T_y, typename T_loc, typename T_scale>
return_type_t<T_y, T_loc, T_scale> lognormal_cdf(const T_y& y, const T_loc& mu,
                                                 const T_scale& sigma) {
  static const char* function = "lognormal_cdf";

  using T_partials_return = partials_return_t<T_y, T_loc, T_scale>;

  T_partials_return cdf = 1.0;

  using std::exp;
  using std::log;

  if (size_zero(y, mu, sigma)) {
    return cdf;
  }

  check_not_nan(function, "Random variable", y);
  check_nonnegative(function, "Random variable", y);
  check_finite(function, "Location parameter", mu);
  check_positive_finite(function, "Scale parameter", sigma);

  operands_and_partials<T_y, T_loc, T_scale> ops_partials(y, mu, sigma);

  scalar_seq_view<T_y> y_vec(y);
  scalar_seq_view<T_loc> mu_vec(mu);
  scalar_seq_view<T_scale> sigma_vec(sigma);
  size_t N = max_size(y, mu, sigma);

  for (size_t i = 0; i < size(y); i++) {
    if (value_of(y_vec[i]) == 0.0) {
      return ops_partials.build(0.0);
    }
  }

  for (size_t n = 0; n < N; n++) {
    const T_partials_return y_dbl = value_of(y_vec[n]);
    const T_partials_return mu_dbl = value_of(mu_vec[n]);
    const T_partials_return sigma_dbl = value_of(sigma_vec[n]);
    const T_partials_return scaled_diff
        = (log(y_dbl) - mu_dbl) / (sigma_dbl * SQRT_TWO);
    const T_partials_return rep_deriv = SQRT_TWO_OVER_SQRT_PI * 0.5
                                        * exp(-scaled_diff * scaled_diff)
                                        / sigma_dbl;

    const T_partials_return cdf_ = 0.5 * erfc(-scaled_diff);
    cdf *= cdf_;

    if (!is_constant_all<T_y>::value) {
      ops_partials.edge1_.partials_[n] += rep_deriv / cdf_ / y_dbl;
    }
    if (!is_constant_all<T_loc>::value) {
      ops_partials.edge2_.partials_[n] -= rep_deriv / cdf_;
    }
    if (!is_constant_all<T_scale>::value) {
      ops_partials.edge3_.partials_[n]
          -= rep_deriv * scaled_diff * SQRT_TWO / cdf_;
    }
  }

  if (!is_constant_all<T_y>::value) {
    for (size_t n = 0; n < size(y); ++n) {
      ops_partials.edge1_.partials_[n] *= cdf;
    }
  }
  if (!is_constant_all<T_loc>::value) {
    for (size_t n = 0; n < size(mu); ++n) {
      ops_partials.edge2_.partials_[n] *= cdf;
    }
  }
  if (!is_constant_all<T_scale>::value) {
    for (size_t n = 0; n < size(sigma); ++n) {
      ops_partials.edge3_.partials_[n] *= cdf;
    }
  }
  return ops_partials.build(cdf);
}

}  // namespace math
}  // namespace stan
#endif
