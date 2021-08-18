/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace eXl
{
  template <typename Real>
  class Vector3;

  template <typename Real>
  class Vector4;

  template <typename Real>
  class Quaternion;

  template <typename Real>
  class Matrix4
  {
  public:
      
    Matrix4 ();
      
    Matrix4 (bool makeZero);

    Matrix4 (const Matrix4& mat);

    Matrix4 (
        Real m00, Real m01, Real m02, Real m03,
        Real m10, Real m11, Real m12, Real m13,
        Real m20, Real m21, Real m22, Real m23,
        Real m30, Real m31, Real m32, Real m33);

    Matrix4 (const Real entry[16]);

    static Matrix4<Real> FromPosition(Vector3<Real> const&);

    static Matrix4<Real> FromPositionAndOrientation(Vector3<Real> const&, Quaternion<Real> const& iOrient);

    Matrix4& operator= (const Matrix4& mat);

    void MakeZero ();
    void MakeIdentity ();

    Matrix4 operator+ (const Matrix4& mat) const;
    Matrix4 operator- (const Matrix4& mat) const;
    Matrix4 operator* (Real scalar) const;
    Matrix4 operator/ (Real scalar) const;
    Matrix4 operator- () const;

    Matrix4& operator+= (const Matrix4& mat);
    Matrix4& operator-= (const Matrix4& mat);
    Matrix4& operator*= (Real scalar);
    Matrix4& operator/= (Real scalar);

    // M*vec
    Vector4<Real> operator* (const Vector4<Real>& vec) const;

    //// u^T*M*v
    //Real QForm (const Vector4<Real>& u, const Vector4<Real>& v) const;

    Matrix4 Transpose () const;

    Matrix4 operator* (const Matrix4& mat) const;

    Matrix4 TransposeTimes (const Matrix4& mat) const;

    Matrix4 TimesTranspose (const Matrix4& mat) const;

    Matrix4 TransposeTimesTranspose (const Matrix4& mat) const;

    Matrix4 Inverse (const Real epsilon = (Real)0) const;
    Matrix4 Adjoint () const;
    Real Determinant () const;

    void MakeObliqueProjection (const Vector3<Real>& normal,
        const Vector3<Real>& origin, const Vector3<Real>& direction);

    void MakePerspectiveProjection (const Vector3<Real>& normal,
        const Vector3<Real>& origin, const Vector3<Real>& eye);

    void MakeReflection (const Vector3<Real>& normal,
        const Vector3<Real>& origin);

    static const Matrix4 ZERO;
    static const Matrix4 IDENTITY;

  //protected:
    union
    {
      Real m_Data[16];
      Real m_Matrix[4][4];
    };
  };

  template <typename Real>
  inline Matrix4<Real> operator* (Real scalar, const Matrix4<Real>& mat);
  
  template <typename Real>
  inline Vector4<Real> operator* (const Vector4<Real>& vec,
    const Matrix4<Real>& mat);
  
  typedef Matrix4<float> Matrix4f;
  typedef Matrix4<double> Matrix4d;
}

#include "math.hpp"

namespace eXl
{
  #include "matrix4.inl"
}
