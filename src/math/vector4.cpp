/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/vector4.hpp>

namespace eXl
{
  template<> const Vector4<float> Vector4<float>::ZERO(0.0f, 0.0f, 0.0f, 0.0f);
  template<> const Vector4<float> Vector4<float>::UNIT_X(1.0f, 0.0f, 0.0f, 0.0f);
  template<> const Vector4<float> Vector4<float>::UNIT_Y(0.0f, 1.0f, 0.0f, 0.0f);
  template<> const Vector4<float> Vector4<float>::UNIT_Z(0.0f, 0.0f, 1.0f, 0.0f);
  template<> const Vector4<float> Vector4<float>::UNIT_W(0.0f, 0.0f, 0.0f, 1.0f);
  template<> const Vector4<float> Vector4<float>::ONE(1.0f, 1.0f, 1.0f ,1.0f);
  
  template<> const Vector4<double> Vector4<double>::ZERO(0.0, 0.0, 0.0, 0.0);
  template<> const Vector4<double> Vector4<double>::UNIT_X(1.0, 0.0, 0.0, 0.0);
  template<> const Vector4<double> Vector4<double>::UNIT_Y(0.0, 1.0, 0.0, 0.0);
  template<> const Vector4<double> Vector4<double>::UNIT_Z(0.0, 0.0, 1.0, 0.0);
  template<> const Vector4<double> Vector4<double>::UNIT_W(0.0, 0.0, 0.0, 1.0);
  template<> const Vector4<double> Vector4<double>::ONE(1.0, 1.0, 1.0, 1.0);
  
  std::ostream& operator <<(std::ostream& oStream,const Vector4f& tolog)
  {
    oStream<<"("<<tolog.X()<<","<<tolog.Y()<<tolog.Z()<<","<<tolog.W()<<")";
    return oStream;
  }


  std::ostream& operator <<(std::ostream& oStream,const Vector4d& tolog)
  {
    oStream<<"("<<tolog.X()<<","<<tolog.Y()<<tolog.Z()<<","<<tolog.W()<<")";
    return oStream;
  }
  
}