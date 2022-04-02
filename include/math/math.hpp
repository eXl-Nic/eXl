/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cmath>
#include <core/type/typetraits.hpp>
#include <math/mathexp.hpp>

#include <glm/ext.hpp>

#ifdef PI
#undef PI
#endif

namespace eXl
{
  using Vec2i = glm::ivec2;
  using Vec3i = glm::ivec3;
  using Vec4i = glm::ivec4;
  using Vec2u = glm::uvec2;
  using Vec2 = glm::vec2;
  using Vec3 = glm::vec3;
  using Vec4 = glm::vec4;
  using Mat4 = glm::mat4;
  using Vec2d = glm::dvec2;
  using Vec3d = glm::dvec3;
  using Vec4d = glm::dvec4;
  using Mat4d = glm::dmat4;
  using Quaternion = glm::quat;

  template<typename T>
  constexpr T Zero()
  {
    return glm::zero<T>();
  }

  template<typename T>
  constexpr T One()
  {
    return glm::one<T>();
  }

  template<typename T>
  constexpr T Identity()
  {
    return glm::identity<T>();
  }

  template<typename T>
  typename T::value_type NormalizeAndGetLength(T& iVal)
  {
    typename T::value_type len = length(iVal);
    iVal *= T(1) / len;
    return len;
  }

  template<glm::length_t L, typename T, glm::qualifier Q>
  inline bool LexicographicCompare(glm::vec<L,T,Q> const& iVal1, glm::vec<L,T,Q> const& iVal2)
  {
    auto comp = glm::lessThan(iVal1, iVal2);
    for (uint32_t i = 0; i < L; ++i)
    {
      if (comp[i])
      {
        return true;
      }
    }

    return false;
  }

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

    static constexpr Real Pi()
    {
      return glm::pi<Real>();
    }

    static constexpr Real ZeroTolerance()
    {
      return 0;
    }

    static constexpr Real Epsilon()
    {
      return glm::epsilon<Real>();
    }

    static constexpr Real MaxReal()
    {
      return std::numeric_limits<Real>().max();
    }
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

  template <>
  inline constexpr float Math<float>::ZeroTolerance()
  {
    return 1e-05f;
  }

  template <>
  inline constexpr double Math<double>::ZeroTolerance()
  {
    return 1e-08f;
  }

  typedef Math<int>    Mathi;
  typedef Math<float>  Mathf;
  typedef Math<double> Mathd;

  template <typename Real>
  class AABB2D;
}

#include <math/aabb2d.hpp>
#include <math/segment.hpp>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#define DEFINE_MATH_TYPE_EX(type, friendlyname) DEFINE_TYPE_EX(type, friendlyname, EXL_MATH_API)
#define DEFINE_MATH_TYPE(type) DEFINE_MATH_TYPE_EX(type, type)

namespace eXl
{
  DEFINE_MATH_TYPE(Vec4)
  DEFINE_MATH_TYPE(Vec3)
  DEFINE_MATH_TYPE(Vec2)
  DEFINE_MATH_TYPE(Vec4i)
  DEFINE_MATH_TYPE(Vec3i)
  DEFINE_MATH_TYPE(Vec2i)
  DEFINE_MATH_TYPE(Quaternion)
  DEFINE_MATH_TYPE(Mat4)

  DEFINE_MATH_TYPE_EX(AABB2D<int>, AABB2Di)
  DEFINE_MATH_TYPE_EX(AABB2D<float>, AABB2Df)

  template <>
  struct TypeTraits::IsComparable<Vec3>{static constexpr bool s_Value = false;};

  template <>
  struct TypeTraits::IsComparable<Vec4>{static constexpr bool s_Value = false;};

  template <>
  struct TypeTraits::IsComparable<Mat4>{static constexpr bool s_Value = false;};

