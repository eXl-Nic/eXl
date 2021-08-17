/**
  Geometric Tools, LLC
  Copyright (c) 1998-2010
  Distributed under the Boost Software License, Version 1.0.
  http://www.boost.org/LICENSE_1_0.txt
  http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
  
  File Version: 5.0.0 (2010/01/01)
*/

#pragma once

#include "mathexp.hpp"
#include <iostream>

namespace eXl
{
  template <typename Real>
  class Vector3;

  template <typename Real>
  class Quaternion
  {
  public:
    Quaternion (Real w=1.0, Real x=0.0, Real y=0.0, Real z=0.0);
    Quaternion (const Quaternion& q);

    Quaternion (const Vector3<Real>& axis, Real angle);

    Quaternion (const Vector3<Real> rotColumn[3]);

    inline operator const Real* () const;
    inline operator Real* ();
    inline Real operator[] (int i) const;
    inline Real& operator[] (int i);
    inline Real W () const;
    inline Real& W ();
    inline Real X () const;
    inline Real& X ();
    inline Real Y () const;
    inline Real& Y ();
    inline Real Z () const;
    inline Real& Z ();

    inline Quaternion& operator= (const Quaternion& q);

    inline bool operator== (const Quaternion& q) const;
    inline bool operator!= (const Quaternion& q) const;
    inline bool operator<  (const Quaternion& q) const;
    inline bool operator<= (const Quaternion& q) const;
    inline bool operator>  (const Quaternion& q) const;
    inline bool operator>= (const Quaternion& q) const;

    inline Quaternion operator+ (const Quaternion& q) const;
    inline Quaternion operator- (const Quaternion& q) const;
    inline Quaternion operator* (const Quaternion& q) const;
    inline Quaternion operator* (Real scalar) const;
    inline Quaternion operator/ (Real scalar) const;
    inline Quaternion operator- () const;

    friend Quaternion<Real> operator* (Real scalar,const Quaternion<Real>& q)
    {
        return q*scalar;
    }

    inline Quaternion& operator+= (const Quaternion& q);
    inline Quaternion& operator-= (const Quaternion& q);
    inline Quaternion& operator*= (Real scalar);
    inline Quaternion& operator/= (Real scalar);

    void FromRotationMatrix (const Vector3<Real> rotColumn[3]);
    void ToRotationMatrix (Vector3<Real> rotColumn[3]) const;
    void FromAxisAngle (const Vector3<Real>& axis, Real angle);
    void ToAxisAngle (Vector3<Real>& axis, Real& angle) const;

    inline Real Length () const;
    inline Real SquaredLength () const;
    inline Real Dot (const Quaternion& q) const;
    inline Real Normalize ();
    inline Real Normalize (Real epsilon);
    Quaternion Inverse () const;
    Quaternion Conjugate () const;
    Quaternion Exp () const;
    Quaternion Log () const;

    Vector3<Real> Rotate (const Vector3<Real>& vec) const;

    Quaternion& Slerp (Real t, const Quaternion& p, const Quaternion& q);
    Quaternion& SlerpExtraSpins (Real t, const Quaternion& p,
        const Quaternion& q, int extraSpins);

    Quaternion& Intermediate (const Quaternion& q0, const Quaternion& q1,
        const Quaternion& q2);

    Quaternion& Squad (Real t, const Quaternion& q0, const Quaternion& a0,
        const Quaternion& a1, const Quaternion& q1);

    Quaternion& Align (const Vector3<Real>& v1, const Vector3<Real>& v2);

    void DecomposeTwistTimesSwing (const Vector3<Real>& v1,
        Quaternion& twist, Quaternion& swing);

    void DecomposeSwingTimesTwist (const Vector3<Real>& v1,
        Quaternion& swing, Quaternion& twist);

    Quaternion GetClosestX () const;
    Quaternion GetClosestY () const;
    Quaternion GetClosestZ () const;
    Quaternion GetClosestXY () const;
    Quaternion GetClosestYX () const;
    Quaternion GetClosestZX () const;
    Quaternion GetClosestXZ () const;
    Quaternion GetClosestYZ () const;
    Quaternion GetClosestZY () const;

    void FactorXYZ (Real& cx, Real& sx, Real& cy, Real& sy, Real& cz,
        Real& sz);

    void FactorXZY (Real& cx, Real& sx, Real& cz, Real& sz, Real& cy,
        Real& sy);

    void FactorYZX (Real& cy, Real& sy, Real& cz, Real& sz, Real& cx,
        Real& sx);

    void FactorYXZ (Real& cy, Real& sy, Real& cx, Real& sx, Real& cz,
        Real& sz);

    void FactorZXY (Real& cz, Real& sz, Real& cx, Real& sx, Real& cy,
        Real& sy);

    void FactorZYX (Real& cz, Real& sz, Real& cy, Real& sy, Real& cx,
        Real& sx);

    // Special quaternions.
    static const Quaternion ZERO;
    static const Quaternion IDENTITY;

    union
    {
      Real mTuple[4];
      struct
      {
        Real m_W;
        Real m_X;
        Real m_Y;
        Real m_Z;
      };
    };

private:
    Quaternion GetClosest (int axis) const;
  };

  typedef Quaternion<float> Quaternionf;
  typedef Quaternion<double> Quaterniond;

  EXL_MATH_API std::ostream& operator<<(std::ostream& oStream,const Quaternionf& tolog);
}

#include "math.hpp"

namespace eXl
{

  template <typename Real>
  size_t hash_value(Quaternion<Real> const& iVal)
  {
    size_t hash = 0;
    boost::hash_combine(hash, iVal.X());
    boost::hash_combine(hash, iVal.Y());
    boost::hash_combine(hash, iVal.Z());
    boost::hash_combine(hash, iVal.W());
    return hash;
  }

  #include "quaternion.inl"
}

