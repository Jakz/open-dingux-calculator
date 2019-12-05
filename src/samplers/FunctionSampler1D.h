#ifndef FUNCTION_SAMPLER_1D_H
#define FUNCTION_SAMPLER_1D_H

#define _USE_MATH_DEFINES
#include <functional>
#include <list>
#include <cmath>
#include <float.h>

/////////////////
// Description //
/////////////////
//
// This function samples a function in order to make its plot visually
// appealing. It does so by placing an upper bound on the normalized bend
// angle between adjacent plot segments, similar to what Mathematica does
// in its Plot[] function. For an example of its usage, see the test code.

// Version 2.0 - 2009-05-08 - Completely rewritten into simpler style
// Version 1.2 - 2009-05-07 - Fixed up FindQPeaks() to be more efficient
// Version 1.1 - 2009-04-24 - Added FindQPeaks()
// Version 1.0 - ????-??-?? - Original release

// Uncomment the following line to print out (lots!) of debugging
// output that shows the subdivision process.
//#define DEBUG_SAMPLER

#ifdef DEBUG_SAMPLER
# include <iostream>
#endif

namespace FunctionSampler1D {

  // An instance of the sampler is created with a certain set of
  // parameters:
  //   InitialPoints     The initial number of uniformly spaced points
  //                     to sample the function. This MUST be at least 3.
  //   RangeThreshold    The fraction of the maxmimum y-extent below
  //                     which subdividing should be avoided.
  //   MaxBend           When a consecutive triplet of samples are
  //                     fitted to a unit square, the maximum bend angle
  //                     between the two line segments that will be
  //                     tolerated before a subdivision will be
  //                     performed.
  //   MaxRecursion      The maximum level of bisection allowed.
  // Instantiation of a Params structure will provide suitable default
  // values.
  struct SampleFunctionParams {
    size_t InitialPoints;
    float RangeThreshold;
    float MaxBend;
    size_t MaxRecursion;
    SampleFunctionParams() :
      InitialPoints(25),
      RangeThreshold(0.005f),
      MaxBend(cos(20.0*M_PI / 180.0f)),
      MaxRecursion(20) {
    
    }

    void setMaxBend(float v)
    {
      MaxBend = cos(v);
    }
  };


