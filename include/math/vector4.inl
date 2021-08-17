
template <typename Real>
Vector4<Real>::Vector4 ()
{
    // Uninitialized for performance in array construction.
}

template <typename Real>
Vector4<Real>::Vector4 (const Vector4& vec)
{
    m_Data[0] = vec.m_Data[0];
    m_Data[1] = vec.m_Data[1];
    m_Data[2] = vec.m_Data[2];
    m_Data[3] = vec.m_Data[3];
}

//template <typename Real>
//Vector4<Real>::Vector4 (const Tuple<4,Real>& tuple)
//{
//    m_Data[0] = tuple[0];
//    m_Data[1] = tuple[1];
//    m_Data[2] = tuple[2];
//    m_Data[3] = tuple[3];
//}

template <typename Real>
Vector4<Real>::Vector4 (Real x, Real y, Real z, Real w)
{
    m_Data[0] = x;
    m_Data[1] = y;
    m_Data[2] = z;
    m_Data[3] = w;
}

template <typename Real>
Vector4<Real>& Vector4<Real>::operator= (const Vector4& vec)
{
    m_Data[0] = vec.m_Data[0];
    m_Data[1] = vec.m_Data[1];
    m_Data[2] = vec.m_Data[2];
    m_Data[3] = vec.m_Data[3];
    return *this;
}

template <typename Real>
inline Real Vector4<Real>::X () const
{
    return m_Data[0];
}

template <typename Real>
inline Real& Vector4<Real>::X ()
{
    return m_Data[0];
}

template <typename Real>
inline Real Vector4<Real>::Y () const
{
    return m_Data[1];
}

template <typename Real>
inline Real& Vector4<Real>::Y ()
{
    return m_Data[1];
}

template <typename Real>
inline Real Vector4<Real>::Z () const
{
    return m_Data[2];
}

template <typename Real>
inline Real& Vector4<Real>::Z ()
{
    return m_Data[2];
}

template <typename Real>
inline Real Vector4<Real>::W () const
{
    return m_Data[3];
}

template <typename Real>
inline Real& Vector4<Real>::W ()
{
    return m_Data[3];
}

template <typename Real>
inline Vector4<Real> Vector4<Real>::operator+ (const Vector4& vec) const
{
    return Vector4
    (
        m_Data[0] + vec.m_Data[0],
        m_Data[1] + vec.m_Data[1],
        m_Data[2] + vec.m_Data[2],
        m_Data[3] + vec.m_Data[3]
    );
}

template <typename Real>
inline Vector4<Real> Vector4<Real>::operator- (const Vector4& vec) const
{
    return Vector4
    (
        m_Data[0] - vec.m_Data[0],
        m_Data[1] - vec.m_Data[1],
        m_Data[2] - vec.m_Data[2],
        m_Data[3] - vec.m_Data[3]
    );
}

template <typename Real>
inline Vector4<Real> Vector4<Real>::operator* (Real scalar) const
{
    return Vector4
    (
        scalar*m_Data[0],
        scalar*m_Data[1],
        scalar*m_Data[2],
        scalar*m_Data[3]
    );
}

template <typename Real>
inline Vector4<Real> Vector4<Real>::operator/ (Real scalar) const
{
    Vector4 result;

    if (scalar != (Real)0)
    {
        Real invScalar = ((Real)1)/scalar;
        result.m_Data[0] = invScalar*m_Data[0];
        result.m_Data[1] = invScalar*m_Data[1];
        result.m_Data[2] = invScalar*m_Data[2];
        result.m_Data[3] = invScalar*m_Data[3];
    }
    else
    {
        result.m_Data[0] = Math<Real>::MAX_REAL;
        result.m_Data[1] = Math<Real>::MAX_REAL;
        result.m_Data[2] = Math<Real>::MAX_REAL;
        result.m_Data[3] = Math<Real>::MAX_REAL;
    }

    return result;
}

template <typename Real>
inline Vector4<Real> Vector4<Real>::operator- () const
{
    return Vector4
    (
        -m_Data[0],
        -m_Data[1],
        -m_Data[2],
        -m_Data[3]
    );
}

template <typename Real>
inline Vector4<Real>& Vector4<Real>::operator+= (const Vector4& vec)
{
    m_Data[0] += vec.m_Data[0];
    m_Data[1] += vec.m_Data[1];
    m_Data[2] += vec.m_Data[2];
    m_Data[3] += vec.m_Data[3];
    return *this;
}

