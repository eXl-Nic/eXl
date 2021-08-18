/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/math.hpp>
#include <cfloat>
#include <climits>

namespace eXl
{
  //const QReal QReal::ZERO;

  //template <> const float Math<float>::PI = std::atan(1.0f)*4.0f;
  //template <> const double Math<double>::PI = std::atan(1.0)*4.0;
  //template <> const int Math<int>::PI = 3;
  //template <> const unsigned int Math<unsigned int>::PI = 3;
  //
  //template <> const float Math<float>::ZERO_TOLERANCE = 1e-05f;
  //template <> const float Math<float>::EPSILON = FLT_EPSILON;
  //template <> const float Math<float>::MAX_REAL = FLT_MAX;
  //
  //template <> const double Math<double>::ZERO_TOLERANCE = 1e-08f;
  //template <> const double Math<double>::EPSILON = DBL_EPSILON;
  //template <> const double Math<double>::MAX_REAL = DBL_MAX;
  //
  //template <> const int Math<int>::ZERO_TOLERANCE = 0;
  //template <> const int Math<int>::EPSILON = 0;
  //template <> const int Math<int>::MAX_REAL = INT_MAX;
  //
  //template <> const unsigned int Math<unsigned int>::ZERO_TOLERANCE = 0;
  //template <> const unsigned int Math<unsigned int>::EPSILON = 0;
  //template <> const unsigned int Math<unsigned int>::MAX_REAL = UINT_MAX;

  template <typename Real> const Real Math<Real>::PI = Math<Real>::GetPI();
  template <typename Real> const Real Math<Real>::ZERO_TOLERANCE = Math<Real>::GetZERO_TOLERANCE();
  template <typename Real> const Real Math<Real>::EPSILON = Math<Real>::GetEPSILON();
  template <typename Real> const Real Math<Real>::MAX_REAL = Math<Real>::GetMAX_REAL();

  template <> float Math<float>::GetPI()
  {
    return std::atan(1.0f)*4.0f;
  }

  template <> double Math<double>::GetPI()
  {
    return std::atan(1.0)*4.0;
  }

  template <> int Math<int>::GetPI()
  {
    return 3;
  }

  template <> unsigned int Math<unsigned int>::GetPI()
  {
    return 3;
  }

  template <> float Math<float>::GetEPSILON()
  {
    return FLT_EPSILON;
  }

  template <> double Math<double>::GetEPSILON()
  {
    return DBL_EPSILON;
  }

  template <> int Math<int>::GetEPSILON()
  {
    return 0;
  }

  template <> unsigned int Math<unsigned int>::GetEPSILON()
  {
    return 0;
  }

  template <> float Math<float>::GetZERO_TOLERANCE() 
  {
    return 1e-05f;
  }

  template <> double Math<double>::GetZERO_TOLERANCE()
  {
    return 1e-08f;
  }

  template <> int Math<int>::GetZERO_TOLERANCE()
  {
    return 0;
  }

  template <> unsigned int Math<unsigned int>::GetZERO_TOLERANCE()
  {
    return 0;
  }

  template <> float Math<float>::GetMAX_REAL() 
  {
    return FLT_MAX;
  }

  template <> double Math<double>::GetMAX_REAL() 
  {
    return DBL_MAX;
  }

  template <> int Math<int>::GetMAX_REAL()
  {
    return INT_MAX;
  }

  template <> unsigned int Math<unsigned int>::GetMAX_REAL()
  {
    return UINT_MAX;
  }

  template class Math<int>;
  template class Math<float>;
  template class Math<double>;
  template class Math<unsigned int>;

}