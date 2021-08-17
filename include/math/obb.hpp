#ifndef OBB_INCLUDED
#define OBB_INCLUDED

#include "levelgen_exp.hpp"
#include <math/aabb2d.hpp>
#include <math/polygon.hpp>

namespace eXl
{
  struct EXL_LEVELGEN_API OBB
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

#endif