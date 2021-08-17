  template <typename Real>
  Vector3<Real>::Vector3(Real x,Real y, Real z){m_Data[0]=x;m_Data[1]=y;m_Data[2]=z;}
  
  template <typename Real>
  Vector3<Real>::Vector3(const Real* afTuple)
  {
    m_Data[0]=afTuple[0];
    m_Data[1]=afTuple[1];
    m_Data[2]=afTuple[2];
  }

  template <typename Real>
  Vector3<Real>::Vector3(Vector2<Real> const& iOther)
  {
    m_Data[0]=iOther.X();
    m_Data[1]=iOther.Y();
    m_Data[2]=0.0;
  }

  template <class Real>
  Vector3<Real>::Vector3(Vector3<Real> const& iOther)
  {
    m_Data[0] = iOther.X();
    m_Data[1] = iOther.Y();
    m_Data[2] = iOther.Z();
  }

  template <class Real>
  template <class OtherReal>
  Vector3<Real>::Vector3(Vector3<OtherReal> const& iOther)
  {
    m_Data[0] = iOther.X();
    m_Data[1] = iOther.Y();
    m_Data[2] = iOther.Z();
  }

  template <typename Real>
  Vector3<Real>& Vector3<Real>::operator= (const Vector3& vec)
  {
    m_Data[0] = vec.m_Data[0];
    m_Data[1] = vec.m_Data[1];
    m_Data[2] = vec.m_Data[2];
    return *this;
  }

  template <typename Real>
  inline bool Vector3<Real>::operator== (const Vector3& vec) const
  {
    return m_Data[0] == vec.m_Data[0]
        && m_Data[1] == vec.m_Data[1]
        && m_Data[2] == vec.m_Data[2];
  }

  template <typename Real>
  inline bool Vector3<Real>::operator!= (const Vector3& vec) const
  {
    return m_Data[0] != vec.m_Data[0]
        || m_Data[1] != vec.m_Data[1]
        || m_Data[2] != vec.m_Data[2];
  }

  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::X () const
  {
    return m_Data[0];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real& Vector3<Real>::X ()
  {
    return m_Data[0];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Y () const
  {
    return m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real& Vector3<Real>::Y ()
  {
    return m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Z () const
  {
    return m_Data[2];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real& Vector3<Real>::Z ()
  {
    return m_Data[2];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real> Vector3<Real>::operator+ (const Vector3& vec) const
  {
    return Vector3
      (
       m_Data[0] + vec.m_Data[0],
       m_Data[1] + vec.m_Data[1],
       m_Data[2] + vec.m_Data[2]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real> Vector3<Real>::operator- (const Vector3& vec) const
  {
    return Vector3
      (
       m_Data[0] - vec.m_Data[0],
       m_Data[1] - vec.m_Data[1],
       m_Data[2] - vec.m_Data[2]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real> Vector3<Real>::operator* (Real scalar) const
  {
    return Vector3
      (
       scalar*m_Data[0],
       scalar*m_Data[1],
       scalar*m_Data[2]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real> Vector3<Real>::operator/ (Real scalar) const
  {
    Vector3 result;

    if (scalar != (Real)0)
      {
        Real invScalar = ((Real)1)/scalar;
        result.m_Data[0] = invScalar*m_Data[0];
        result.m_Data[1] = invScalar*m_Data[1];
        result.m_Data[2] = invScalar*m_Data[2];
      }
    else
      {
        result.m_Data[0] = Math<Real>::MAX_REAL;
        result.m_Data[1] = Math<Real>::MAX_REAL;
        result.m_Data[2] = Math<Real>::MAX_REAL;
      }

    return result;
  }

  template <>
  inline Vector3<int> Vector3<int>::operator/ (int scalar) const
  {
    Vector3 result;

    if (scalar != 0)
    {
      result.m_Data[0] = m_Data[0] / scalar;
      result.m_Data[1] = m_Data[1] / scalar;
      result.m_Data[2] = m_Data[2] / scalar;
    }
    else
    {
      result.m_Data[0] = Math<int>::MAX_REAL;
      result.m_Data[1] = Math<int>::MAX_REAL;
      result.m_Data[2] = Math<int>::MAX_REAL;
    }

    return result;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real> Vector3<Real>::operator- () const
  {
    return Vector3
      (
       -m_Data[0],
       -m_Data[1],
       -m_Data[2]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real>& Vector3<Real>::operator+= (const Vector3& vec)
  {
    m_Data[0] += vec.m_Data[0];
    m_Data[1] += vec.m_Data[1];
    m_Data[2] += vec.m_Data[2];
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real>& Vector3<Real>::operator-= (const Vector3& vec)
  {
    m_Data[0] -= vec.m_Data[0];
    m_Data[1] -= vec.m_Data[1];
    m_Data[2] -= vec.m_Data[2];
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real>& Vector3<Real>::operator*= (Real scalar)
  {
    m_Data[0] *= scalar;
    m_Data[1] *= scalar;
    m_Data[2] *= scalar;
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector3<Real>& Vector3<Real>::operator/= (Real scalar)
  {
    if (scalar != (Real)0)
      {
        Real invScalar = ((Real)1)/scalar;
        m_Data[0] *= invScalar;
        m_Data[1] *= invScalar;
        m_Data[2] *= invScalar;
      }
    else
      {
        m_Data[0] *= Math<Real>::MAX_REAL;
        m_Data[1] *= Math<Real>::MAX_REAL;
        m_Data[2] *= Math<Real>::MAX_REAL;
      }

    return *this;
  }

  template <>
  inline Vector3<int>& Vector3<int>::operator/= (int scalar)
  {
    if (scalar != 0)
    {
      m_Data[0] /= scalar;
      m_Data[1] /= scalar;
      m_Data[2] /= scalar;
    }
    else
    {
      m_Data[0] *= Math<int>::MAX_REAL;
      m_Data[1] *= Math<int>::MAX_REAL;
      m_Data[2] *= Math<int>::MAX_REAL;
    }

    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Length () const
  {
    return Math<Real>::Sqrt
      (
       m_Data[0]*m_Data[0] +
       m_Data[1]*m_Data[1] +
       m_Data[2]*m_Data[2]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::SquaredLength () const
  {
    return
      m_Data[0]*m_Data[0] +
      m_Data[1]*m_Data[1] +
      m_Data[2]*m_Data[2];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Dot (const Vector3& vec) const
  {
    return
      m_Data[0]*vec.m_Data[0] +
      m_Data[1]*vec.m_Data[1] +
      m_Data[2]*vec.m_Data[2];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Normalize ()
  {
    return Normalize(Math<Real>::ZERO_TOLERANCE);
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector3<Real>::Normalize (const Real epsilon)
  {
    Real length = Length();

    if (length > epsilon)
      {
        Real invLength = ((Real)1)/length;
        m_Data[0] *= invLength;
        m_Data[1] *= invLength;
        m_Data[2] *= invLength;
      }
    else
      {
        length = (Real)0;
        m_Data[0] = (Real)0;
        m_Data[1] = (Real)0;
        m_Data[2] = (Real)0;
      }

    return length;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  Vector3<Real> Vector3<Real>::Cross (const Vector3& vec) const
  {
    return Vector3
      (
       m_Data[1]*vec.m_Data[2] - m_Data[2]*vec.m_Data[1],
       m_Data[2]*vec.m_Data[0] - m_Data[0]*vec.m_Data[2],
       m_Data[0]*vec.m_Data[1] - m_Data[1]*vec.m_Data[0]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  Vector3<Real> Vector3<Real>::UnitCross (const Vector3& vec) const
  {
    Vector3 cross
      (
       m_Data[1]*vec.m_Data[2] - m_Data[2]*vec.m_Data[1],
       m_Data[2]*vec.m_Data[0] - m_Data[0]*vec.m_Data[2],
       m_Data[0]*vec.m_Data[1] - m_Data[1]*vec.m_Data[0]
       );
    cross.Normalize();
    return cross;
  }

template <typename Real>
inline Vector3<Real> operator* (Real scalar, const Vector3<Real>& vec)
{
    return Vector3<Real>
    (
        scalar*vec.X(),
        scalar*vec.Y(),
        scalar*vec.Z()
    );
}