template <typename Real>
inline Vector4<Real>& Vector4<Real>::operator-= (const Vector4& vec)
{
    m_Data[0] -= vec.m_Data[0];
    m_Data[1] -= vec.m_Data[1];
    m_Data[2] -= vec.m_Data[2];
    m_Data[3] -= vec.m_Data[3];
    return *this;
}

template <typename Real>
inline Vector4<Real>& Vector4<Real>::operator*= (Real scalar)
{
    m_Data[0] *= scalar;
    m_Data[1] *= scalar;
    m_Data[2] *= scalar;
    m_Data[3] *= scalar;
    return *this;
}

template <typename Real>
inline Vector4<Real>& Vector4<Real>::operator/= (Real scalar)
{
    if (scalar != (Real)0)
    {
        Real invScalar = ((Real)1)/scalar;
        m_Data[0] *= invScalar;
        m_Data[1] *= invScalar;
        m_Data[2] *= invScalar;
        m_Data[3] *= invScalar;
    }
    else
    {
        m_Data[0] *= Math<Real>::MAX_REAL;
        m_Data[1] *= Math<Real>::MAX_REAL;
        m_Data[2] *= Math<Real>::MAX_REAL;
        m_Data[3] *= Math<Real>::MAX_REAL;
    }

    return *this;
}

template <typename Real>
inline Real Vector4<Real>::Length () const
{
    return Math<Real>::Sqrt
    (
        m_Data[0]*m_Data[0] +
        m_Data[1]*m_Data[1] +
        m_Data[2]*m_Data[2] +
        m_Data[3]*m_Data[3]
    );
}

template <typename Real>
inline Real Vector4<Real>::SquaredLength () const
{
    return
        m_Data[0]*m_Data[0] +
        m_Data[1]*m_Data[1] +
        m_Data[2]*m_Data[2] +
        m_Data[3]*m_Data[3];
}

template <typename Real>
inline Real Vector4<Real>::Dot (const Vector4& vec) const
{
    return
        m_Data[0]*vec.m_Data[0] +
        m_Data[1]*vec.m_Data[1] +
        m_Data[2]*vec.m_Data[2] +
        m_Data[3]*vec.m_Data[3];
}

template <typename Real>
inline Real Vector4<Real>::Normalize ()
{
  return Normalize(Math<Real>::ZERO_TOLERANCE);
}

template <typename Real>
inline Real Vector4<Real>::Normalize (const Real epsilon)
{
    Real length = Length();

    if (length > epsilon)
    {
        Real invLength = ((Real)1)/length;
        m_Data[0] *= invLength;
        m_Data[1] *= invLength;
        m_Data[2] *= invLength;
        m_Data[3] *= invLength;
    }
    else
    {
        length = (Real)0;
        m_Data[0] = (Real)0;
        m_Data[1] = (Real)0;
        m_Data[2] = (Real)0;
        m_Data[3] = (Real)0;
    }

    return length;
}

template <typename Real>
void Vector4<Real>::ComputeExtremes (int numVectors, const Vector4* vectors,
    Vector4& vmin, Vector4& vmax)
{
    assertion(numVectors > 0 && vectors,
        "Invalid inputs to ComputeExtremes\n");

    vmin = vectors[0];
    vmax = vmin;
    for (int j = 1; j < numVectors; ++j)
    {
        const Vector4& vec = vectors[j];
        for (int i = 0; i < 4; ++i)
        {
            if (vec[i] < vmin[i])
            {
                vmin[i] = vec[i];
            }
            else if (vec[i] > vmax[i])
            {
                vmax[i] = vec[i];
            }
        }
    }
}

template <typename Real>
inline bool Vector4<Real>::operator <(Vector4 const& iOther) const
{
  if(m_Data[0] == iOther.m_Data[0])
  {
    if(m_Data[1] == iOther.m_Data[1])
    {
      if(m_Data[2] == iOther.m_Data[2])
      {
        return m_Data[3] < iOther.m_Data[3];
      }
      else
      {
        return m_Data[2] < iOther.m_Data[2];
      }
    }
    else
    {
      return m_Data[1] < iOther.m_Data[1];
    }
  }
  else
  {
    return m_Data[0] < iOther.m_Data[0];
  }
}

template <typename Real>
inline Vector4<Real> operator* (Real scalar, const Vector4<Real>& vec)
{
    return Vector4<Real>
    (
        scalar*vec[0],
        scalar*vec[1],
        scalar*vec[2],
        scalar*vec[3]
    );
}

//template <typename Real>
//std::ostream& operator<< (std::ostream& outFile, const Vector4<Real>& vec)
//{
//     return outFile << vec.X() << ' ' << vec.Y() << ' ' << vec.Z()
//         << ' ' << vec.W();
//}