  template <typename Real>
  struct StreamerTemplateHandler<glm::vec<4,Real> >
  {
    static Err Do(Streamer& iStreamer, glm::vec<4,Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(iObj->z);
      iStreamer.PopKey();
      iStreamer.PushKey("W");
      iStreamer.Write(iObj->w);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<glm::vec<3,Real> > 
  {
    static Err Do(Streamer& iStreamer, glm::vec<3,Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(iObj->z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<glm::vec<2,Real> >
  {
    static Err Do(Streamer& iStreamer, glm::vec<2,Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Write(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(&iObj->y);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<glm::vec<4,Real> >
  {
    static Err Do(Unstreamer& iStreamer, glm::vec<4,Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(&iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(&iObj->z);
      iStreamer.PopKey();
      iStreamer.PushKey("W");
      iStreamer.Read(&iObj->w);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<glm::vec<3,Real> >
  {
    static Err Do(Unstreamer& iStreamer, glm::vec<3,Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(&iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(&iObj->z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<glm::vec<2,Real> >
  {
    static Err Do(Unstreamer& iStreamer, glm::vec<2,Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("X");
      iStreamer.Read(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(&iObj->y);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <>
  struct StreamerTemplateHandler<Quaternion >
  {
    static inline Err Do(Streamer& iStreamer, Quaternion const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("W");
      iStreamer.Write(&iObj->w);
      iStreamer.PopKey();
      iStreamer.PushKey("X");
      iStreamer.Write(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Write(&iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Write(&iObj->z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <>
  struct UnstreamerTemplateHandler<Quaternion >
  {
    static inline Err Do(Unstreamer& iStreamer, Quaternion* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("W");
      iStreamer.Read(&iObj->w);
      iStreamer.PopKey();
      iStreamer.PushKey("X");
      iStreamer.Read(&iObj->x);
      iStreamer.PopKey();
      iStreamer.PushKey("Y");
      iStreamer.Read(&iObj->y);
      iStreamer.PopKey();
      iStreamer.PushKey("Z");
      iStreamer.Read(&iObj->z);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct StreamerTemplateHandler<glm::mat<4,4,Real> >
  {
    static Err Do(Streamer& iStreamer, glm::mat<4, 4, Real> const* iObj)
    {
      iStreamer.BeginSequence();
      for(unsigned int i = 0; i<16; ++i)
      {
        iStreamer.Write(value_ptr(*iObj) + i);
      }
      iStreamer.EndSequence();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<glm::mat<4, 4, Real> >
  {
    static Err Do(Unstreamer& iStreamer, glm::mat<4, 4, Real>* iObj)
    {
      iStreamer.BeginSequence();
      for(unsigned int i = 0; i<16; ++i)
      {
        iStreamer.Read(value_ptr(*iObj) + i);
        iStreamer.NextSequenceElement();
      }
      return Err::Success;
    }
  };

  template<typename T>
  struct Segment;

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

  template <typename Real>
  struct StreamerTemplateHandler<AABB2D<Real> >
  {
    static Err Do(Streamer& iStreamer, AABB2D<Real> const* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Min");
      iStreamer.Write(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Max");
      iStreamer.Write(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

  template <typename Real>
  struct UnstreamerTemplateHandler<AABB2D<Real> >
  {
    static Err Do(Unstreamer& iStreamer, AABB2D<Real>* iObj)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Min");
      iStreamer.Read(iObj->m_Data);
      iStreamer.PopKey();
      iStreamer.PushKey("Max");
      iStreamer.Read(iObj->m_Data + 1);
      iStreamer.PopKey();
      iStreamer.EndStruct();
      return Err::Success;
    }
  };

}

namespace glm
{
  template<glm::length_t L, typename T, glm::qualifier Q>
  inline size_t hash_value(glm::vec<L, T, Q> const& iVal)
  {
    size_t hash = 0;
    for (uint32_t i = 0; i < L; ++i)
    {
      boost::hash_combine(hash, iVal[i]);
    }
    return hash;
  }
}