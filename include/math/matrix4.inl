
template <typename Real>
Matrix4<Real>::Matrix4 ()
{
  MakeIdentity();
}

template <typename Real>
Matrix4<Real>::Matrix4 (bool makeZero)
{
  if(makeZero)
  {
    MakeZero();
  }
  else
  {
    MakeIdentity();
  }
}

template <typename Real>
Matrix4<Real>::Matrix4 (const Matrix4& mat)
{
    m_Data[ 0] = mat.m_Data[ 0];
    m_Data[ 1] = mat.m_Data[ 1];
    m_Data[ 2] = mat.m_Data[ 2];
    m_Data[ 3] = mat.m_Data[ 3];
    m_Data[ 4] = mat.m_Data[ 4];
    m_Data[ 5] = mat.m_Data[ 5];
    m_Data[ 6] = mat.m_Data[ 6];
    m_Data[ 7] = mat.m_Data[ 7];
    m_Data[ 8] = mat.m_Data[ 8];
    m_Data[ 9] = mat.m_Data[ 9];
    m_Data[10] = mat.m_Data[10];
    m_Data[11] = mat.m_Data[11];
    m_Data[12] = mat.m_Data[12];
    m_Data[13] = mat.m_Data[13];
    m_Data[14] = mat.m_Data[14];
    m_Data[15] = mat.m_Data[15];
}

template <typename Real>
Matrix4<Real>::Matrix4 (Real m00, Real m01, Real m02, Real m03, Real m10,
    Real m11, Real m12, Real m13, Real m20, Real m21, Real m22, Real m23,
    Real m30, Real m31, Real m32, Real m33)
{
    m_Data[ 0] = m00;
    m_Data[ 1] = m01;
    m_Data[ 2] = m02;
    m_Data[ 3] = m03;
    m_Data[ 4] = m10;
    m_Data[ 5] = m11;
    m_Data[ 6] = m12;
    m_Data[ 7] = m13;
    m_Data[ 8] = m20;
    m_Data[ 9] = m21;
    m_Data[10] = m22;
    m_Data[11] = m23;
    m_Data[12] = m30;
    m_Data[13] = m31;
    m_Data[14] = m32;
    m_Data[15] = m33;
}

template <typename Real>
Matrix4<Real>::Matrix4 (const Real entry[16])
{
  //if (rowMajor)
  {
      m_Data[ 0] = entry[ 0];
      m_Data[ 1] = entry[ 1];
      m_Data[ 2] = entry[ 2];
      m_Data[ 3] = entry[ 3];
      m_Data[ 4] = entry[ 4];
      m_Data[ 5] = entry[ 5];
      m_Data[ 6] = entry[ 6];
      m_Data[ 7] = entry[ 7];
      m_Data[ 8] = entry[ 8];
      m_Data[ 9] = entry[ 9];
      m_Data[10] = entry[10];
      m_Data[11] = entry[11];
      m_Data[12] = entry[12];
      m_Data[13] = entry[13];
      m_Data[14] = entry[14];
      m_Data[15] = entry[15];
  }
  //else
  //{
  //    m_Data[ 0] = entry[ 0];
  //    m_Data[ 1] = entry[ 4];
  //    m_Data[ 2] = entry[ 8];
  //    m_Data[ 3] = entry[12];
  //    m_Data[ 4] = entry[ 1];
  //    m_Data[ 5] = entry[ 5];
  //    m_Data[ 6] = entry[ 9];
  //    m_Data[ 7] = entry[13];
  //    m_Data[ 8] = entry[ 2];
  //    m_Data[ 9] = entry[ 6];
  //    m_Data[10] = entry[10];
  //    m_Data[11] = entry[14];
  //    m_Data[12] = entry[ 3];
  //    m_Data[13] = entry[ 7];
  //    m_Data[14] = entry[11];
  //    m_Data[15] = entry[15];
  //}
}

template <typename Real>
Matrix4<Real>& Matrix4<Real>::operator= (const Matrix4& mat)
{
    m_Data[ 0] = mat.m_Data[ 0];
    m_Data[ 1] = mat.m_Data[ 1];
    m_Data[ 2] = mat.m_Data[ 2];
    m_Data[ 3] = mat.m_Data[ 3];
    m_Data[ 4] = mat.m_Data[ 4];
    m_Data[ 5] = mat.m_Data[ 5];
    m_Data[ 6] = mat.m_Data[ 6];
    m_Data[ 7] = mat.m_Data[ 7];
    m_Data[ 8] = mat.m_Data[ 8];
    m_Data[ 9] = mat.m_Data[ 9];
    m_Data[10] = mat.m_Data[10];
    m_Data[11] = mat.m_Data[11];
    m_Data[12] = mat.m_Data[12];
    m_Data[13] = mat.m_Data[13];
    m_Data[14] = mat.m_Data[14];
    m_Data[15] = mat.m_Data[15];
    return *this;
}

template <typename Real>
Matrix4<Real> Matrix4<Real>::FromPosition(Vector3<Real> const& iPos)
{
  Matrix4<Real> mat;
  mat.MakeIdentity();
  *reinterpret_cast<Vector3<Real>*>(mat.m_Data + 12) = iPos;

  return mat;
}

template <typename Real>
Matrix4<Real> Matrix4<Real>::FromPositionAndOrientation(Vector3<Real> const& iPos, Quaternion<Real> const& iOrient)
{
  Matrix4<Real> mat;
  mat.m_Data[ 3] = (Real)0;
  mat.m_Data[ 7] = (Real)0;
  mat.m_Data[11] = (Real)0;
  mat.m_Data[15] = (Real)1;

  *reinterpret_cast<Vector3<Real>*>(mat.m_Data +  0) = iOrient.Rotate(Vector3<Real>::UNIT_X);
  *reinterpret_cast<Vector3<Real>*>(mat.m_Data +  4) = iOrient.Rotate(Vector3<Real>::UNIT_Y);
  *reinterpret_cast<Vector3<Real>*>(mat.m_Data +  8) = iOrient.Rotate(Vector3<Real>::UNIT_Z);
  *reinterpret_cast<Vector3<Real>*>(mat.m_Data + 12) = iPos;

  return mat;
}

template <typename Real>
void Matrix4<Real>::MakeZero ()
{
    m_Data[ 0] = (Real)0;
    m_Data[ 1] = (Real)0;
    m_Data[ 2] = (Real)0;
    m_Data[ 3] = (Real)0;
    m_Data[ 4] = (Real)0;
    m_Data[ 5] = (Real)0;
    m_Data[ 6] = (Real)0;
    m_Data[ 7] = (Real)0;
    m_Data[ 8] = (Real)0;
    m_Data[ 9] = (Real)0;
    m_Data[10] = (Real)0;
    m_Data[11] = (Real)0;
    m_Data[12] = (Real)0;
    m_Data[13] = (Real)0;
    m_Data[14] = (Real)0;
    m_Data[15] = (Real)0;
}

