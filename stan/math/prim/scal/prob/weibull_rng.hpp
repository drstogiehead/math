#ifndef STAN_MATH_PRIM_SCAL_PROB_WEIBULL_RNG_HPP
#define STAN_MATH_PRIM_SCAL_PROB_WEIBULL_RNG_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/scal/err/check_consistent_sizes.hpp>
#include <stan/math/prim/scal/err/check_positive_finite.hpp>
#include <boost/random/weibull_distribution.hpp>
#include <boost/random/variate_generator.hpp>

namespace stan {
namespace math {

/**
 * Return a Weibull random variate for the given shape
 * and scale parameters using the specified random number generator.
 *
 * alpha and sigma can each be a scalar or a one-dimensional container. Any
 * non-scalar inputs must be the same size.
 *
 * @tparam T_shape Type of shape parameter
 * @tparam T_scale Type of scale parameter
 * @tparam RNG type of random number generator
 * @param alpha (Sequence of) positive shape parameter(s)
 * @param sigma (Sequence of) positive scale parameter(s)
 * @param rng random number generator
 * @return (Sequence of) Weibull random variate(s)
 * @throw std::domain_error if alpha is nonpositive or sigma is nonpositive
 * @throw std::invalid_argument if non-scalar arguments are of different
 * sizes
 */
template <typename T_shape, typename T_scale, class RNG>
inline typename VectorBuilder<true, double, T_shape, T_scale>::type weibull_rng(
    const T_shape& alpha, const T_scale& sigma, RNG& rng) {
  using boost::random::weibull_distribution;
  using boost::variate_generator;

  static const char* function = "weibull_rng";

  check_positive_finite(function, "Shape parameter", alpha);
  check_positive_finite(function, "Scale parameter", sigma);
  check_consistent_sizes(function, "Shape parameter", alpha, "Scale Parameter",
                         sigma);

  scalar_seq_view<T_shape> alpha_vec(alpha);
  scalar_seq_view<T_scale> sigma_vec(sigma);
  size_t N = max_size(alpha, sigma);
  VectorBuilder<true, double, T_shape, T_scale> output(N);

  for (size_t n = 0; n < N; ++n) {
    variate_generator<RNG&, weibull_distribution<> > weibull_rng(
        rng, weibull_distribution<>(alpha_vec[n], sigma_vec[n]));
    output[n] = weibull_rng();
  }

  return output.data();
}

}  // namespace math
}  // namespace stan
#endif
