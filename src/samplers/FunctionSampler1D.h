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
// #define DEBUG_SAMPLER

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
    double RangeThreshold;
    double MaxBend;
    size_t MaxRecursion;
    SampleFunctionParams() :
      InitialPoints(25),
      RangeThreshold(0.005),
      MaxBend(10.0*M_PI / 180.0),
      MaxRecursion(20) {}
  };


  // Input requirements:
  //   FunctionType:
  //   + Must implement std::unary_function
  //   + Must implement
  //     std::unary_function::result_type operator()(double) [const] { ... }
  //   + The std::unary_function::result_type must have define
  //     operator double() const{ ... } to return the function value
  // Output properties:
  //   + The output value list is in sorted ascending order by the x-coordinate
  //     (first value in pair)
  template <class FunctionType>
  void SampleFunction(
    FunctionType f,
    double x0, double x1,
    const SampleFunctionParams &params,
    std::list<std::pair<double, typename FunctionType::result_type> > &values
  ) {
    typedef typename FunctionType::result_type function_result;
    typedef typename std::pair<double, function_result> list_element_t;
    typedef typename std::list<list_element_t> value_list_t;
    typedef typename value_list_t::iterator value_list_iterator_t;
    values.erase(values.begin(), values.end());

    double dx = (x1 - x0) / (double)(params.InitialPoints - 1);
    double y_min = DBL_MAX, y_max = -DBL_MAX;
    for (size_t j = 0; j < params.InitialPoints; ++j) {
      double x_new = x0 + (double)j*dx;
      function_result p = f(x_new);
      double pval = static_cast<double>(p);
      if (pval < y_min) { y_min = pval; }
      if (pval > y_max) { y_max = pval; }
      values.push_back(list_element_t(x_new, p));
    }

    value_list_iterator_t iend = values.end(); iend--;
    value_list_iterator_t istart = values.begin(); istart++;
    value_list_iterator_t i;

    i = istart;
    double min_dx = dx * pow(0.5, (int)params.MaxRecursion);
    while (i != iend) {
      value_list_iterator_t i_prev = i; i_prev--;
      value_list_iterator_t i_next = i; i_next++;
      double yp, y0, yn;
      double xp, x0, xn;
      yp = double(i_prev->second); xp = i_prev->first;
      y0 = double(i->second); x0 = i->first;
      yn = double(i_next->second); xn = i_next->first;

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
      double local_y_max = std::max(yp, y0);
      local_y_max = std::max(local_y_max, yn);
      double local_y_min = std::min(yp, y0);
      local_y_min = std::min(local_y_min, yn);
      double local_x_max = std::max(xp, x0);
      local_x_max = std::max(local_x_max, xn);
      double local_x_min = std::min(xp, x0);
      local_x_min = std::min(local_x_min, xn);
      double dx0 = (x0 - xp) / (local_x_max - local_x_min);
      double dx1 = (xn - x0) / (local_x_max - local_x_min);
      double dy0 = (y0 - yp) / (local_y_max - local_y_min);
      double dy1 = (yn - y0) / (local_y_max - local_y_min);
      // Find the cosine of the angle between segments
      // 0->p and p->n using dot product
      double cosq = (dx0*dx1 + dy0 * dy1)
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
      if ((xn - x0) < DBL_EPSILON || (x0 - xp) < DBL_EPSILON) {
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
      if ((cosq < cos(params.MaxBend)) || (dx1 > 3 * dx0) || (dx0 > 3 * dx1)) {
        if (x0 - xp > xn - x0) {
          // Add point before the current iterator
          double x_new = 0.5*(xp + x0);
#if defined(DEBUG_SAMPLER)
          std::cerr << "*  Inserting before at x=" << x_new << std::endl;
#endif
          function_result p = f(x_new);
          double pval = static_cast<double>(p);
          if (p < y_min) { y_min = p; }
          if (p > y_max) { y_max = p; }
          values.insert(i, list_element_t(x_new, p));
          --i;
          continue;
        }
        else {
          // Add point after the current iterator
          double x_new = 0.5*(x0 + xn);
#if defined(DEBUG_SAMPLER)
          std::cerr << "*  Inserting after at x=" << x_new << std::endl;
#endif
          function_result p = f(x_new);
          double pval = static_cast<double>(p);
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

#endif // FUNCTION_SAMPLER_1D_H