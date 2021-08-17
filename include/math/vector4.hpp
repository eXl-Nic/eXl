/**
  Geometric Tools, LLC
  Copyright (c) 1998-2010
  Distributed under the Boost Software License, Version 1.0.
  http://www.boost.org/LICENSE_1_0.txt
  http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
  
  File Version: 5.0.0 (2010/01/01)

*/

#pragma once

#include <iostream>

namespace eXl
{

  template <typename Real>
  class Vector4
  {
  public:
      
    Vector4 ();
    Vector4 (const Vector4& vec);      
    Vector4 (Real x, Real y, Real z, Real w);

    Vector4& operator= (const Vector4& vec);

    inline Real X () const;
    inline Real& X ();
    inline Real Y () const;
    inline Real& Y ();
    inline Real Z () const;
    inline Real& Z ();
    inline Real W () const;
    inline Real& W ();

    inline Vector4 operator+ (const Vector4& vec) const;
    inline Vector4 operator- (const Vector4& vec) const;
    inline Vector4 operator* (Real scalar) const;
    inline Vector4 operator/ (Real scalar) const;
    inline Vector4 operator- () const;

    inline Vector4& operator+= (const Vector4& vec);
    inline Vector4& operator-= (const Vector4& vec);
    inline Vector4& operator*= (Real scalar);
    inline Vector4& operator/= (Real scalar);

    inline Real Length () const;
    inline Real SquaredLength () const;
    inline Real Dot (const Vector4& vec) const;
    inline Real Normalize ();
    inline Real Normalize (const Real epsilon);

    static void ComputeExtremes (int numVectors, const Vector4* vectors,
        Vector4& vmin, Vector4& vmax);

    inline bool operator <(Vector4 const& iOther) const;

    static const Vector4 ZERO;
    static const Vector4 UNIT_X;  // (1,0,0,0)
    static const Vector4 UNIT_Y;  // (0,1,0,0)
    static const Vector4 UNIT_Z;  // (0,0,1,0)
    static const Vector4 UNIT_W;  // (0,0,0,1)
    static const Vector4 ONE;     // (1,1,1,1)

  //protected:
    union
    {
      Real m_Data[4];
      struct
      {
        Real m_X;
        Real m_Y;
        Real m_Z;
        Real m_W;
      };
    };
  };

  template <typename Real>
  inline Vector4<Real> operator* (Real scalar, const Vector4<Real>& vec);

  typedef Vector4<int> Vector4i;
  typedef Vector4<float> Vector4f;
  typedef Vector4<double> Vector4d;
  
  std::ostream& operator<<(std::ostream& oStream,const Vector4f& tolog);
  std::ostream& operator<<(std::ostream& oStream,const Vector4d& tolog);

}

#include "math.hpp"

namespace eXl
{
  #include "vector4.inl"
}
