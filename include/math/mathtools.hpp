/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/random.hpp>
#include <math/polygon.hpp>
#include <math/aabb2dpolygon.hpp>
#include <math/math.hpp>

#include <boost/random.hpp>
#include <cstdint>


namespace eXl
{
  class Serializer;

  class MathTools
  {
  public:

    //template <typename Real>
    //static Err Stream(glm::vec<2,Real> const& iPoint, Serializer iStreamer);

    //template <typename Real>
    //static Err Unstream(glm::vec<2,Real>& oPoint, Unstreamer& iUnstreamer);

    //template <typename Real>
    //static Err Stream(Polygon<Real> const& iPolygon, Streamer& iStreamer);
    //
    //template <typename Real>
    //static Err Unstream(Polygon<Real>& oPolygon, Unstreamer& iUnstreamer);
    //
    //template <typename Real>
    //static Err Stream(AABB2DPolygon<Real> const& iPolygon, Streamer& iStreamer);
    //
    //template <typename Real>
    //static Err Unstream(AABB2DPolygon<Real>& oPolygon, Unstreamer& iUnstreamer);

    //Clamp angle between 0 and 2*PI
    inline static float ClampAngle(float iAngle)
    {
      int div = iAngle / (Mathf::Pi() * 2);
      if(iAngle < 0)
      {
        --div;
      }
      return iAngle - div * Mathf::Pi() * 2;
    }

    // AngleDist between -PI and PI
    inline static float AngleDist(float iAngle1, float iAngle2)
    {
      float dist = ClampAngle(iAngle1) - ClampAngle(iAngle2);
      if(dist > Mathf::Pi())
      {
        return 2.0 * Mathf::Pi() - dist;
      }
      else if(dist < -Mathf::Pi())
      {
        return dist + 2.0 * Mathf::Pi();
      }
      return dist;
    }


    template <typename Real>
    static inline glm::vec<2,Real> Perp(glm::vec<2,Real> const& iVec)
    {
      return glm::vec<2,Real>(-iVec.y, iVec.x);
    }

    static inline uint64_t SquaredLength(Vec2i const& iVect)
    {
      return (int64_t)iVect.x * (int64_t)iVect.x + (int64_t)iVect.y * (int64_t)iVect.y;
    }

    template <typename Real>
    static inline Vec2i ToIVec(glm::vec<2,Real> const& iVec)
    {
      return Vec2i(Math<Real>::Round(iVec.x), Math<Real>::Round(iVec.y));
    }

    template <typename Real>
    static inline Vec2 ToFVec(glm::vec<2,Real> const& iVec)
    {
      return Vec2(iVec.x, iVec.y);
    }

    template <typename Real>
    static inline Vec2d ToDVec(glm::vec<2,Real> const& iVec)
    {
      return Vec2d(iVec.x, iVec.y);
    }

