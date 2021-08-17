/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <cmath>
#include <core/type/typetraits.hpp>
#include <math/mathexp.hpp>

#ifdef PI
#undef PI
#endif

namespace eXl
{

  template <class Real>
  struct PreciseType
  {
    typedef double type;
  };

  template <typename Real>
  class Math
  {
  public:

    static inline unsigned int PGCD(unsigned int iVal1, unsigned int iVal2)
    {
      if(iVal2 > iVal1)
        return PGCD(iVal2, iVal1);
      else if(iVal2 == 0)
        return iVal1;
      else
      {
        unsigned int rem = iVal1 % iVal2;
        return PGCD(iVal2, rem);
      }
    }

    static inline int Mod(int iNum, int iDiv)
    {
      int rem = iNum % iDiv;
      return rem >= 0 ? rem : rem + iDiv;
    }

    inline static Real Abs(Real iVal)   {return iVal > 0 ? iVal : -iVal;}
    inline static Real FAbs(Real iVal)  {return Abs(iVal);}
    inline static Real Cos(Real iAngle) {return std::cos(iAngle);}
    inline static Real Sin(Real iAngle) {return std::sin(iAngle);}
    inline static Real Tan(Real iAngle) {return std::tan(iAngle);}
    inline static Real Sqrt(Real iVal)  {return std::sqrt(iVal);}
    inline static Real ACos(Real iValue) {return std::acos(iValue);}
    inline static Real ASin(Real iValue) {return std::asin(iValue);}
    inline static Real ATan(Real iValue) {return std::atan(iValue);}
    inline static Real Clamp(Real iValue, Real iMin, Real iMax) 
    {
      return iValue < iMin ? iMin : (iValue > iMax ? iMax : iValue);
    }

    inline static Real Min(Real iVal1, Real iVal2) {return iVal1 > iVal2 ? iVal2 : iVal1;}
    inline static Real Max(Real iVal1, Real iVal2) {return iVal1 > iVal2 ? iVal1 : iVal2;}

    inline static Real Floor(Real iVal1){return std::floor(iVal1);}
    inline static Real Ceil(Real iVal1){return std::ceil(iVal1);}
    inline static Real Sign(Real iVal1){return std::signbit(iVal1) ? -1.0 : 1.0;}
    
    inline static Real Round(float iVal){return iVal;}
    inline static Real Round(double iVal){return iVal;}
    inline static Real SCurve3(Real a)
    {
      return (a * a * (3.0 - 2.0 * a));
    }

    inline static Real Sigmoid(Real a)
    {
      return 1.0 / (1.0 + exp(-a));
    }

    inline static typename PreciseType<Real>::type ToPrecise(Real iVal){return static_cast<typename PreciseType<Real>::type>(iVal);}

    static const Real PI;
    static const Real ZERO_TOLERANCE;
    static const Real EPSILON;
    static const Real MAX_REAL;

  private:

    static Real GetPI();
    static Real GetZERO_TOLERANCE();
    static Real GetEPSILON();
    static Real GetMAX_REAL();
  };

  template <>
  inline int Math<int>::Round(float iVal)
  {
	  return (int)(iVal + 0.49999997f);
  }

  template <>
  inline int Math<int>::Round(double iVal)
  {
	  return int(round(iVal));
  }

  template <>
  inline int Math<int>::Sign(int iVal)
  {
	  return iVal > 0 ? 1 : -1;
  }

  template <>
  inline unsigned int Math<unsigned int>::Sign(unsigned int iVal)
  {
	  return 1;
  }

  typedef Math<int>    Mathi;
  typedef Math<float>  Mathf;
  typedef Math<double> Mathd;

  template <typename Real>
  class AABB2D;
}

