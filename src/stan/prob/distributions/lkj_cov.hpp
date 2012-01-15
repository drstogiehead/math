#ifndef __STAN__PROB__DISTRIBUTIONS__LKJ_COV_HPP__
#define __STAN__PROB__DISTRIBUTIONS__LKJ_COV_HPP__

#include <stan/prob/traits.hpp>
#include <stan/prob/error_handling.hpp>
#include <stan/prob/constants.hpp>

#include <stan/prob/distributions/lognormal.hpp>
#include <stan/prob/distributions/lkj_corr.hpp>

namespace stan {
  namespace prob {
    using boost::math::tools::promote_args;
    using boost::math::policies::policy;

    using Eigen::Matrix;
    using Eigen::Dynamic;
    
    // LKJ_cov(y|mu,sigma,eta) [ y covariance matrix (not correlation matrix)
    //                         mu vector, sigma > 0 vector, eta > 0 ]
    template <bool propto = false,
	      typename T_y, typename T_loc, typename T_scale, typename T_shape, 
	      class Policy = policy<> >
    inline typename promote_args<T_y,T_loc,T_scale,T_shape>::type
    lkj_cov_log(const Matrix<T_y,Dynamic,Dynamic>& y,
		const Matrix<T_loc,Dynamic,1>& mu,
		const Matrix<T_scale,Dynamic,1>& sigma,
		const T_shape& eta,
		const Policy& = Policy()) {

      typename promote_args<T_y,T_loc,T_scale,T_shape>::type lp(0.0);
      if (!check_size_match(function, y.rows(), mu.size(), &lp, Policy()))
        return lp;
      if (!check_finite(function, mu, "Location parameter, mu", &lp, Policy()))
        return lp;
      
      const unsigned int K = y.rows();
      const Array<T_y,Dynamic,1> sds = y.diagonal().array().sqrt();
      for(unsigned int k = 0; k < K; k++) {
	lp += lognormal_log<propto>(log(sds(k,1)), mu(k,1), sigma(k,1));
      }
      if(eta == 1.0) {
	// no need to rescale y into a correlation matrix
	lp += lkj_corr_log<propto>(y,eta); 
	return log_prob;
      }
      DiagonalMatrix<double,Dynamic> D(K);
      D.diagonal() = sds.inverse();
      lp += lkj_corr_log<propto>(D * y * D, eta);
      return lp;
    }


    // LKJ_Cov(y|mu,sigma,eta) [ y covariance matrix (not correlation matrix)
    //                         mu scalar, sigma > 0 scalar, eta > 0 ]
    template <bool propto = false,
	      typename T_y, typename T_loc, typename T_scale, typename T_shape, 
	      class Policy = policy<> >
    inline typename promote_args<T_y,T_loc,T_scale,T_shape>::type
    lkj_cov_log(const Matrix<T_y,Dynamic,Dynamic>& y,
		const T_loc& mu, 
		const T_scale& sigma, 
		const T_shape& eta, 
		const Policy& = Policy()) {
      static const char* function = "stan::prob::multi_normal_log<%1%>(%1%)";

      typename promote_args<T_y,T_loc,T_scale,T_shape>::type lp(0.0);
      if (!check_finite(function, mu, "Location parameter, mu", &lp, Policy()))
        return lp;
      
      const unsigned int K = y.rows();
      const Array<T_y,Dynamic,1> sds = y.diagonal().array().sqrt();
      for(unsigned int k = 0; k < K; k++) {
	lp += lognormal_log<propto>(sds(k,1), mu, sigma);
      }
      if(eta == 1.0) {
	lp += lkj_corr_log<propto>(y,eta); // no need to rescale y into a correlation matrix
	return lp;
      }
      DiagonalMatrix<double,Dynamic> D(K);
      D.diagonal() = sds.inverse();
      lp += lkj_corr_log<propto>(D * y * D, eta);
      return lp;
    }


  }
}
#endif
