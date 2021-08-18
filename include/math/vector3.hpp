/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <iostream>

namespace eXl
{
  template <class Real>
  class Vector2;

  template <class Real>
  class Vector3
  {
  public :

    Vector3(Real x=(0.0),Real y=(0.0), Real z=(0.0));

    Vector3(const Real* afTuple);

    Vector3(Vector3<Real> const& iOther);

    explicit Vector3(Vector2<Real> const& iOther);

    template <class OtherReal>
    explicit Vector3(Vector3<OtherReal> const& iOther);

    inline Real X () const;
    inline Real& X ();
    inline Real Y () const;
    inline Real& Y ();
    inline Real Z () const;
    inline Real& Z ();

    inline Vector3& operator= (const Vector3& vec);
    inline bool operator== (const Vector3& vec) const;
    inline bool operator!= (const Vector3& vec) const;

    inline Vector3 operator+ (const Vector3& vec) const;
    inline Vector3 operator- (const Vector3& vec) const;
    inline Vector3 operator* (Real scalar) const;
    inline Vector3 operator/ (Real scalar) const;
    inline Vector3 operator- () const;

    inline Vector3& operator+= (const Vector3& vec);
    inline Vector3& operator-= (const Vector3& vec);
    inline Vector3& operator*= (Real scalar);
    inline Vector3& operator/= (Real scalar);

    inline Real Length () const;
    inline Real SquaredLength () const;
    inline Real Dot (const Vector3& vec) const;
    inline Real Normalize ();
    inline Real Normalize (const Real epsilon);

    Vector3 Cross (const Vector3& vec) const;
    Vector3 UnitCross (const Vector3& vec) const;

    Real GetX() const{return m_X;}
    Real GetY() const{return m_Y;}
    Real GetZ() const{return m_Z;}
    void SetX(Real iX) {m_X=iX;}
    void SetY(Real iY) {m_Y=iY;}
    void SetZ(Real iZ) {m_Z=iZ;}

    static const Vector3 ZERO;
    static const Vector3 UNIT_X;
    static const Vector3 UNIT_Y;
    static const Vector3 UNIT_Z;
    static const Vector3 ONE;

  //protected:
    union
    {
      Real m_Data[3];
      struct
      {
        Real m_X;
        Real m_Y;
        Real m_Z;
      };
    };

  };

  template <typename Real>
  inline Vector3<Real> operator* (Real scalar, const Vector3<Real>& vec);

  typedef Vector3<int> Vector3i;
  typedef Vector3<float> Vector3f; 
  typedef Vector3<double> Vector3d;
  
  std::ostream& operator<<(std::ostream& oStream,const Vector3f& tolog);

  std::ostream& operator<<(std::ostream& oStream,const Vector3d& tolog);
}

#include "math.hpp"

namespace eXl
{
  template <typename Real>
  size_t hash_value(Vector3<Real> const& iVal)
  {
    size_t hash = 0;
    boost::hash_combine(hash, iVal.X());
    boost::hash_combine(hash, iVal.Y());
    boost::hash_combine(hash, iVal.Z());
    return hash;
  }

  #include "vector3.inl"
}