template <typename Real>
void Matrix4<Real>::MakeIdentity ()
{
    m_Data[ 0] = (Real)1;
    m_Data[ 1] = (Real)0;
    m_Data[ 2] = (Real)0;
    m_Data[ 3] = (Real)0;
    m_Data[ 4] = (Real)0;
    m_Data[ 5] = (Real)1;
    m_Data[ 6] = (Real)0;
    m_Data[ 7] = (Real)0;
    m_Data[ 8] = (Real)0;
    m_Data[ 9] = (Real)0;
    m_Data[10] = (Real)1;
    m_Data[11] = (Real)0;
    m_Data[12] = (Real)0;
    m_Data[13] = (Real)0;
    m_Data[14] = (Real)0;
    m_Data[15] = (Real)1;
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator+ (const Matrix4& mat) const
{
    return Matrix4<Real>
    (
        m_Data[ 0] + mat.m_Data[ 0],
        m_Data[ 1] + mat.m_Data[ 1],
        m_Data[ 2] + mat.m_Data[ 2],
        m_Data[ 3] + mat.m_Data[ 3],
        m_Data[ 4] + mat.m_Data[ 4],
        m_Data[ 5] + mat.m_Data[ 5],
        m_Data[ 6] + mat.m_Data[ 6],
        m_Data[ 7] + mat.m_Data[ 7],
        m_Data[ 8] + mat.m_Data[ 8],
        m_Data[ 9] + mat.m_Data[ 9],
        m_Data[10] + mat.m_Data[10],
        m_Data[11] + mat.m_Data[11],
        m_Data[12] + mat.m_Data[12],
        m_Data[13] + mat.m_Data[13],
        m_Data[14] + mat.m_Data[14],
        m_Data[15] + mat.m_Data[15]
    );
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator- (const Matrix4& mat) const
{
    return Matrix4<Real>
    (
        m_Data[ 0] - mat.m_Data[ 0],
        m_Data[ 1] - mat.m_Data[ 1],
        m_Data[ 2] - mat.m_Data[ 2],
        m_Data[ 3] - mat.m_Data[ 3],
        m_Data[ 4] - mat.m_Data[ 4],
        m_Data[ 5] - mat.m_Data[ 5],
        m_Data[ 6] - mat.m_Data[ 6],
        m_Data[ 7] - mat.m_Data[ 7],
        m_Data[ 8] - mat.m_Data[ 8],
        m_Data[ 9] - mat.m_Data[ 9],
        m_Data[10] - mat.m_Data[10],
        m_Data[11] - mat.m_Data[11],
        m_Data[12] - mat.m_Data[12],
        m_Data[13] - mat.m_Data[13],
        m_Data[14] - mat.m_Data[14],
        m_Data[15] - mat.m_Data[15]
    );
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator* (Real scalar) const
{
    return Matrix4<Real>
    (
        scalar*m_Data[ 0],
        scalar*m_Data[ 1],
        scalar*m_Data[ 2],
        scalar*m_Data[ 3],
        scalar*m_Data[ 4],
        scalar*m_Data[ 5],
        scalar*m_Data[ 6],
        scalar*m_Data[ 7],
        scalar*m_Data[ 8],
        scalar*m_Data[ 9],
        scalar*m_Data[10],
        scalar*m_Data[11],
        scalar*m_Data[12],
        scalar*m_Data[13],
        scalar*m_Data[14],
        scalar*m_Data[15]
    );
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator/ (Real scalar) const
{
    if (scalar != (Real)0)
    {
        Real invScalar = ((Real)1)/scalar;
        return Matrix4<Real>
        (
            invScalar*m_Data[ 0],
            invScalar*m_Data[ 1],
            invScalar*m_Data[ 2],
            invScalar*m_Data[ 3],
            invScalar*m_Data[ 4],
            invScalar*m_Data[ 5],
            invScalar*m_Data[ 6],
            invScalar*m_Data[ 7],
            invScalar*m_Data[ 8],
            invScalar*m_Data[ 9],
            invScalar*m_Data[10],
            invScalar*m_Data[11],
            invScalar*m_Data[12],
            invScalar*m_Data[13],
            invScalar*m_Data[14],
            invScalar*m_Data[15]
        );
    }
    else
    {
        return Matrix4<Real>
        (
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL,
            Math<Real>::MAX_REAL
        );
    }
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator- () const
{
    return Matrix4<Real>
    (
        -m_Data[ 0],
        -m_Data[ 1],
        -m_Data[ 2],
        -m_Data[ 3],
        -m_Data[ 4],
        -m_Data[ 5],
        -m_Data[ 6],
        -m_Data[ 7],
        -m_Data[ 8],
        -m_Data[ 9],
        -m_Data[10],
        -m_Data[11],
        -m_Data[12],
        -m_Data[13],
        -m_Data[14],
        -m_Data[15]
    );
}

template <class Real>
inline Matrix4<Real>& Matrix4<Real>::operator+= (const Matrix4& mat)
{
    m_Data[ 0] += mat.m_Data[ 0];
    m_Data[ 1] += mat.m_Data[ 1];
    m_Data[ 2] += mat.m_Data[ 2];
    m_Data[ 3] += mat.m_Data[ 3];
    m_Data[ 4] += mat.m_Data[ 4];
    m_Data[ 5] += mat.m_Data[ 5];
    m_Data[ 6] += mat.m_Data[ 6];
    m_Data[ 7] += mat.m_Data[ 7];
    m_Data[ 8] += mat.m_Data[ 8];
    m_Data[ 9] += mat.m_Data[ 9];
    m_Data[10] += mat.m_Data[10];
    m_Data[11] += mat.m_Data[11];
    m_Data[12] += mat.m_Data[12];
    m_Data[13] += mat.m_Data[13];
    m_Data[14] += mat.m_Data[14];
    m_Data[15] += mat.m_Data[15];
    return *this;
}

template <class Real>
inline Matrix4<Real>& Matrix4<Real>::operator-= (const Matrix4& mat)
{
    m_Data[ 0] -= mat.m_Data[ 0];
    m_Data[ 1] -= mat.m_Data[ 1];
    m_Data[ 2] -= mat.m_Data[ 2];
    m_Data[ 3] -= mat.m_Data[ 3];
    m_Data[ 4] -= mat.m_Data[ 4];
    m_Data[ 5] -= mat.m_Data[ 5];
    m_Data[ 6] -= mat.m_Data[ 6];
    m_Data[ 7] -= mat.m_Data[ 7];
    m_Data[ 8] -= mat.m_Data[ 8];
    m_Data[ 9] -= mat.m_Data[ 9];
    m_Data[10] -= mat.m_Data[10];
    m_Data[11] -= mat.m_Data[11];
    m_Data[12] -= mat.m_Data[12];
    m_Data[13] -= mat.m_Data[13];
    m_Data[14] -= mat.m_Data[14];
    m_Data[15] -= mat.m_Data[15];
    return *this;
}

template <class Real>
inline Matrix4<Real>& Matrix4<Real>::operator*= (Real scalar)
{
    m_Data[ 0] *= scalar;
    m_Data[ 1] *= scalar;
    m_Data[ 2] *= scalar;
    m_Data[ 3] *= scalar;
    m_Data[ 4] *= scalar;
    m_Data[ 5] *= scalar;
    m_Data[ 6] *= scalar;
    m_Data[ 7] *= scalar;
    m_Data[ 8] *= scalar;
    m_Data[ 9] *= scalar;
    m_Data[10] *= scalar;
    m_Data[11] *= scalar;
    m_Data[12] *= scalar;
    m_Data[13] *= scalar;
    m_Data[14] *= scalar;
    m_Data[15] *= scalar;
    return *this;
}

template <class Real>
inline Matrix4<Real>& Matrix4<Real>::operator/= (Real scalar)
{
    if (scalar != (Real)0)
    {
        Real invScalar = ((Real)1)/scalar;
        m_Data[ 0] *= invScalar;
        m_Data[ 1] *= invScalar;
        m_Data[ 2] *= invScalar;
        m_Data[ 3] *= invScalar;
        m_Data[ 4] *= invScalar;
        m_Data[ 5] *= invScalar;
        m_Data[ 6] *= invScalar;
        m_Data[ 7] *= invScalar;
        m_Data[ 8] *= invScalar;
        m_Data[ 9] *= invScalar;
        m_Data[10] *= invScalar;
        m_Data[11] *= invScalar;
        m_Data[12] *= invScalar;
        m_Data[13] *= invScalar;
        m_Data[14] *= invScalar;
        m_Data[15] *= invScalar;
    }
    else
    {
        m_Data[ 0] = Math<Real>::MAX_REAL;
        m_Data[ 1] = Math<Real>::MAX_REAL;
        m_Data[ 2] = Math<Real>::MAX_REAL;
        m_Data[ 3] = Math<Real>::MAX_REAL;
        m_Data[ 4] = Math<Real>::MAX_REAL;
        m_Data[ 5] = Math<Real>::MAX_REAL;
        m_Data[ 6] = Math<Real>::MAX_REAL;
        m_Data[ 7] = Math<Real>::MAX_REAL;
        m_Data[ 8] = Math<Real>::MAX_REAL;
        m_Data[ 9] = Math<Real>::MAX_REAL;
        m_Data[10] = Math<Real>::MAX_REAL;
        m_Data[11] = Math<Real>::MAX_REAL;
        m_Data[12] = Math<Real>::MAX_REAL;
        m_Data[13] = Math<Real>::MAX_REAL;
        m_Data[14] = Math<Real>::MAX_REAL;
        m_Data[15] = Math<Real>::MAX_REAL;
    }

    return *this;
}

template <class Real>
inline Vector4<Real> Matrix4<Real>::operator* (const Vector4<Real>& vec) const
{
    return Vector4<Real>
    (
      m_Data[0] * vec.m_Data[0] +
      m_Data[4] * vec.m_Data[1] +
      m_Data[8] * vec.m_Data[2] +
      m_Data[12] * vec.m_Data[3],

      m_Data[1] * vec.m_Data[0] +
      m_Data[5] * vec.m_Data[1] +
      m_Data[9] * vec.m_Data[2] +
      m_Data[13] * vec.m_Data[3],

      m_Data[2] * vec.m_Data[0] +
      m_Data[6] * vec.m_Data[1] +
      m_Data[10] * vec.m_Data[2] +
      m_Data[14] * vec.m_Data[3],

      m_Data[3] * vec.m_Data[0] +
      m_Data[7] * vec.m_Data[1] +
      m_Data[11] * vec.m_Data[2] +
      m_Data[15] * vec.m_Data[3]

#if 0
        m_Data[ 0]*vec.m_Data[0] +
        m_Data[ 1]*vec.m_Data[1] +
        m_Data[ 2]*vec.m_Data[2] +
        m_Data[ 3]*vec.m_Data[3],

        m_Data[ 4]*vec.m_Data[0] +
        m_Data[ 5]*vec.m_Data[1] +
        m_Data[ 6]*vec.m_Data[2] +
        m_Data[ 7]*vec.m_Data[3],

        m_Data[ 8]*vec.m_Data[0] +
        m_Data[ 9]*vec.m_Data[1] +
        m_Data[10]*vec.m_Data[2] +
        m_Data[11]*vec.m_Data[3],

        m_Data[12]*vec.m_Data[0] +
        m_Data[13]*vec.m_Data[1] +
        m_Data[14]*vec.m_Data[2] +
        m_Data[15]*vec.m_Data[3]
#endif
    );
}

template <class Real>
Matrix4<Real> Matrix4<Real>::Transpose () const
{
    return Matrix4<Real>
    (
        m_Data[ 0],
        m_Data[ 4],
        m_Data[ 8],
        m_Data[12],
        m_Data[ 1],
        m_Data[ 5],
        m_Data[ 9],
        m_Data[13],
        m_Data[ 2],
        m_Data[ 6],
        m_Data[10],
        m_Data[14],
        m_Data[ 3],
        m_Data[ 7],
        m_Data[11],
        m_Data[15]
    );
}

template <class Real>
inline Matrix4<Real> Matrix4<Real>::operator* (const Matrix4& mat) const
{
    // A*B
    return Matrix4<Real>
    (
      m_Data[0] * mat.m_Data[0] +
      m_Data[4] * mat.m_Data[1] +
      m_Data[8] * mat.m_Data[2] +
      m_Data[12] * mat.m_Data[3],

      m_Data[1] * mat.m_Data[0] +
      m_Data[5] * mat.m_Data[1] +
      m_Data[9] * mat.m_Data[2] +
      m_Data[13] * mat.m_Data[3],

      m_Data[2] * mat.m_Data[0] +
      m_Data[6] * mat.m_Data[1] +
      m_Data[10] * mat.m_Data[2] +
      m_Data[14] * mat.m_Data[3],

      m_Data[3] * mat.m_Data[0] +
      m_Data[7] * mat.m_Data[1] +
      m_Data[11] * mat.m_Data[2] +
      m_Data[15] * mat.m_Data[3],

      m_Data[0] * mat.m_Data[4] +
      m_Data[4] * mat.m_Data[5] +
      m_Data[8] * mat.m_Data[6] +
      m_Data[12] * mat.m_Data[7],

      m_Data[1] * mat.m_Data[4] +
      m_Data[5] * mat.m_Data[5] +
      m_Data[9] * mat.m_Data[6] +
      m_Data[13] * mat.m_Data[7],

      m_Data[2] * mat.m_Data[4] +
      m_Data[6] * mat.m_Data[5] +
      m_Data[10] * mat.m_Data[6] +
      m_Data[14] * mat.m_Data[7],

      m_Data[3] * mat.m_Data[4] +
      m_Data[7] * mat.m_Data[5] +
      m_Data[11] * mat.m_Data[6] +
      m_Data[15] * mat.m_Data[7],

      m_Data[0] * mat.m_Data[8] +
      m_Data[4] * mat.m_Data[9] +
      m_Data[8] * mat.m_Data[10] +
      m_Data[12] * mat.m_Data[11],

      m_Data[1] * mat.m_Data[8] +
      m_Data[5] * mat.m_Data[9] +
      m_Data[9] * mat.m_Data[10] +
      m_Data[13] * mat.m_Data[11],

      m_Data[2] * mat.m_Data[8] +
      m_Data[6] * mat.m_Data[9] +
      m_Data[10] * mat.m_Data[10] +
      m_Data[14] * mat.m_Data[11],

      m_Data[3] * mat.m_Data[8] +
      m_Data[7] * mat.m_Data[9] +
      m_Data[11] * mat.m_Data[10] +
      m_Data[15] * mat.m_Data[11],

      m_Data[0] * mat.m_Data[12] +
      m_Data[4] * mat.m_Data[13] +
      m_Data[8] * mat.m_Data[14] +
      m_Data[12] * mat.m_Data[15],

      m_Data[1] * mat.m_Data[12] +
      m_Data[5] * mat.m_Data[13] +
      m_Data[9] * mat.m_Data[14] +
      m_Data[13] * mat.m_Data[15],

      m_Data[2] * mat.m_Data[12] +
      m_Data[6] * mat.m_Data[13] +
      m_Data[10] * mat.m_Data[14] +
      m_Data[14] * mat.m_Data[15],

      m_Data[3] * mat.m_Data[12] +
      m_Data[7] * mat.m_Data[13] +
      m_Data[11] * mat.m_Data[14] +
      m_Data[15] * mat.m_Data[15]

#if 0
        m_Data[ 0]*mat.m_Data[ 0] +
        m_Data[ 1]*mat.m_Data[ 4] +
        m_Data[ 2]*mat.m_Data[ 8] +
        m_Data[ 3]*mat.m_Data[12],

        m_Data[ 0]*mat.m_Data[ 1] +
        m_Data[ 1]*mat.m_Data[ 5] +
        m_Data[ 2]*mat.m_Data[ 9] +
        m_Data[ 3]*mat.m_Data[13],

        m_Data[ 0]*mat.m_Data[ 2] +
        m_Data[ 1]*mat.m_Data[ 6] +
        m_Data[ 2]*mat.m_Data[10] +
        m_Data[ 3]*mat.m_Data[14],

        m_Data[ 0]*mat.m_Data[ 3] +
        m_Data[ 1]*mat.m_Data[ 7] +
        m_Data[ 2]*mat.m_Data[11] +
        m_Data[ 3]*mat.m_Data[15],

        m_Data[ 4]*mat.m_Data[ 0] +
        m_Data[ 5]*mat.m_Data[ 4] +
        m_Data[ 6]*mat.m_Data[ 8] +
        m_Data[ 7]*mat.m_Data[12],

        m_Data[ 4]*mat.m_Data[ 1] +
        m_Data[ 5]*mat.m_Data[ 5] +
        m_Data[ 6]*mat.m_Data[ 9] +
        m_Data[ 7]*mat.m_Data[13],

        m_Data[ 4]*mat.m_Data[ 2] +
        m_Data[ 5]*mat.m_Data[ 6] +
        m_Data[ 6]*mat.m_Data[10] +
        m_Data[ 7]*mat.m_Data[14],

        m_Data[ 4]*mat.m_Data[ 3] +
        m_Data[ 5]*mat.m_Data[ 7] +
        m_Data[ 6]*mat.m_Data[11] +
        m_Data[ 7]*mat.m_Data[15],

        m_Data[ 8]*mat.m_Data[ 0] +
        m_Data[ 9]*mat.m_Data[ 4] +
        m_Data[10]*mat.m_Data[ 8] +
        m_Data[11]*mat.m_Data[12],

        m_Data[ 8]*mat.m_Data[ 1] +
        m_Data[ 9]*mat.m_Data[ 5] +
        m_Data[10]*mat.m_Data[ 9] +
        m_Data[11]*mat.m_Data[13],

        m_Data[ 8]*mat.m_Data[ 2] +
        m_Data[ 9]*mat.m_Data[ 6] +
        m_Data[10]*mat.m_Data[10] +
        m_Data[11]*mat.m_Data[14],

        m_Data[ 8]*mat.m_Data[ 3] +
        m_Data[ 9]*mat.m_Data[ 7] +
        m_Data[10]*mat.m_Data[11] +
        m_Data[11]*mat.m_Data[15],

        m_Data[12]*mat.m_Data[ 0] +
        m_Data[13]*mat.m_Data[ 4] +
        m_Data[14]*mat.m_Data[ 8] +
        m_Data[15]*mat.m_Data[12],

        m_Data[12]*mat.m_Data[ 1] +
        m_Data[13]*mat.m_Data[ 5] +
        m_Data[14]*mat.m_Data[ 9] +
        m_Data[15]*mat.m_Data[13],

        m_Data[12]*mat.m_Data[ 2] +
        m_Data[13]*mat.m_Data[ 6] +
        m_Data[14]*mat.m_Data[10] +
        m_Data[15]*mat.m_Data[14],

        m_Data[12]*mat.m_Data[ 3] +
        m_Data[13]*mat.m_Data[ 7] +
        m_Data[14]*mat.m_Data[11] +
        m_Data[15]*mat.m_Data[15]
#endif
    );
}

template <class Real>
Matrix4<Real> Matrix4<Real>::TransposeTimes (const Matrix4& mat) const
{
    // A^T*B
    return Matrix4<Real>
    (
        m_Data[ 0]*mat.m_Data[ 0] +
        m_Data[ 4]*mat.m_Data[ 4] +
        m_Data[ 8]*mat.m_Data[ 8] +
        m_Data[12]*mat.m_Data[12],

        m_Data[ 0]*mat.m_Data[ 1] +
        m_Data[ 4]*mat.m_Data[ 5] +
        m_Data[ 8]*mat.m_Data[ 9] +
        m_Data[12]*mat.m_Data[13],

        m_Data[ 0]*mat.m_Data[ 2] +
        m_Data[ 4]*mat.m_Data[ 6] +
        m_Data[ 8]*mat.m_Data[10] +
        m_Data[12]*mat.m_Data[14],

        m_Data[ 0]*mat.m_Data[ 3] +
        m_Data[ 4]*mat.m_Data[ 7] +
        m_Data[ 8]*mat.m_Data[11] +
        m_Data[12]*mat.m_Data[15],

        m_Data[ 1]*mat.m_Data[ 0] +
        m_Data[ 5]*mat.m_Data[ 4] +
        m_Data[ 9]*mat.m_Data[ 8] +
        m_Data[13]*mat.m_Data[12],

        m_Data[ 1]*mat.m_Data[ 1] +
        m_Data[ 5]*mat.m_Data[ 5] +
        m_Data[ 9]*mat.m_Data[ 9] +
        m_Data[13]*mat.m_Data[13],

        m_Data[ 1]*mat.m_Data[ 2] +
        m_Data[ 5]*mat.m_Data[ 6] +
        m_Data[ 9]*mat.m_Data[10] +
        m_Data[13]*mat.m_Data[14],

        m_Data[ 1]*mat.m_Data[ 3] +
        m_Data[ 5]*mat.m_Data[ 7] +
        m_Data[ 9]*mat.m_Data[11] +
        m_Data[13]*mat.m_Data[15],

        m_Data[ 2]*mat.m_Data[ 0] +
        m_Data[ 6]*mat.m_Data[ 4] +
        m_Data[10]*mat.m_Data[ 8] +
        m_Data[14]*mat.m_Data[12],

        m_Data[ 2]*mat.m_Data[ 1] +
        m_Data[ 6]*mat.m_Data[ 5] +
        m_Data[10]*mat.m_Data[ 9] +
        m_Data[14]*mat.m_Data[13],

        m_Data[ 2]*mat.m_Data[ 2] +
        m_Data[ 6]*mat.m_Data[ 6] +
        m_Data[10]*mat.m_Data[10] +
        m_Data[14]*mat.m_Data[14],

        m_Data[ 2]*mat.m_Data[ 3] +
        m_Data[ 6]*mat.m_Data[ 7] +
        m_Data[10]*mat.m_Data[11] +
        m_Data[14]*mat.m_Data[15],

        m_Data[ 3]*mat.m_Data[ 0] +
        m_Data[ 7]*mat.m_Data[ 4] +
        m_Data[11]*mat.m_Data[ 8] +
        m_Data[15]*mat.m_Data[12],

        m_Data[ 3]*mat.m_Data[ 1] +
        m_Data[ 7]*mat.m_Data[ 5] +
        m_Data[11]*mat.m_Data[ 9] +
        m_Data[15]*mat.m_Data[13],

        m_Data[ 3]*mat.m_Data[ 2] +
        m_Data[ 7]*mat.m_Data[ 6] +
        m_Data[11]*mat.m_Data[10] +
        m_Data[15]*mat.m_Data[14],

        m_Data[ 3]*mat.m_Data[ 3] +
        m_Data[ 7]*mat.m_Data[ 7] +
        m_Data[11]*mat.m_Data[11] +
        m_Data[15]*mat.m_Data[15]
    );
}

template <class Real>
Matrix4<Real> Matrix4<Real>::TimesTranspose (const Matrix4& mat) const
{
    // A*B^T
    return Matrix4<Real>
    (
        m_Data[ 0]*mat.m_Data[ 0] +
        m_Data[ 1]*mat.m_Data[ 1] +
        m_Data[ 2]*mat.m_Data[ 2] +
        m_Data[ 3]*mat.m_Data[ 3],

        m_Data[ 0]*mat.m_Data[ 4] +
        m_Data[ 1]*mat.m_Data[ 5] +
        m_Data[ 2]*mat.m_Data[ 6] +
        m_Data[ 3]*mat.m_Data[ 7],

        m_Data[ 0]*mat.m_Data[ 8] +
        m_Data[ 1]*mat.m_Data[ 9] +
        m_Data[ 2]*mat.m_Data[10] +
        m_Data[ 3]*mat.m_Data[11],

        m_Data[ 0]*mat.m_Data[12] +
        m_Data[ 1]*mat.m_Data[13] +
        m_Data[ 2]*mat.m_Data[14] +
        m_Data[ 3]*mat.m_Data[15],

        m_Data[ 4]*mat.m_Data[ 0] +
        m_Data[ 5]*mat.m_Data[ 1] +
        m_Data[ 6]*mat.m_Data[ 2] +
        m_Data[ 7]*mat.m_Data[ 3],

        m_Data[ 4]*mat.m_Data[ 4] +
        m_Data[ 5]*mat.m_Data[ 5] +
        m_Data[ 6]*mat.m_Data[ 6] +
        m_Data[ 7]*mat.m_Data[ 7],

        m_Data[ 4]*mat.m_Data[ 8] +
        m_Data[ 5]*mat.m_Data[ 9] +
        m_Data[ 6]*mat.m_Data[10] +
        m_Data[ 7]*mat.m_Data[11],

        m_Data[ 4]*mat.m_Data[12] +
        m_Data[ 5]*mat.m_Data[13] +
        m_Data[ 6]*mat.m_Data[14] +
        m_Data[ 7]*mat.m_Data[15],

        m_Data[ 8]*mat.m_Data[ 0] +
        m_Data[ 9]*mat.m_Data[ 1] +
        m_Data[10]*mat.m_Data[ 2] +
        m_Data[11]*mat.m_Data[ 3],

        m_Data[ 8]*mat.m_Data[ 4] +
        m_Data[ 9]*mat.m_Data[ 5] +
        m_Data[10]*mat.m_Data[ 6] +
        m_Data[11]*mat.m_Data[ 7],

        m_Data[ 8]*mat.m_Data[ 8] +
        m_Data[ 9]*mat.m_Data[ 9] +
        m_Data[10]*mat.m_Data[10] +
        m_Data[11]*mat.m_Data[11],

        m_Data[ 8]*mat.m_Data[12] +
        m_Data[ 9]*mat.m_Data[13] +
        m_Data[10]*mat.m_Data[14] +
        m_Data[11]*mat.m_Data[15],

        m_Data[12]*mat.m_Data[ 0] +
        m_Data[13]*mat.m_Data[ 1] +
        m_Data[14]*mat.m_Data[ 2] +
        m_Data[15]*mat.m_Data[ 3],

        m_Data[12]*mat.m_Data[ 4] +
        m_Data[13]*mat.m_Data[ 5] +
        m_Data[14]*mat.m_Data[ 6] +
        m_Data[15]*mat.m_Data[ 7],

        m_Data[12]*mat.m_Data[ 8] +
        m_Data[13]*mat.m_Data[ 9] +
        m_Data[14]*mat.m_Data[10] +
        m_Data[15]*mat.m_Data[11],

        m_Data[12]*mat.m_Data[12] +
        m_Data[13]*mat.m_Data[13] +
        m_Data[14]*mat.m_Data[14] +
        m_Data[15]*mat.m_Data[15]
    );
}

template <class Real>
Matrix4<Real> Matrix4<Real>::TransposeTimesTranspose (const Matrix4& mat)
    const
{
    // A^T*B^T
    return Matrix4<Real>
    (
        m_Data[ 0]*mat.m_Data[ 0] +
        m_Data[ 4]*mat.m_Data[ 1] +
        m_Data[ 8]*mat.m_Data[ 2] +
        m_Data[12]*mat.m_Data[ 3],

        m_Data[ 0]*mat.m_Data[ 4] +
        m_Data[ 4]*mat.m_Data[ 5] +
        m_Data[ 8]*mat.m_Data[ 6] +
        m_Data[12]*mat.m_Data[ 7],

        m_Data[ 0]*mat.m_Data[ 8] +
        m_Data[ 4]*mat.m_Data[ 9] +
        m_Data[ 8]*mat.m_Data[10] +
        m_Data[12]*mat.m_Data[11],

        m_Data[ 0]*mat.m_Data[12] +
        m_Data[ 4]*mat.m_Data[13] +
        m_Data[ 8]*mat.m_Data[14] +
        m_Data[12]*mat.m_Data[15],

        m_Data[ 1]*mat.m_Data[ 0] +
        m_Data[ 5]*mat.m_Data[ 1] +
        m_Data[ 9]*mat.m_Data[ 2] +
        m_Data[13]*mat.m_Data[ 3],

        m_Data[ 1]*mat.m_Data[ 4] +
        m_Data[ 5]*mat.m_Data[ 5] +
        m_Data[ 9]*mat.m_Data[ 6] +
        m_Data[13]*mat.m_Data[ 7],

        m_Data[ 1]*mat.m_Data[ 8] +
        m_Data[ 5]*mat.m_Data[ 9] +
        m_Data[ 9]*mat.m_Data[10] +
        m_Data[13]*mat.m_Data[11],

        m_Data[ 1]*mat.m_Data[12] +
        m_Data[ 5]*mat.m_Data[13] +
        m_Data[ 9]*mat.m_Data[14] +
        m_Data[13]*mat.m_Data[15],

        m_Data[ 2]*mat.m_Data[ 0] +
        m_Data[ 6]*mat.m_Data[ 1] +
        m_Data[10]*mat.m_Data[ 2] +
        m_Data[14]*mat.m_Data[ 3],

        m_Data[ 2]*mat.m_Data[ 4] +
        m_Data[ 6]*mat.m_Data[ 5] +
        m_Data[10]*mat.m_Data[ 6] +
        m_Data[14]*mat.m_Data[ 7],

        m_Data[ 2]*mat.m_Data[ 8] +
        m_Data[ 6]*mat.m_Data[ 9] +
        m_Data[10]*mat.m_Data[10] +
        m_Data[14]*mat.m_Data[11],

        m_Data[ 2]*mat.m_Data[12] +
        m_Data[ 6]*mat.m_Data[13] +
        m_Data[10]*mat.m_Data[14] +
        m_Data[14]*mat.m_Data[15],

        m_Data[ 3]*mat.m_Data[ 0] +
        m_Data[ 7]*mat.m_Data[ 1] +
        m_Data[11]*mat.m_Data[ 2] +
        m_Data[15]*mat.m_Data[ 3],

        m_Data[ 3]*mat.m_Data[ 4] +
        m_Data[ 7]*mat.m_Data[ 5] +
        m_Data[11]*mat.m_Data[ 6] +
        m_Data[15]*mat.m_Data[ 7],

        m_Data[ 3]*mat.m_Data[ 8] +
        m_Data[ 7]*mat.m_Data[ 9] +
        m_Data[11]*mat.m_Data[10] +
        m_Data[15]*mat.m_Data[11],

        m_Data[ 3]*mat.m_Data[12] +
        m_Data[ 7]*mat.m_Data[13] +
        m_Data[11]*mat.m_Data[14] +
        m_Data[15]*mat.m_Data[15]
    );
}

template <typename Real>
Matrix4<Real> Matrix4<Real>::Inverse (const Real epsilon) const
{
    Real a0 = m_Data[ 0]*m_Data[ 5] - m_Data[ 1]*m_Data[ 4];
    Real a1 = m_Data[ 0]*m_Data[ 6] - m_Data[ 2]*m_Data[ 4];
    Real a2 = m_Data[ 0]*m_Data[ 7] - m_Data[ 3]*m_Data[ 4];
    Real a3 = m_Data[ 1]*m_Data[ 6] - m_Data[ 2]*m_Data[ 5];
    Real a4 = m_Data[ 1]*m_Data[ 7] - m_Data[ 3]*m_Data[ 5];
    Real a5 = m_Data[ 2]*m_Data[ 7] - m_Data[ 3]*m_Data[ 6];
    Real b0 = m_Data[ 8]*m_Data[13] - m_Data[ 9]*m_Data[12];
    Real b1 = m_Data[ 8]*m_Data[14] - m_Data[10]*m_Data[12];
    Real b2 = m_Data[ 8]*m_Data[15] - m_Data[11]*m_Data[12];
    Real b3 = m_Data[ 9]*m_Data[14] - m_Data[10]*m_Data[13];
    Real b4 = m_Data[ 9]*m_Data[15] - m_Data[11]*m_Data[13];
    Real b5 = m_Data[10]*m_Data[15] - m_Data[11]*m_Data[14];

    Real det = a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;
    if (Math<Real>::FAbs(det) > epsilon)
    {
        Matrix4 inverse;
        inverse.m_Data[ 0] = + m_Data[ 5]*b5 - m_Data[ 6]*b4 + m_Data[ 7]*b3;
        inverse.m_Data[ 4] = - m_Data[ 4]*b5 + m_Data[ 6]*b2 - m_Data[ 7]*b1;
        inverse.m_Data[ 8] = + m_Data[ 4]*b4 - m_Data[ 5]*b2 + m_Data[ 7]*b0;
        inverse.m_Data[12] = - m_Data[ 4]*b3 + m_Data[ 5]*b1 - m_Data[ 6]*b0;
        inverse.m_Data[ 1] = - m_Data[ 1]*b5 + m_Data[ 2]*b4 - m_Data[ 3]*b3;
        inverse.m_Data[ 5] = + m_Data[ 0]*b5 - m_Data[ 2]*b2 + m_Data[ 3]*b1;
        inverse.m_Data[ 9] = - m_Data[ 0]*b4 + m_Data[ 1]*b2 - m_Data[ 3]*b0;
        inverse.m_Data[13] = + m_Data[ 0]*b3 - m_Data[ 1]*b1 + m_Data[ 2]*b0;
        inverse.m_Data[ 2] = + m_Data[13]*a5 - m_Data[14]*a4 + m_Data[15]*a3;
        inverse.m_Data[ 6] = - m_Data[12]*a5 + m_Data[14]*a2 - m_Data[15]*a1;
        inverse.m_Data[10] = + m_Data[12]*a4 - m_Data[13]*a2 + m_Data[15]*a0;
        inverse.m_Data[14] = - m_Data[12]*a3 + m_Data[13]*a1 - m_Data[14]*a0;
        inverse.m_Data[ 3] = - m_Data[ 9]*a5 + m_Data[10]*a4 - m_Data[11]*a3;
        inverse.m_Data[ 7] = + m_Data[ 8]*a5 - m_Data[10]*a2 + m_Data[11]*a1;
        inverse.m_Data[11] = - m_Data[ 8]*a4 + m_Data[ 9]*a2 - m_Data[11]*a0;
        inverse.m_Data[15] = + m_Data[ 8]*a3 - m_Data[ 9]*a1 + m_Data[10]*a0;

        Real invDet = ((Real)1)/det;
        inverse.m_Data[ 0] *= invDet;
        inverse.m_Data[ 1] *= invDet;
        inverse.m_Data[ 2] *= invDet;
        inverse.m_Data[ 3] *= invDet;
        inverse.m_Data[ 4] *= invDet;
        inverse.m_Data[ 5] *= invDet;
        inverse.m_Data[ 6] *= invDet;
        inverse.m_Data[ 7] *= invDet;
        inverse.m_Data[ 8] *= invDet;
        inverse.m_Data[ 9] *= invDet;
        inverse.m_Data[10] *= invDet;
        inverse.m_Data[11] *= invDet;
        inverse.m_Data[12] *= invDet;
        inverse.m_Data[13] *= invDet;
        inverse.m_Data[14] *= invDet;
        inverse.m_Data[15] *= invDet;

        return inverse;
    }

    return ZERO;
}

template <typename Real>
Matrix4<Real> Matrix4<Real>::Adjoint () const
{
    Real a0 = m_Data[ 0]*m_Data[ 5] - m_Data[ 1]*m_Data[ 4];
    Real a1 = m_Data[ 0]*m_Data[ 6] - m_Data[ 2]*m_Data[ 4];
    Real a2 = m_Data[ 0]*m_Data[ 7] - m_Data[ 3]*m_Data[ 4];
    Real a3 = m_Data[ 1]*m_Data[ 6] - m_Data[ 2]*m_Data[ 5];
    Real a4 = m_Data[ 1]*m_Data[ 7] - m_Data[ 3]*m_Data[ 5];
    Real a5 = m_Data[ 2]*m_Data[ 7] - m_Data[ 3]*m_Data[ 6];
    Real b0 = m_Data[ 8]*m_Data[13] - m_Data[ 9]*m_Data[12];
    Real b1 = m_Data[ 8]*m_Data[14] - m_Data[10]*m_Data[12];
    Real b2 = m_Data[ 8]*m_Data[15] - m_Data[11]*m_Data[12];
    Real b3 = m_Data[ 9]*m_Data[14] - m_Data[10]*m_Data[13];
    Real b4 = m_Data[ 9]*m_Data[15] - m_Data[11]*m_Data[13];
    Real b5 = m_Data[10]*m_Data[15] - m_Data[11]*m_Data[14];

    return Matrix4<Real>
    (
        + m_Data[ 5]*b5 - m_Data[ 6]*b4 + m_Data[ 7]*b3,
        - m_Data[ 1]*b5 + m_Data[ 2]*b4 - m_Data[ 3]*b3,
        + m_Data[13]*a5 - m_Data[14]*a4 + m_Data[15]*a3,
        - m_Data[ 9]*a5 + m_Data[10]*a4 - m_Data[11]*a3,
        - m_Data[ 4]*b5 + m_Data[ 6]*b2 - m_Data[ 7]*b1,
        + m_Data[ 0]*b5 - m_Data[ 2]*b2 + m_Data[ 3]*b1,
        - m_Data[12]*a5 + m_Data[14]*a2 - m_Data[15]*a1,
        + m_Data[ 8]*a5 - m_Data[10]*a2 + m_Data[11]*a1,
        + m_Data[ 4]*b4 - m_Data[ 5]*b2 + m_Data[ 7]*b0,
        - m_Data[ 0]*b4 + m_Data[ 1]*b2 - m_Data[ 3]*b0,
        + m_Data[12]*a4 - m_Data[13]*a2 + m_Data[15]*a0,
        - m_Data[ 8]*a4 + m_Data[ 9]*a2 - m_Data[11]*a0,
        - m_Data[ 4]*b3 + m_Data[ 5]*b1 - m_Data[ 6]*b0,
        + m_Data[ 0]*b3 - m_Data[ 1]*b1 + m_Data[ 2]*b0,
        - m_Data[12]*a3 + m_Data[13]*a1 - m_Data[14]*a0,
        + m_Data[ 8]*a3 - m_Data[ 9]*a1 + m_Data[10]*a0
    );
}

template <typename Real>
Real Matrix4<Real>::Determinant () const
{
    Real a0 = m_Data[ 0]*m_Data[ 5] - m_Data[ 1]*m_Data[ 4];
    Real a1 = m_Data[ 0]*m_Data[ 6] - m_Data[ 2]*m_Data[ 4];
    Real a2 = m_Data[ 0]*m_Data[ 7] - m_Data[ 3]*m_Data[ 4];
    Real a3 = m_Data[ 1]*m_Data[ 6] - m_Data[ 2]*m_Data[ 5];
    Real a4 = m_Data[ 1]*m_Data[ 7] - m_Data[ 3]*m_Data[ 5];
    Real a5 = m_Data[ 2]*m_Data[ 7] - m_Data[ 3]*m_Data[ 6];
    Real b0 = m_Data[ 8]*m_Data[13] - m_Data[ 9]*m_Data[12];
    Real b1 = m_Data[ 8]*m_Data[14] - m_Data[10]*m_Data[12];
    Real b2 = m_Data[ 8]*m_Data[15] - m_Data[11]*m_Data[12];
    Real b3 = m_Data[ 9]*m_Data[14] - m_Data[10]*m_Data[13];
    Real b4 = m_Data[ 9]*m_Data[15] - m_Data[11]*m_Data[13];
    Real b5 = m_Data[10]*m_Data[15] - m_Data[11]*m_Data[14];
    Real det = a0*b5 - a1*b4 + a2*b3 + a3*b2 - a4*b1 + a5*b0;
    return det;
}

template <typename Real>
void Matrix4<Real>::MakeObliqueProjection (const Vector3<Real>& normal,
    const Vector3<Real>& origin, const Vector3<Real>& direction)
{
    // The projection plane is Dot(N,X-P) = 0 where N is a 3-by-1 unit-length
    // normal vector and P is a 3-by-1 point on the plane.  The projection
    // is oblique to the plane, in the direction of the 3-by-1 vector D.
    // Necessarily Dot(N,D) is not zero for this projection to make sense.
    // Given a 3-by-1 point U, compute the intersection of the line U+t*D
    // with the plane to obtain t = -Dot(N,U-P)/Dot(N,D).  Then
    //
    //   projection(U) = P + [I - D*N^T/Dot(N,D)]*(U-P)
    //
    // A 4-by-4 homogeneous transformation representing the projection is
    //
    //       +-                               -+
    //   M = | D*N^T - Dot(N,D)*I   -Dot(N,P)D |
    //       |          0^T          -Dot(N,D) |
    //       +-                               -+
    //
    // where M applies to [U^T 1]^T by M*[U^T 1]^T.  The matrix is chosen so
    // that M[3][3] > 0 whenever Dot(N,D) < 0 (projection is onto the
    // "positive side" of the plane).

    Real dotND = normal.Dot(direction);
    Real dotNO = normal.Dot(origin);
    m_Data[ 0] = direction.m_Data[0]*normal.m_Data[0] - dotND;
    m_Data[ 1] = direction.m_Data[0]*normal.m_Data[1];
    m_Data[ 2] = direction.m_Data[0]*normal.m_Data[2];
    m_Data[ 3] = -dotNO*direction.m_Data[0];
    m_Data[ 4] = direction.m_Data[1] * normal.m_Data[0];
    m_Data[ 5] = direction.m_Data[1] * normal.m_Data[1] - dotND;
    m_Data[ 6] = direction.m_Data[1] * normal.m_Data[2];
    m_Data[ 7] = -dotNO*direction.m_Data[1];
    m_Data[ 8] = direction.m_Data[2] * normal.m_Data[0];
    m_Data[ 9] = direction.m_Data[2] * normal.m_Data[1];
    m_Data[10] = direction.m_Data[2] * normal.m_Data[2] - dotND;
    m_Data[11] = -dotNO*direction.m_Data[2];
    m_Data[12] = 0.0f;
    m_Data[13] = 0.0f;
    m_Data[14] = 0.0f;
    m_Data[15] = -dotND;
}

template <typename Real>
void Matrix4<Real>::MakePerspectiveProjection (
    const Vector3<Real>& normal, const Vector3<Real>& origin,
    const Vector3<Real>& eye)
{
    //     +-                                                 -+
    // M = | Dot(N,E-P)*I - E*N^T    -(Dot(N,E-P)*I - E*N^T)*E |
    //     |        -N^t                      Dot(N,E)         |
    //     +-                                                 -+
    //
    // where E is the eye point, P is a point on the plane, and N is a
    // unit-length plane normal.

    Real dotND = normal.Dot(eye - origin);
    m_Data[ 0] = dotND - eye[0]*normal[0];
    m_Data[ 1] = -eye[0]*normal[1];
    m_Data[ 2] = -eye[0]*normal[2];
    m_Data[ 3] = -(m_Data[0]*eye[0] + m_Data[1]*eye[1] + m_Data[2]*eye[2]);
    m_Data[ 4] = -eye[1]*normal[0];
    m_Data[ 5] = dotND - eye[1]*normal[1];
    m_Data[ 6] = -eye[1]*normal[2];
    m_Data[ 7] = -(m_Data[4]*eye[0] + m_Data[5]*eye[1] + m_Data[6]*eye[2]);
    m_Data[ 8] = -eye[2]*normal[0];
    m_Data[ 9] = -eye[2]*normal[1];
    m_Data[10] = dotND- eye[2]*normal[2];
    m_Data[11] = -(m_Data[8]*eye[0] + m_Data[9]*eye[1] + m_Data[10]*eye[2]);
    m_Data[12] = -normal[0];
    m_Data[13] = -normal[1];
    m_Data[14] = -normal[2];
    m_Data[15] = normal.Dot(eye);
}

template <typename Real>
void Matrix4<Real>::MakeReflection (const Vector3<Real>& normal,
    const Vector3<Real>& origin)
{
    //     +-                         -+
    // M = | I-2*N*N^T    2*Dot(N,P)*N |
    //     |     0^T            1      |
    //     +-                         -+
    //
    // where P is a point on the plane and N is a unit-length plane normal.

    Real twoDotNO = ((Real)2)*(normal.Dot(origin));
    m_Data[ 0] = (Real)1 - ((Real)2)*normal[0]*normal[0];
    m_Data[ 1] = -((Real)2)*normal[0]*normal[1];
    m_Data[ 2] = -((Real)2)*normal[0]*normal[2];
    m_Data[ 3] = twoDotNO*normal[0];
    m_Data[ 4] = -((Real)2)*normal[1]*normal[0];
    m_Data[ 5] = (Real)1 - ((Real)2)*normal[1]*normal[1];
    m_Data[ 6] = -((Real)2)*normal[1]*normal[2];
    m_Data[ 7] = twoDotNO*normal[1];
    m_Data[ 8] = -((Real)2)*normal[2]*normal[0];
    m_Data[ 9] = -((Real)2)*normal[2]*normal[1];
    m_Data[10] = (Real)1 - ((Real)2)*normal[2]*normal[2];
    m_Data[11] = twoDotNO*normal[2];
    m_Data[12] = (Real)0;
    m_Data[13] = (Real)0;
    m_Data[14] = (Real)0;
    m_Data[15] = (Real)1;
}

template <typename Real>
inline Matrix4<Real> operator* (Real scalar, const Matrix4<Real>& mat)
{
    return mat*scalar;
}

template <typename Real>
inline Vector4<Real> operator* (const Vector4<Real>& vec,
    const Matrix4<Real>& mat)
{
    return Vector4<Real>(
        vec.m_Data[0]*mat.m_Matrix[0][0]+vec.m_Data[1]*mat.m_Matrix[1][0]+vec.m_Data[2]*mat.m_Matrix[2][0]+vec.m_Data[3]*mat.m_Matrix[3][0],
        vec.m_Data[0]*mat.m_Matrix[0][1]+vec.m_Data[1]*mat.m_Matrix[1][1]+vec.m_Data[2]*mat.m_Matrix[2][1]+vec.m_Data[3]*mat.m_Matrix[3][1],
        vec.m_Data[0]*mat.m_Matrix[0][2]+vec.m_Data[1]*mat.m_Matrix[1][2]+vec.m_Data[2]*mat.m_Matrix[2][2]+vec.m_Data[3]*mat.m_Matrix[3][2],
        vec.m_Data[0]*mat.m_Matrix[0][3]+vec.m_Data[1]*mat.m_Matrix[1][3]+vec.m_Data[2]*mat.m_Matrix[2][3]+vec.m_Data[3]*mat.m_Matrix[3][3]);
}