#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/matrix4.hpp>
#include <math/quaternion.hpp>
#include <math/aabb2d.hpp>
#include <math/segment.hpp>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#define DEFINE_MATH_TYPE_EX(type, friendlyname) DEFINE_TYPE_EX(type, friendlyname, EXL_MATH_API)
#define DEFINE_MATH_TYPE(type) DEFINE_MATH_TYPE_EX(type, type)

namespace eXl
{
  DEFINE_MATH_TYPE(Vector4f)
  DEFINE_MATH_TYPE(Vector3f)
  DEFINE_MATH_TYPE(Vector2f)
  DEFINE_MATH_TYPE(Vector4i)
  DEFINE_MATH_TYPE(Vector3i)
  DEFINE_MATH_TYPE(Vector2i)
  DEFINE_MATH_TYPE(Quaternionf)
  DEFINE_MATH_TYPE(Matrix4f)

  DEFINE_MATH_TYPE_EX(AABB2D<int>, AABB2Di)
  DEFINE_MATH_TYPE_EX(AABB2D<float>, AABB2Df)

  template <>
  struct TypeTraits::IsComparable<Vector4f>{static constexpr bool s_Value = false;};

  template <>
  struct TypeTraits::IsComparable<Vector4i>{static constexpr bool s_Value = false;};

  template <>
  struct TypeTraits::IsComparable<Matrix4f>{static constexpr bool s_Value = false;};

  template <typename Real>
  struct StreamerTemplateHandler<Vector4<Real> >
  {
    static Err Do(Streamer& iStreamer, Vector4<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(iObj->m_Data + 2);
      iStreamer.PopKey();
      iStreamer.PushKey("W");
      iStreamer.Write(iObj->m_Data + 3);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<Vector3<Real> > 
  {
    static Err Do(Streamer& iStreamer, Vector3<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(iObj->m_Data + 2);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<Vector2<Real> >
  {
    static Err Do(Streamer& iStreamer, Vector2<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Vector4<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Vector4<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(iObj->m_Data + 2);
      iStreamer.PopKey();
      iStreamer.PushKey("W");
      iStreamer.Read(iObj->m_Data + 3);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Vector3<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Vector3<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(iObj->m_Data + 2);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Vector2<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Vector2<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<Quaternion<Real> >
  {
    static Err Do(Streamer& iStreamer, Quaternion<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("W");
      iStreamer.Write(&iObj->m_W);
      iStreamer.PopKey();
      iStreamer.PushKey("X");
      iStreamer.Write(&iObj->m_X);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(&iObj->m_Y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(&iObj->m_Z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Quaternion<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Quaternion<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("W");
      iStreamer.Read(&iObj->m_W);
      iStreamer.PopKey();
      iStreamer.PushKey("X");
      iStreamer.Read(&iObj->m_X);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(&iObj->m_Y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(&iObj->m_Z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<Matrix4<Real> >
  {
    static Err Do(Streamer& iStreamer, Matrix4<Real> const* iObj)
    {
      iStreamer.BeginSequence();
      for(unsigned int i = 0; i<16; ++i)
      {
        iStreamer.Write(iObj->m_Data + i);
      }
      iStreamer.EndSequence();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Matrix4<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Matrix4<Real>* iObj)
    {
      iStreamer.BeginSequence();
      for(unsigned int i = 0; i<16; ++i)
      {
        iStreamer.Read(iObj->m_Data + i);
        iStreamer.NextSequenceElement();
      }
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<Segment<Real> >
  {
    static Err Do(Streamer& iStreamer, Segment<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Ext1");
      iStreamer.Write(&iObj->m_Ext1);
      iStreamer.PopKey();
      iStreamer.PushKey("Ext2");
      iStreamer.Write(&iObj->m_Ext2);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<Segment<Real> >
  {
    static Err Do(Unstreamer& iStreamer, Segment<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Ext1");
      iStreamer.Read(&iObj->m_Ext1);
      iStreamer.PopKey();
      iStreamer.PushKey("Ext2");
      iStreamer.Read(&iObj->m_Ext2);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

}