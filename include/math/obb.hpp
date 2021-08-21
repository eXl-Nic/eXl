/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <math/mathexp.hpp>
#include <math/aabb2d.hpp>
#include <math/polygon.hpp>

namespace eXl
{
  struct EXL_MATH_API OBB
  {
    AABB2Dd m_Dims;
    Vector2d m_Axis[2];
    inline OBB(){}
    template <typename Real>
    inline OBB(Vector2<Real> iDims, Vector2<Real> const& iCenter, Vector2d iPrimaryAxis, Vector2d iSecondaryAxis)
    {
      m_Axis[0] = iPrimaryAxis;
      m_Axis[1] = iSecondaryAxis;
      Vector2d offset(MathTools::ToDVec(iCenter).Dot(iPrimaryAxis), MathTools::ToDVec(iCenter).Dot(iSecondaryAxis));
      m_Dims = AABB2Dd::FromCenterAndSize(offset, MathTools::ToDVec(iDims));
    }

    OBB(Polygoni const& iPoly);

    Polygoni MakePoly();

  };
}
