#ifndef PRINCIPALAXIS_INCLUDED
#define PRINCIPALAXIS_INCLUDED

#include "levelgen_exp.hpp"
#include <math/polygon.hpp>

namespace eXl
{
  class EXL_LEVELGEN_API ComputePrincipalAxis
  {
  public:

    bool operator()(Polygoni const& iPoly);
    bool operator()(Polygonf const& iPoly);
    bool operator()(Polygond const& iPoly);

    //Vector2d const& GetCenter() const {return m_Center; }
    Vector2d const& GetPrimaryAxis() const { return m_PrimaryAxis; }

  protected:
    template <typename Real>
    bool Compute(Polygon<Real> const& iPoly);

    //Vector2d m_Center;
    Vector2d m_PrimaryAxis;
  };
}

#endif