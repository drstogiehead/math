#ifndef STAN_MATH_OPENCL_REV_LBETA_HPP
#define STAN_MATH_OPENCL_REV_LBETA_HPP
#ifdef STAN_OPENCL

#include <stan/math/prim/meta/is_kernel_expression.hpp>
#include <stan/math/opencl/matrix_cl.hpp>
#include <stan/math/opencl/kernel_generator.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/rev/fun/value_of.hpp>
#include <stan/math/rev/core/reverse_pass_callback.hpp>
#include <stan/math/prim/fun/value_of.hpp>

namespace stan {
namespace math {

/**
 * Return the elementwise `lbeta()` on two input kernel
 * generator expression
 *
 * @tparam T_a type of first expression
 * @tparam T_b type of second expression
 * @param a first expression
 * @param b second expression
 * @return elementwise `lbeta()`
 */
template <
    typename T_a, typename T_b,
    require_all_nonscalar_prim_or_rev_kernel_expression_t<T_a, T_b>* = nullptr,
    require_any_var_t<T_a, T_b>* = nullptr>
inline auto lbeta(const T_a& a, const T_b& b) {
  const arena_t<T_a>& a_arena = a;
  const arena_t<T_b>& b_arena = b;

  var_value<matrix_cl<double>> res
      = lbeta(value_of(a_arena), value_of(b_arena));
  reverse_pass_callback([a_arena, b_arena, res]() mutable {
    auto digamma_ab = digamma(value_of(a_arena) + value_of(b_arena));
    if (!is_constant<T_a>::value && !is_constant<T_b>::value) {
      auto& a_adj = forward_as<var_value<matrix_cl<double>>>(a_arena).adj();
      auto& b_adj = forward_as<var_value<matrix_cl<double>>>(b_arena).adj();
      results(a_adj, b_adj) = expressions(
          a_adj
              + elt_multiply(res.adj(),
                             (digamma(value_of(a_arena)) - digamma_ab)),
          b_adj
              + elt_multiply(res.adj(),
                             (digamma(value_of(b_arena)) - digamma_ab)));
    } else if (!is_constant<T_a>::value) {
      auto& a_adj = forward_as<var_value<matrix_cl<double>>>(a_arena).adj();
      a_adj = a_adj
              + elt_multiply(res.adj(),
                             (digamma(value_of(a_arena)) - digamma_ab));
    } else {
      auto& b_adj = forward_as<var_value<matrix_cl<double>>>(b_arena).adj();
      b_adj = b_adj
              + elt_multiply(res.adj(),
                             (digamma(value_of(b_arena)) - digamma_ab));
    }
  });
  return res;
}

}  // namespace math
}  // namespace stan

#endif
#endif