    template <typename Real>
    static inline Real Sign(Real iValue)
    {
      return iValue > 0 ? 1 : -1;
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(glm::vec<2,Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, unsigned int &oSegNum, glm::vec<2,Real1>& oPoint, glm::vec<2,Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      glm::vec<2,Real2> dummy;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, dummy, oSegNum, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(glm::vec<2,Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, glm::vec<2,Real2> (&oSeg)[2], glm::vec<2,Real1>& oPoint, glm::vec<2,Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      unsigned int dummy;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, oSeg, dummy, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(glm::vec<2,Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, glm::vec<2,Real2> (&oSeg)[2], unsigned int& oSegNum, glm::vec<2,Real1>& oPoint, glm::vec<2,Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, oSeg, oSegNum, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(glm::vec<2,Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, glm::vec<2,Real2> (&oSeg)[2], unsigned int& oSegNum, int& oHole, glm::vec<2,Real1>& oPoint, glm::vec<2,Real1>& oDir, bool iCheckExt = true, bool iCheckHoles = false)
    {
      Real1 minDist;
      oHole = -1;
      bool res = _NearestPointOnPoly(iPoint, iPoly.Border(), iMinSegLen, oSeg, oSegNum, oPoint, oDir, minDist, iCheckExt);
      if(iCheckHoles)
      {
        for(unsigned int i = 0 ; i< iPoly.Holes().size(); ++i)
        {
          unsigned int segNum;
          glm::vec<2,Real1> point;
          glm::vec<2,Real2> seg[2];
          glm::vec<2,Real1>    dir;
          Real1 curMin;
          bool newRes = _NearestPointOnPoly(iPoint, iPoly.Holes()[i], iMinSegLen, seg, segNum, point, dir, curMin, iCheckExt);
          if(newRes)
          {
            if(!res || curMin < minDist)
            {
              oHole = i;
              res = newRes;
              oSegNum = segNum;
              oPoint = point;
              oDir = dir;
              oSeg[0] = seg[0];
              oSeg[1] = seg[1];
              minDist = curMin;
            }
          }
        }
      }
      return res;
    }

    template <typename Real1, typename Real2>
    static inline bool _NearestPointOnPoly(glm::vec<2,Real1> const& iPoint, Vector<glm::vec<2,Real2> > const& polyBorder, Real1 iMinSegLen, glm::vec<2,Real2> (&oSeg)[2], unsigned int& oSegNum, glm::vec<2,Real1>& oPoint, glm::vec<2,Real1>& oDir, Real1& oDist, bool iCheckExt)
    {
      glm::vec<2,Real1> polyCenter;
      for(auto point : polyBorder)
      {
        polyCenter += glm::vec<2,Real1>(point.x, point.y);
      }

      if(polyBorder.front() == polyBorder.back())
      {
        polyCenter -= glm::vec<2,Real1>(polyBorder.front().x, polyBorder.front().y);
        polyCenter = polyCenter / (polyBorder.size() - 1);
      }
      else
      {
        polyCenter = polyCenter / polyBorder.size();
      }

      //glm::vec<2,Real1> iDir = polyCenter - iPoint;
      //Real1 minDist = iDir.Normalize();

      glm::vec<2,Real1> prevPoint(polyBorder.back().x, polyBorder.back().y);

      glm::vec<2,Real1> iDir = polyCenter - iPoint;
      iDir.Normalize();
      Real1 minDist = (iPoint - prevPoint).Length() + 1;

      for (unsigned int i = 0; i < polyBorder.size(); ++i)
      {
        glm::vec<2,Real1> curPoint(polyBorder[i].x, polyBorder[i].y);

        if (curPoint != prevPoint)
        {
          glm::vec<2,Real1> dir = curPoint - prevPoint;
          glm::vec<2,Real1> extDir(MathTools::Perp(dir));

          if (!iCheckExt || (iDir).Dot(extDir) < 0)
          {
            Real1 segLen = dir.Normalize();
            if (segLen > iMinSegLen)
            {
              glm::vec<2,Real1> prevPointOff = prevPoint + dir * 0.5 * iMinSegLen;
              glm::vec<2,Real1> curPointOff = curPoint - dir * 0.5 * iMinSegLen;
              segLen -= iMinSegLen;
              Real1 dirPoj = (iPoint - prevPointOff).Dot(dir);
              glm::vec<2,Real1> candidate;

              if (dirPoj < -Math<Real1>::ZeroTolerance())
              {
                candidate = prevPointOff;
              }
              else if (dirPoj > segLen - Math<Real1>::ZeroTolerance())
              {
                candidate = curPointOff;
              }
              else
              {
                candidate = prevPointOff + (dir * dirPoj);
              }

              Real1 curDist = (candidate - iPoint).Length();
              if (curDist < minDist)
              {
                oSeg[0] = glm::vec<2,Real2>(prevPoint.x, prevPoint.y);
                oSeg[1] = glm::vec<2,Real2>(curPoint.x, curPoint.y);
                if(i == 0)
                  oSegNum = polyBorder.size() - 1;
                else
                  oSegNum = i - 1;
                oPoint = candidate;
                oDir = extDir;
                oDir.Normalize();
                minDist = curDist;
              }
            }
          }
        }

        prevPoint = curPoint;
      }

      oDist = minDist;

      if(minDist != (iPoint - polyCenter).Length())
        return true;
      else
        return false;
    }

    template <typename Real>
    static void SimplifyPolygon(Polygon<Real> const& iPoly1, double iEpsilon, Polygon<Real>& oPoly);

    template <typename Real1, typename Real2>
    static inline unsigned int FindPolyIntersection(glm::vec<2,Real1> const& iPoint1, glm::vec<2,Real1> const& iPoint2, Polygon<Real2> const& iPoly, glm::vec<2,Real1>(&oPoints)[2])
    {
      unsigned int curInter = 0;
      Vector<glm::vec<2,Real2> > const& border = iPoly.Border();
      glm::vec<2,Real1> prevPoint(border.back().x, border.back().y);
      for (unsigned int i = 0; i < border.size() && curInter < 2; ++i)
      {
        glm::vec<2,Real1> curPoint(border[i].x, border[i].y);
            
        unsigned int res = Segment<Real1>::Intersect(prevPoint, curPoint, iPoint1, iPoint2, oPoints[curInter]);
        if (res & Segmentd::PointOnSegment1)
        {
          if ((curInter == 0 || (oPoints[0] - oPoints[curInter]).Length() > Math<Real1>::ZeroTolerance()))
          {
            ++curInter;
          }
        }
        prevPoint = curPoint;
      }
      return curInter;
    }

    template <typename Real1, typename Real2>
    static bool FindCommonSegment(Polygon<Real2> const& iPoly1, Polygon<Real2> const& iPoly2, glm::vec<2,Real1>& oSegPt1, glm::vec<2,Real1>& oSegPt2, Real1 iMinLen = Math<Real1>::ZeroTolerance())
    {
      bool foundCommon = false;
      glm::vec<2,Real1> prevPt1 = glm::vec<2,Real1>(iPoly1.Border().back().x,iPoly1.Border().back().y);
      for(auto curPt1I : iPoly1.Border())
      {
        glm::vec<2,Real1> curPt1 = glm::vec<2,Real1>(curPt1I.x,curPt1I.y);
        if(prevPt1 != curPt1)
        {
          glm::vec<2,Real1> prevPt2 = glm::vec<2,Real1>(iPoly2.Border().back().x, iPoly2.Border().back().y);
          for(auto curPt2I : iPoly2.Border())
          {
            glm::vec<2,Real1> curPt2 = glm::vec<2,Real1>(curPt2I.x,curPt2I.y);
            if(prevPt2 != curPt2)
            {
                unsigned int res = Segment<Real1>::Intersect(prevPt1, curPt1, prevPt2, curPt2);
                if(res & Segment<Real1>::AlignedSegments
                && (res & Segment<Real1>::ConfoundSegments || res & Segmentd::PointOnSegments))
                {
                  glm::vec<2,Real1> dir = curPt1 - prevPt1;
                  AABB2D<Real1> box1(0.0,0.0, dir.Normalize(), 0.0);
                  double val1 = (prevPt2 - prevPt1).Dot(dir);
                  double val2 = (curPt2 - prevPt1).Dot(dir);
                  AABB2D<Real1> box2 = val1 > val2 ? AABB2D<Real1>(val2, 0.0, val1, 0.0) : AABB2D<Real1>(val1, 0.0, val2, 0.0);
                  AABB2D<Real1> result;
                  result.SetCommonBox(box1, box2);
                  double commonLen = result.m_Data[1].x - result.m_Data[0].x;
                  if(commonLen >= iMinLen)
                  {
                    foundCommon = true;
                    oSegPt1 = prevPt1 + dir * result.m_Data[0].x;
                    oSegPt2 = prevPt1 + dir * (result.m_Data[0].x + commonLen);
                    break;
                  }
                }
            }
            prevPt2 = curPt2;
          }
        }
        if(foundCommon)
          break;
        prevPt1 = curPt1;
      }
      return foundCommon;
    }

    template <typename Real>
    static inline Real GetAngleFromVec(glm::vec<2,Real> const& iDir)
    {
      return Math<Real>::Abs(iDir.x) < Math<Real>::Epsilon() ? (iDir.y > 0.0 ? Math<Real>::PI * 0.5 : Math<Real>::PI * 1.5) 
        : (iDir.x > 0.0 ? Math<Real>::ATan(iDir.y / iDir.x) : Math<Real>::PI - Math<Real>::ATan(iDir.y / (-1.0 * iDir.x)));
    }

    template<typename Real>
    static inline glm::vec<2,Real> GetPerp(glm::vec<2,Real> const& iVec)
    {
      return glm::vec<2,Real>(iVec.y * -1.0, iVec.x);
    }

    template<typename Real>
    static inline glm::vec<2,Real> GetLocal(glm::vec<2,Real> const& iBase, glm::vec<2,Real> const& iVec)
    {
      return glm::vec<2,Real>(iBase.x * iVec.x + iBase.y * iVec.y, iBase.x * iVec.y - iBase.y * iVec.x);
    }

    template<typename Real>
    static inline Vec2i RoundVector(glm::vec<2,Real> const& iVec)
    {
      return Vec2i(Math<Real>::Round(iVec.x), Math<Real>::Round(iVec.y));
    }

    template <typename Real>
    static inline Real ModAngle(Real iAngle)
    {
      iAngle = fmod(iAngle, 2.0*Math<Real>::PI);
      iAngle -= Mathf::Pi();
      return iAngle;
    }

    template <typename Real>
    static inline Real RandFloatIn(Random& randGen, Real iMin, Real iMax)
    {
      return Real(randGen() % 10000) / 10000.0 * (iMax - iMin) + iMin;
    };

    template <typename Real>
    static inline Real RandNormFloatIn(Random& randGen, Real iMin, Real iMax)
    {
      boost::random::normal_distribution<Real> distrib;
      RandomWrapper randW(&randGen);
      Real rangeValNorm = distrib(randW);
      Real rangeVal = rangeValNorm * ((iMax-iMin) * 0.33);
      Real offsetVal = rangeVal + (iMax + iMin) * 0.5;

      return Math<Real>::Clamp( offsetVal , iMin, iMax);
    };

    static inline int RandIntIn(Random& randGen, int iMin, int iMax)
    {
      return (randGen() % (iMax - iMin)) + iMin;
    };

    static inline Vec2 Perturbate(Random& iRand, Vec2 const& iPos, float iRadius)
    {
      float const angle = ((iRand.Generate() % 10000) / 10000.0 - 1.0) * Mathf::Pi();
      float const dist = ((iRand.Generate() % 10000) / 10000.0 ) * iRadius;

      return iPos + Vec2(Mathd::Cos(angle), Mathd::Sin(angle)) * dist;
    }

    template <typename Real>
    static inline glm::vec<2,Real> const& As2DVec(glm::vec<3,Real> const& iVec)
    {
      return reinterpret_cast<glm::vec<2,Real> const&>(iVec);
    }

    template <typename Real>
    static inline glm::vec<2,Real>& As2DVec(glm::vec<3,Real>& iVec)
    {
      return reinterpret_cast<glm::vec<2,Real>&>(iVec);
    }

    // Prereq : points have to be in CCW order
    template <typename Real>
    static glm::vec<2,Real> ConeGetMidSegment(glm::vec<2,Real> const (&iRange)[2])
    {
      glm::vec<2,Real> mid = iRange[0] + iRange[1];
      Real dist = NormalizeAndGetLength(mid);
      if (dist < Math<Real>::ZeroTolerance())
      {
        mid = GetPerp(iRange[0]);
      }
      mid *= Math<Real>::Sign(Segment<Real>::Cross(iRange[0], mid));

      return mid;
    }

    template <typename Real>
    static void ConeUpdateRange(glm::vec<2,Real> const (&iRange)[2], glm::vec<2,Real>& oMid, Real& oLowLimit)
    {
      oMid = ConeGetMidSegment(iRange);
      oLowLimit = dot(iRange[0], oMid) / length(iRange[0]);
    }

    template <typename Real>
    static bool IsInCone(glm::vec<2,Real> const& iDir, glm::vec<2,Real> const& iMidSeg, Real iLowLimit, Real iEpsilon = Math<Real>::Epsilon())
    {
      return dot(iDir,iMidSeg) > (iLowLimit - iEpsilon);
    }

    template <typename Real>
    static glm::vec<3,Real> Reflect(glm::vec<3,Real> const& iIncomingDir, glm::vec<3,Real> const& iNormal)
    {
      return iIncomingDir - iNormal * dot(iIncomingDir, iNormal) * 2;
    }

    template <typename Real>
    static void ClampToBox(glm::vec<2,Real>& ioPoint, AABB2D<Real> const& iBox)
    {
      for (uint32_t i = 0; i < 2; ++i)
      {
        ioPoint.m_Data[i] = Math<Real>::Clamp(ioPoint.m_Data[i], iBox.m_Data[0].m_Data[i], iBox.m_Data[1].m_Data[i]);
      }
    }

    //static void AlignVector(Vec2d& ioDir, Vec2i& oCanonicalVector, unsigned int iFactor)
    //{
    //  if (Mathd::Abs(ioDir.x) > Mathd::ZeroTolerance())
    //  {
    //    if (Mathd::Abs(ioDir.y) > Mathd::ZeroTolerance())
    //    {
    //      double ratio = (ioDir.x / ioDir.y));
    //      int sign = Sign(ratio);
    //      ratio = ratio * sign;
    //      if (ratio > 1)
    //      {
    //        ratio = 1 / ratio;
    //        unsigned int ratioI = Mathi::Round(ratio * iFactor);
    //        oCanonicalVector = Vec2i(Sign(ioDir.x) * iFactor, Sign(ioDir.y) * ratioI);
    //      }
    //      else
    //      {
    //        unsigned int ratioI = Mathi::Round(ratio * iFactor);
    //        oCanonicalVector = Vec2i(Sign(ioDir.x) * ratioI, Sign(ioDir.y) * iFactor);
    //      }
    //      ioDir = MathTools::ToDVec(oCanonicalVector) * ioDir.Length() / Mathd::Sqrt(ratioI*ratioI + iFactor*iFactor);
    //      return;
    //    }
    //  }
    //  ioDir = Vec2d::ZERO;
    //  oCanonicalVector = zero<Vec2i>();
    //}
    //
    //static void AlignSegment(Vec2d& ioPoint1, Vec2d ioPoint2, unsigned int iFactor)
    //{
    //  
    //}

  };

  //template <>
  //EXL_MATH_API Err MathTools::Stream<float>(Vec2 const& iPoint, Serializer iStreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Unstream<float>(Vec2& oPoint, Unstreamer& iUnstreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Stream<int>(Vec2i const& iPoint, Serializer iStreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Unstream<int>(Vec2i& oPoint, Unstreamer& iUnstreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Stream<int>(Polygoni const& iPoint, Streamer& iStreamer);
  //
  //template <>
  //EXL_MATH_API Err MathTools::Unstream<int>(Polygoni& oPoint, Unstreamer& iUnstreamer);
  //
  //template <>
  //EXL_MATH_API Err MathTools::Stream<int>(AABB2DPolygoni const& iPoint, Streamer& iStreamer);
  //
  //template <>
  //EXL_MATH_API Err MathTools::Unstream<int>(AABB2DPolygoni& oPoint, Unstreamer& iUnstreamer);

  template <>
  EXL_MATH_API void MathTools::SimplifyPolygon<int>(Polygoni const& iPoly1, double iEpsilon, Polygoni& oPoly);
  //template <>
  //EXL_GEN_API void MathTools::SimplifyPolygon<float>(Polygonf const& iPoly1, double iEpsilon, Polygonf& oPoly);
  //template <>
  //EXL_GEN_API void MathTools::SimplifyPolygon<double>(Polygond const& iPoly1, double iEpsilon, Polygond& oPoly);

  //template <typename Real>
  //struct StreamerTemplateHandler<Polygon<Real>>
  //{
  //  static Err Do(Streamer& iStreamer, Polygon<Real> const* iObj)
  //  {
  //    return MathTools::Stream<Real>(*iObj, iStreamer);
  //  }
  //};
  //
  //template <typename Real>
  //struct UnstreamerTemplateHandler<Polygon<Real>>
  //{
  //  static Err Do(Unstreamer& iUnstreamer, Polygon<Real>* iObj)
  //  {
  //    return MathTools::Unstream<Real>(*iObj, iUnstreamer);
  //  }
  //};

  EXL_MATH_API uint32_t Factorial(uint32_t i);

  struct EXL_MATH_API CombinationHelper_Iter
  {
    // Compute K amongst N with as an iterator adaptor.
    CombinationHelper_Iter(uint32_t iSize, uint32_t iNum, bool iIsEnd);

    void ComputeMaxStep(uint32_t iSize, uint32_t iNum);

    Vector<uint32_t> const& operator*() const { return m_Stack; }

    CombinationHelper_Iter& operator ++()
    {
      Advance();
      return *this;
    }

    bool operator == (CombinationHelper_Iter const& iOther) const
    {
      return m_Step == iOther.m_Step;
    }
    bool operator != (CombinationHelper_Iter const& iOther) const
    {
      return !(*this == iOther);
    }

    void Advance();

    Vector<uint32_t> m_Stack;
    uint32_t m_Size;
    uint32_t m_Num;
    uint32_t m_Step;
    uint32_t m_MaxStep;
  };

  struct CombinationHelper
  {
    CombinationHelper(uint32_t iSize, uint32_t iNum)
      : m_Size(iSize)
      , m_Num(iNum)
    {

    }

    CombinationHelper_Iter begin() const { return CombinationHelper_Iter(m_Size, m_Num, false); }
    CombinationHelper_Iter end() const { return CombinationHelper_Iter(m_Size, m_Num, true); }

    uint32_t m_Size;
    uint32_t m_Num;
  };

  template <typename T>
  struct TCombinationHelper_Set_Iter
  {
    TCombinationHelper_Set_Iter(Vector<T> const& iElements, CombinationHelper_Iter const& iIter, bool iEnd)
      : m_Elements(iElements)
      , m_Iter(iIter)
    {
      if (iEnd)
      {
        m_Cur = iIter.m_Num;
      }
      else
      {
        m_Cur = 0;
      }
    }

    T const& operator*()
    {
      return m_Elements[(*m_Iter)[m_Cur]];
    }

    TCombinationHelper_Set_Iter& operator ++()
    {
      ++m_Cur;
      return *this;
    }

    bool operator == (TCombinationHelper_Set_Iter const& iOther) const
    {
      return m_Cur == iOther.m_Cur;
    }
    bool operator != (TCombinationHelper_Set_Iter const& iOther) const
    {
      return !(*this == iOther);
    }

    uint32_t m_Cur;
    Vector<T> const& m_Elements;
    CombinationHelper_Iter const& m_Iter;
  };

  template <typename T>
  struct TCombinationHelper_Set
  {
    TCombinationHelper_Set(Vector<T> const& iElements, CombinationHelper_Iter const& iIter)
      : m_Elements(iElements)
      , m_Iter(iIter)
    {

    }

    TCombinationHelper_Set_Iter<T> begin() const { return TCombinationHelper_Set_Iter<T>(m_Elements, m_Iter, false); }
    TCombinationHelper_Set_Iter<T> end() const { return TCombinationHelper_Set_Iter<T>(m_Elements, m_Iter, true); }

    Vector<T> const& m_Elements;
    CombinationHelper_Iter const& m_Iter;
  };

  template <typename T>
  struct TCombinationHelper_Iter
  {
    TCombinationHelper_Iter(Vector<T> const& iElements, uint32_t iNum, bool iEnd)
      : m_Elements(iElements)
      , m_Iter(iElements.size(), iNum, iEnd)
    {

    }

    TCombinationHelper_Set<T> operator*()
    {
      return TCombinationHelper_Set<T>(m_Elements, m_Iter);
    }

    TCombinationHelper_Iter& operator ++()
    {
      ++m_Iter;
      return *this;
    }

    bool operator == (TCombinationHelper_Iter const& iOther) const
    {
      return m_Iter == iOther.m_Iter;
    }
    bool operator != (TCombinationHelper_Iter const& iOther) const
    {
      return !(*this == iOther);
    }

    Vector<T> const& m_Elements;
    CombinationHelper_Iter m_Iter;
  };

  template <typename T>
  struct TCombinationHelper
  {
    TCombinationHelper(Vector<T> const& iElements, uint32_t iNum)
      : m_Elements(iElements)
      , m_Num(iNum)
    {

    }

    TCombinationHelper_Iter<T> begin() const { return TCombinationHelper_Iter<T>(m_Elements, m_Num, false); }
    TCombinationHelper_Iter<T> end() const { return TCombinationHelper_Iter<T>(m_Elements, m_Num, true); }

    Vector<T> const& m_Elements;
    uint32_t m_Num;
  };
}