  // Input requirements:
  //   FunctionType:
  //   + Must implement std::unary_function
  //   + Must implement
  //     std::unary_function::result_type operator()(real_t) [const] { ... }
  //   + The std::unary_function::result_type must have define
  //     operator real_t() const{ ... } to return the function value
  // Output properties:
  //   + The output value list is in sorted ascending order by the x-coordinate
  //     (first value in pair)
  template <class FunctionType, typename real_t>
  void SampleFunction(
    FunctionType f,
    real_t x0, real_t x1,
    const SampleFunctionParams &params,
    std::list<std::pair<real_t, typename FunctionType::result_type> > &values
  ) {
    typedef typename FunctionType::result_type function_result;
    typedef typename std::pair<real_t, function_result> list_element_t;
    typedef typename std::list<list_element_t> value_list_t;
    typedef typename value_list_t::iterator value_list_iterator_t;
    values.erase(values.begin(), values.end());

    real_t dx = (x1 - x0) / (real_t)(params.InitialPoints - 1);
    real_t y_min = DBL_MAX, y_max = -DBL_MAX;
    for (size_t j = 0; j < params.InitialPoints; ++j) {
      real_t x_new = x0 + (real_t)j*dx;
      function_result p = f(x_new);
      real_t pval = static_cast<real_t>(p);
      if (pval < y_min) { y_min = pval; }
      if (pval > y_max) { y_max = pval; }
      values.push_back(list_element_t(x_new, p));
    }

    value_list_iterator_t iend = values.end(); iend--;
    value_list_iterator_t istart = values.begin(); istart++;
    value_list_iterator_t i;

    i = istart;
    real_t min_dx = dx * pow(0.5, (int)params.MaxRecursion);
    while (i != iend) {
      value_list_iterator_t i_prev = i; i_prev--;
      value_list_iterator_t i_next = i; i_next++;
      real_t yp, y0, yn;
      real_t xp, x0, xn;
      yp = real_t(i_prev->second); xp = i_prev->first;
      y0 = real_t(i->second); x0 = i->first;
      yn = real_t(i_next->second); xn = i_next->first;

#if defined(DEBUG_SAMPLER)
      std::cerr << "* x0=" << x0 << ", y0=" << y0 << std::endl;
      std::cerr << "*  xp=" << xp << ", xn=" << xn << std::endl;
      std::cerr << "*  yp=" << yp << ", yn=" << yn << std::endl;
#endif

      if (((xn - x0) < min_dx) && ((x0 - xp) < min_dx)) {
#if defined(DEBUG_SAMPLER)
        std::cerr << "*  MaxRecursion reached: "
          << (xn - x0) << ", " << (x0 - xp)
          << " < " << min_dx << std::endl;
#endif
        ++i;
        continue;
      }

      // Fit to square
      real_t local_y_max = std::max(yp, y0);
      local_y_max = std::max(local_y_max, yn);
      real_t local_y_min = std::min(yp, y0);
      local_y_min = std::min(local_y_min, yn);
      real_t local_x_max = std::max(xp, x0);
      local_x_max = std::max(local_x_max, xn);
      real_t local_x_min = std::min(xp, x0);
      local_x_min = std::min(local_x_min, xn);
      real_t dx0 = (x0 - xp) / (local_x_max - local_x_min);
      real_t dx1 = (xn - x0) / (local_x_max - local_x_min);
      real_t dy0 = (y0 - yp) / (local_y_max - local_y_min);
      real_t dy1 = (yn - y0) / (local_y_max - local_y_min);
      // Find the cosine of the angle between segments
      // 0->p and p->n using dot product
      real_t cosq = (dx0*dx1 + dy0 * dy1)
        / std::sqrt((dx0*dx0 + dy0 * dy0) * (dx1*dx1 + dy1 * dy1));

      // If the function is sufficiently "flat" (small y-variation)
      // at this point, then don't bother refining it.
      if ((std::abs(y0 - yp) < params.RangeThreshold * (y_max - y_min)) &&
        (std::abs(yn - y0) < params.RangeThreshold * (y_max - y_min))) {
#if defined(DEBUG_SAMPLER)
        std::cerr << "*  Flat enough" << std::endl;
#endif
        ++i;
        continue;
      }

#if defined(DEBUG_SAMPLER)
      std::cerr << "*  cosq=" << cosq << std::endl;
#endif
      if ((xn - x0) < FLT_EPSILON || (x0 - xp) < FLT_EPSILON) {
#if defined(DEBUG_SAMPLER)
        std::cerr << "*  Resolution too fine" << std::endl;
#endif
        ++i;
        continue;
      }

      // Perform a subdivision if the angular bend exceeds the limit
      // OR if the subdivisional resolution difference is greater than 3:1
      // In other words, the resolution cannot change by more than one
      // level at a time
      if ((cosq < params.MaxBend) || (dx1 > 3 * dx0) || (dx0 > 3 * dx1)) {
        if (x0 - xp > xn - x0) {
          // Add point before the current iterator
          real_t x_new = 0.5*(xp + x0);
#if defined(DEBUG_SAMPLER)
          std::cerr << "*  Inserting before at x=" << x_new << std::endl;
#endif
          function_result p = f(x_new);
          real_t pval = static_cast<real_t>(p);
          if (p < y_min) { y_min = p; }
          if (p > y_max) { y_max = p; }
          values.insert(i, list_element_t(x_new, p));
          --i;
          continue;
        }
        else {
          // Add point after the current iterator
          real_t x_new = 0.5*(x0 + xn);
#if defined(DEBUG_SAMPLER)
          std::cerr << "*  Inserting after at x=" << x_new << std::endl;
#endif
          function_result p = f(x_new);
          real_t pval = static_cast<real_t>(p);
          if (p < y_min) { y_min = p; }
          if (p > y_max) { y_max = p; }
          values.insert(i_next, list_element_t(x_new, p));
          continue;
        }
      }
      ++i;
      continue;
    }
  }

}; // end namespace FunctionSampler1D

template<typename real_t>
class FunctionSampler
{
public:
  using values_list_t = std::list<std::pair<real_t, real_t>>;
private:
  std::function<real_t(real_t)> function;


public:

  values_list_t divide(size_t depth, real_t epsilon, real_t a, real_t c, values_list_t& values, typename values_list_t::iterator it)
  {
    const real_t b = (a + c) / 2;
    real_t x[] = { a, (a + b) / 2, b, (b + c) / 2, c }, y[5];
    for (size_t i = 0; i < 5; ++i) y[i] = function(x[i]);

    size_t badness = 0;
    for (size_t i = 0; i <= 2; ++i)
    {
      if (y[i + 1] > y[i] && y[i + 1] > y[i + 2])
        ++badness;
      else if (y[i + 1] < y[i] && y[i + 1] < y[i + 2])
        --badness;
    }

    for (size_t i = 0; i < 5; ++i)
      if (isfinite(y[i]))
        ++badness;

    if (badness > 2)
    {
      auto firstHalf = divide(depth - 1, epsilon * 2, a, b, values, it);
      auto secondHalf = divide(depth - 1, epsilon * 2, b, c, values, it);
      values.insert(it, std::next(secondHalf.begin()), secondHalf.end());
      values.insert(it, firstHalf.begin(), firstHalf.end());
    }

    return values_list_t();
  }
  


};

template class FunctionSampler<float>;

#endif // FUNCTION_SAMPLER_1D_H
