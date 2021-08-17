  
  template <typename Real>
  inline Vector2<Real>::Vector2(){m_Data[0]=0;m_Data[1]=0;}

  template <typename Real>
  inline Vector2<Real>::Vector2(Real x,Real y){m_Data[0]=x;m_Data[1]=y;}
  
  template <typename Real>
  inline Vector2<Real>::Vector2(const Real* afTuple)
  {
    m_Data[0]=afTuple[0];
    m_Data[1]=afTuple[1];
  }

  template <class Real>
  inline Vector2<Real>::Vector2(Vector2<Real> const& iOther)
  {
    m_Data[0] = iOther.X();
    m_Data[1] = iOther.Y();
    //memcpy(this, &iOther, sizeof(Vector2<Real>));
  }

  template <class Real>
  template <class OtherReal>
  inline Vector2<Real>::Vector2(Vector2<OtherReal> const& iOther)
  {
    m_Data[0] = iOther.X();
    m_Data[1] = iOther.Y();
  }
  
  template <typename Real>
  inline Vector2<Real>& Vector2<Real>::operator= (const Vector2& vec)
  {
    m_Data[0] = vec.m_Data[0];
    m_Data[1] = vec.m_Data[1];
    return *this;
  }

  template <typename Real>
  inline bool Vector2<Real>::operator== (const Vector2& vec) const
  {
    return m_Data[0] == vec.m_Data[0]
        && m_Data[1] == vec.m_Data[1];
  }

  template <typename Real>
  inline bool Vector2<Real>::operator!= (const Vector2& vec) const
  {
    return m_Data[0] != vec.m_Data[0]
        || m_Data[1] != vec.m_Data[1];
  }

  template <typename Real>
  inline bool Vector2<Real>::operator <(Vector2 const& iOther) const
  {
    if(m_Data[0] == iOther.m_Data[0])
    {
      return m_Data[1] < iOther.m_Data[1];
    }
    else
      return m_Data[0] < iOther.m_Data[0];
  }

  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::X () const
  {
    return m_Data[0];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real& Vector2<Real>::X ()
  {
    return m_Data[0];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::Y () const
  {
    return m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real& Vector2<Real>::Y ()
  {
    return m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real> Vector2<Real>::operator+ (const Vector2& vec) const
  {
    return Vector2
      (
       m_Data[0] + vec.m_Data[0],
       m_Data[1] + vec.m_Data[1]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real> Vector2<Real>::operator- (const Vector2& vec) const
  {
    return Vector2
      (
       m_Data[0] - vec.m_Data[0],
       m_Data[1] - vec.m_Data[1]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real> Vector2<Real>::operator* (Real scalar) const
  {
    return Vector2
      (
       scalar*m_Data[0],
       scalar*m_Data[1]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real> Vector2<Real>::operator/ (Real scalar) const
  {
    Vector2 result;

    if (scalar != (Real)0)
      {
        /*Real invScalar = ((Real)1)/scalar;*/
        result.m_Data[0] = /*invScalar**/m_Data[0] / scalar;
        result.m_Data[1] = /*invScalar**/m_Data[1] / scalar;
      }
    else
      {
        result.m_Data[0] = Math<Real>::MAX_REAL;
        result.m_Data[1] = Math<Real>::MAX_REAL;
      }

    return result;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real> Vector2<Real>::operator- () const
  {
    return Vector2
      (
       -m_Data[0],
       -m_Data[1]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real>& Vector2<Real>::operator+= (const Vector2& vec)
  {
    m_Data[0] += vec.m_Data[0];
    m_Data[1] += vec.m_Data[1];
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real>& Vector2<Real>::operator-= (const Vector2& vec)
  {
    m_Data[0] -= vec.m_Data[0];
    m_Data[1] -= vec.m_Data[1];
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real>& Vector2<Real>::operator*= (Real scalar)
  {
    m_Data[0] *= scalar;
    m_Data[1] *= scalar;
    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Vector2<Real>& Vector2<Real>::operator/= (Real scalar)
  {
    if (scalar != (Real)0)
      {
        //Real invScalar = ((Real)1)/scalar;
        m_Data[0] /= scalar;
        m_Data[1] /= scalar;
      }
    else
      {
        m_Data[0] *= Math<Real>::MAX_REAL;
        m_Data[1] *= Math<Real>::MAX_REAL;
      }

    return *this;
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::Length () const
  {
    return Math<Real>::Sqrt
      (
       m_Data[0]*m_Data[0] +
       m_Data[1]*m_Data[1]
       );
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::SquaredLength () const
  {
    return
      m_Data[0]*m_Data[0] +
      m_Data[1]*m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::Dot (const Vector2& vec) const
  {
    return
      m_Data[0]*vec.m_Data[0] +
      m_Data[1]*vec.m_Data[1];
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::Normalize ()
  {
    return Normalize(Math<Real>::ZERO_TOLERANCE);
  }
  //----------------------------------------------------------------------------
  template <typename Real>
  inline Real Vector2<Real>::Normalize (const Real epsilon)
  {
    Real length = Length();

    if (length > epsilon)
      {
        //Real invLength = ((Real)1)/length;
        m_Data[0] /= length;
        m_Data[1] /= length;
      }
    else
      {
        length = (Real)0;
        m_Data[0] = (Real)0;
        m_Data[1] = (Real)0;
      }

    return length;
  }

  template <typename Real>
  inline bool Vector2<Real>::Near(const Vector2& vec)
  {
    return Near(vec,Math<Real>::ZERO_TOLERANCE);
  }

  template <typename Real>
  inline bool Vector2<Real>::Near(const Vector2& vec,const Real epsilon)
  {
    return (*this - vec).Length() < epsilon;
  }


template <typename Real>
inline Vector2<Real> operator* (Real scalar, const Vector2<Real>& vec)
{
    return Vector2<Real>
    (
        scalar*vec.X(),
        scalar*vec.Y()
    );
}
