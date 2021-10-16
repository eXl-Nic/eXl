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
  class Vector2
  {
  public :

    Vector2();

    Vector2(Real x,Real y);

    Vector2(const Real* afTuple);

    Vector2(Vector2<Real> const& iOther);

    template <class OtherReal>
    explicit Vector2(Vector2<OtherReal> const& iOther);

    inline Real X () const;
    inline Real& X ();
    inline Real Y () const;
    inline Real& Y ();

    inline Vector2& operator= (const Vector2& vec);
    inline bool operator== (const Vector2& vec) const;
    inline bool operator!= (const Vector2& vec) const;

    inline Vector2 operator+ (const Vector2& vec) const;
    inline Vector2 operator- (const Vector2& vec) const;
    inline Vector2 operator* (Real scalar) const;
    inline Vector2 operator/ (Real scalar) const;
    inline Vector2 operator- () const;

    inline Vector2& operator+= (const Vector2& vec);
    inline Vector2& operator-= (const Vector2& vec);
    inline Vector2& operator*= (Real scalar);
    inline Vector2& operator/= (Real scalar);

    inline bool operator <(Vector2 const& iOther) const;

    inline Real Length () const;
    inline Real SquaredLength () const;
    inline Real Dot (const Vector2& vec) const;
    inline Real Normalize ();
    inline Real Normalize (const Real epsilon);
    inline bool Near(const Vector2& vec);
    inline bool Near(const Vector2& vec, const Real epsilon);

    Real GetX() const{return m_Data[0];}
    Real GetY() const{return m_Data[1];}
    void SetX(Real iX) {m_Data[0]=iX;}
    void SetY(Real iY) {m_Data[1]=iY;}

    static const Vector2 ZERO;
    static const Vector2 UNIT_X;
    static const Vector2 UNIT_Y;
    static const Vector2 ONE;

  //protected:
    union
    {
      Real m_Data[2];
      struct
      {
        Real m_X;
        Real m_Y;
      };
    };

  };

  template <typename Real>
  inline Vector2<Real> operator* (Real scalar, const Vector2<Real>& vec);

  typedef Vector2<int> Vector2i; 
  typedef Vector2<float> Vector2f; 
  typedef Vector2<double> Vector2d;

  std::ostream& operator<<(std::ostream& oStream,const Vector2f& tolog);

  std::ostream& operator<<(std::ostream& oStream,const Vector2d& tolog);
}

#include "math.hpp"

namespace eXl
{
  template <typename Real>
  size_t hash_value(Vector2<Real> const& iVal)
  {
    size_t hash = 0;
    boost::hash_combine(hash, iVal.X());
    boost::hash_combine(hash, iVal.Y());
    return hash;
  }

  #include "vector2.inl"
}

