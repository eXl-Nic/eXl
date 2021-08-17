/*
Copyright 2009-2019 Nicolas Colombe

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
    //static Err Stream(Vector2<Real> const& iPoint, Serializer iStreamer);

    //template <typename Real>
    //static Err Unstream(Vector2<Real>& oPoint, Unstreamer& iUnstreamer);

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
      int div = iAngle / (Mathf::PI * 2);
      if(iAngle < 0)
      {
        --div;
      }
      return iAngle - div * Mathf::PI * 2;
    }

    // AngleDist between -PI and PI
    inline static float AngleDist(float iAngle1, float iAngle2)
    {
      float dist = ClampAngle(iAngle1) - ClampAngle(iAngle2);
      if(dist > Mathf::PI)
      {
        return 2.0 * Mathf::PI - dist;
      }
      else if(dist < -Mathf::PI)
      {
        return dist + 2.0 * Mathf::PI;
      }
      return dist;
    }


    template <typename Real>
    static inline Vector2<Real> Perp(Vector2<Real> const& iVec)
    {
      return Vector2<Real>(-iVec.Y(), iVec.X());
    }

    static inline uint64_t SquaredLength(Vector2i const& iVect)
    {
      return (int64_t)iVect.X() * (int64_t)iVect.X() + (int64_t)iVect.Y() * (int64_t)iVect.Y();
    }

    template <typename Real>
    static inline Vector2i ToIVec(Vector2<Real> const& iVec)
    {
      return Vector2i(Math<Real>::Round(iVec.X()), Math<Real>::Round(iVec.Y()));
    }

    template <typename Real>
    static inline Vector2f ToFVec(Vector2<Real> const& iVec)
    {
      return Vector2f(iVec.X(), iVec.Y());
    }

    template <typename Real>
    static inline Vector2d ToDVec(Vector2<Real> const& iVec)
    {
      return Vector2d(iVec.X(), iVec.Y());
    }

    template <typename Real>
    static inline Real Sign(Real iValue)
    {
      return iValue > 0 ? 1 : -1;
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(Vector2<Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, unsigned int &oSegNum, Vector2<Real1>& oPoint, Vector2<Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      Vector2<Real2> dummy;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, dummy, oSegNum, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(Vector2<Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, Vector2<Real2> (&oSeg)[2], Vector2<Real1>& oPoint, Vector2<Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      unsigned int dummy;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, oSeg, dummy, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(Vector2<Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, Vector2<Real2> (&oSeg)[2], unsigned int& oSegNum, Vector2<Real1>& oPoint, Vector2<Real1>& oDir, bool iCheckExt = true)
    {
      int dummyHole;
      return NearestPointOnPoly(iPoint,iPoly, iMinSegLen, oSeg, oSegNum, dummyHole, oPoint, oDir, iCheckExt, false);
    }

    template <typename Real1, typename Real2>
    static inline bool NearestPointOnPoly(Vector2<Real1> const& iPoint, Polygon<Real2> const& iPoly, Real1 iMinSegLen, Vector2<Real2> (&oSeg)[2], unsigned int& oSegNum, int& oHole, Vector2<Real1>& oPoint, Vector2<Real1>& oDir, bool iCheckExt = true, bool iCheckHoles = false)
    {
      Real1 minDist;
      oHole = -1;
      bool res = _NearestPointOnPoly(iPoint, iPoly.Border(), iMinSegLen, oSeg, oSegNum, oPoint, oDir, minDist, iCheckExt);
      if(iCheckHoles)
      {
        for(unsigned int i = 0 ; i< iPoly.Holes().size(); ++i)
        {
          unsigned int segNum;
          Vector2<Real1> point;
          Vector2<Real2> seg[2];
          Vector2<Real1>    dir;
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
    static inline bool _NearestPointOnPoly(Vector2<Real1> const& iPoint, Vector<Vector2<Real2> > const& polyBorder, Real1 iMinSegLen, Vector2<Real2> (&oSeg)[2], unsigned int& oSegNum, Vector2<Real1>& oPoint, Vector2<Real1>& oDir, Real1& oDist, bool iCheckExt)
    {
      Vector2<Real1> polyCenter;
      for(auto point : polyBorder)
      {
        polyCenter += Vector2<Real1>(point.X(), point.Y());
      }

      if(polyBorder.front() == polyBorder.back())
      {
        polyCenter -= Vector2<Real1>(polyBorder.front().X(), polyBorder.front().Y());
        polyCenter = polyCenter / (polyBorder.size() - 1);
      }
      else
      {
        polyCenter = polyCenter / polyBorder.size();
      }

      //Vector2<Real1> iDir = polyCenter - iPoint;
      //Real1 minDist = iDir.Normalize();

      Vector2<Real1> prevPoint(polyBorder.back().X(), polyBorder.back().Y());

      Vector2<Real1> iDir = polyCenter - iPoint;
      iDir.Normalize();
      Real1 minDist = (iPoint - prevPoint).Length() + 1;

      for (unsigned int i = 0; i < polyBorder.size(); ++i)
      {
        Vector2<Real1> curPoint(polyBorder[i].X(), polyBorder[i].Y());

        if (curPoint != prevPoint)
        {
          Vector2<Real1> dir = curPoint - prevPoint;
          Vector2<Real1> extDir(MathTools::Perp(dir));

          if (!iCheckExt || (iDir).Dot(extDir) < 0)
          {
            Real1 segLen = dir.Normalize();
            if (segLen > iMinSegLen)
            {
              Vector2<Real1> prevPointOff = prevPoint + dir * 0.5 * iMinSegLen;
              Vector2<Real1> curPointOff = curPoint - dir * 0.5 * iMinSegLen;
              segLen -= iMinSegLen;
              Real1 dirPoj = (iPoint - prevPointOff).Dot(dir);
              Vector2<Real1> candidate;

              if (dirPoj < -Math<Real1>::ZERO_TOLERANCE)
              {
                candidate = prevPointOff;
              }
              else if (dirPoj > segLen - Math<Real1>::ZERO_TOLERANCE)
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
                oSeg[0] = Vector2<Real2>(prevPoint.X(), prevPoint.Y());
                oSeg[1] = Vector2<Real2>(curPoint.X(), curPoint.Y());
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
    static inline unsigned int FindPolyIntersection(Vector2<Real1> const& iPoint1, Vector2<Real1> const& iPoint2, Polygon<Real2> const& iPoly, Vector2<Real1>(&oPoints)[2])
    {
      unsigned int curInter = 0;
      Vector<Vector2<Real2> > const& border = iPoly.Border();
      Vector2<Real1> prevPoint(border.back().X(), border.back().Y());
      for (unsigned int i = 0; i < border.size() && curInter < 2; ++i)
      {
        Vector2<Real1> curPoint(border[i].X(), border[i].Y());
            
        unsigned int res = Segment<Real1>::Intersect(prevPoint, curPoint, iPoint1, iPoint2, oPoints[curInter]);
        if (res & Segmentd::PointOnSegment1)
        {
          if ((curInter == 0 || (oPoints[0] - oPoints[curInter]).Length() > Math<Real1>::ZERO_TOLERANCE))
          {
            ++curInter;
          }
        }
        prevPoint = curPoint;
      }
      return curInter;
    }

    template <typename Real1, typename Real2>
    static bool FindCommonSegment(Polygon<Real2> const& iPoly1, Polygon<Real2> const& iPoly2, Vector2<Real1>& oSegPt1, Vector2<Real1>& oSegPt2, Real1 iMinLen = Math<Real1>::ZERO_TOLERANCE)
    {
      bool foundCommon = false;
      Vector2<Real1> prevPt1 = Vector2<Real1>(iPoly1.Border().back().X(),iPoly1.Border().back().Y());
      for(auto curPt1I : iPoly1.Border())
      {
        Vector2<Real1> curPt1 = Vector2<Real1>(curPt1I.X(),curPt1I.Y());
        if(prevPt1 != curPt1)
        {
          Vector2<Real1> prevPt2 = Vector2<Real1>(iPoly2.Border().back().X(), iPoly2.Border().back().Y());
          for(auto curPt2I : iPoly2.Border())
          {
            Vector2<Real1> curPt2 = Vector2<Real1>(curPt2I.X(),curPt2I.Y());
            if(prevPt2 != curPt2)
            {
                unsigned int res = Segment<Real1>::Intersect(prevPt1, curPt1, prevPt2, curPt2);
                if(res & Segment<Real1>::AlignedSegments
                && (res & Segment<Real1>::ConfoundSegments || res & Segmentd::PointOnSegments))
                {
                  Vector2<Real1> dir = curPt1 - prevPt1;
                  AABB2D<Real1> box1(0.0,0.0, dir.Normalize(), 0.0);
                  double val1 = (prevPt2 - prevPt1).Dot(dir);
                  double val2 = (curPt2 - prevPt1).Dot(dir);
                  AABB2D<Real1> box2 = val1 > val2 ? AABB2D<Real1>(val2, 0.0, val1, 0.0) : AABB2D<Real1>(val1, 0.0, val2, 0.0);
                  AABB2D<Real1> result;
                  result.SetCommonBox(box1, box2);
                  double commonLen = result.m_Data[1].X() - result.m_Data[0].X();
                  if(commonLen >= iMinLen)
                  {
                    foundCommon = true;
                    oSegPt1 = prevPt1 + dir * result.m_Data[0].X();
                    oSegPt2 = prevPt1 + dir * (result.m_Data[0].X() + commonLen);
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
    static inline Real GetAngleFromVec(Vector2<Real> const& iDir)
    {
      return Math<Real>::Abs(iDir.X()) < Math<Real>::EPSILON ? (iDir.Y() > 0.0 ? Math<Real>::PI * 0.5 : Math<Real>::PI * 1.5) 
        : (iDir.X() > 0.0 ? Math<Real>::ATan(iDir.Y() / iDir.X()) : Math<Real>::PI - Math<Real>::ATan(iDir.Y() / (-1.0 * iDir.X())));
    }

    template<typename Real>
    static inline Vector2<Real> GetPerp(Vector2<Real> const& iVec)
    {
      return Vector2<Real>(iVec.Y() * -1.0, iVec.X());
    }

    template<typename Real>
    static inline Vector2<Real> GetLocal(Vector2<Real> const& iBase, Vector2<Real> const& iVec)
    {
      return Vector2<Real>(iBase.X() * iVec.X() + iBase.Y() * iVec.Y(), iBase.X() * iVec.Y() - iBase.Y() * iVec.X());
    }

    template<typename Real>
    static inline Vector2i RoundVector(Vector2<Real> const& iVec)
    {
      return Vector2i(Math<Real>::Round(iVec.X()), Math<Real>::Round(iVec.Y()));
    }

    template <typename Real>
    static inline Real ModAngle(Real iAngle)
    {
      iAngle = fmod(iAngle, 2.0*Math<Real>::PI);
      iAngle -= Mathf::PI;
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

    static inline Vector2f Perturbate(Random& iRand, Vector2f const& iPos, float iRadius)
    {
      float const angle = ((iRand.Generate() % 10000) / 10000.0 - 1.0) * Mathf::PI;
      float const dist = ((iRand.Generate() % 10000) / 10000.0 ) * iRadius;

      return iPos + Vector2f(Mathd::Cos(angle), Mathd::Sin(angle)) * dist;
    }

    template <typename Real>
    static inline Vector2<Real> const& As2DVec(Vector3<Real> const& iVec)
    {
      return reinterpret_cast<Vector2<Real> const&>(iVec);
    }

    template <typename Real>
    static inline Vector2<Real>& As2DVec(Vector3<Real>& iVec)
    {
      return reinterpret_cast<Vector2<Real>&>(iVec);
    }

    template <typename Real>
    static inline Vector3<Real> To3DVec(Vector2<Real> const& iVec, Real iZComp = 0)
    {
      return Vector3<Real>(iVec.X(), iVec.Y(), iZComp);
    }

    template <typename Real>
    static inline Vector3<Real> const& GetPosition(Matrix4<Real> const& iMat)
    {
      return *reinterpret_cast<Vector3<Real> const*>(iMat.m_Data + 12);
    }

    template <typename Real>
    static inline Vector2<Real> const& GetPosition2D(Matrix4<Real> const& iMat)
    {
      return *reinterpret_cast<Vector2<Real> const*>(iMat.m_Data + 12);
    }

    template <typename Real>
    static inline Vector3<Real>& GetPosition(Matrix4<Real>& iMat)
    {
      return *reinterpret_cast<Vector3<Real>*>(iMat.m_Data + 12);
    }

    template <typename Real>
    static inline Vector2<Real>& GetPosition2D(Matrix4<Real>& iMat)
    {
      return *reinterpret_cast<Vector2<Real>*>(iMat.m_Data + 12);
    }

    // Prereq : points have to be in CCW order
    template <typename Real>
    static Vector2<Real> ConeGetMidSegment(Vector2<Real> const (&iRange)[2])
    {
      Vector2<Real> mid = iRange[0] + iRange[1];
      Real dist = mid.Normalize();
      if (dist < Math<Real>::ZERO_TOLERANCE)
      {
        mid = GetPerp(iRange[0]);
      }
      mid *= Math<Real>::Sign(Segment<Real>::Cross(iRange[0], mid));

      return mid;
    }

    template <typename Real>
    static void ConeUpdateRange(Vector2<Real> const (&iRange)[2], Vector2<Real>& oMid, Real& oLowLimit)
    {
      oMid = ConeGetMidSegment(iRange);
      oLowLimit = iRange[0].Dot(oMid) / iRange[0].Length();
    }

    template <typename Real>
    static bool IsInCone(Vector2<Real> const& iDir, Vector2<Real> const& iMidSeg, Real iLowLimit, Real iEpsilon = Math<Real>::EPSILON)
    {
      return iDir.Dot(iMidSeg) > (iLowLimit - iEpsilon);
    }

    template <typename Real>
    static Vector3<Real> Reflect(Vector3<Real> const& iIncomingDir, Vector3<Real> const& iNormal)
    {
      return iIncomingDir - iNormal * iIncomingDir.Dot(iNormal) * 2;
    }

    template <typename Real>
    static void ClampToBox(Vector2<Real>& ioPoint, AABB2D<Real> const& iBox)
    {
      for (uint32_t i = 0; i < 2; ++i)
      {
        ioPoint.m_Data[i] = Math<Real>::Clamp(ioPoint.m_Data[i], iBox.m_Data[0].m_Data[i], iBox.m_Data[1].m_Data[i]);
      }
    }

    //static void AlignVector(Vector2d& ioDir, Vector2i& oCanonicalVector, unsigned int iFactor)
    //{
    //  if (Mathd::Abs(ioDir.X()) > Mathd::ZERO_TOLERANCE)
    //  {
    //    if (Mathd::Abs(ioDir.Y()) > Mathd::ZERO_TOLERANCE)
    //    {
    //      double ratio = (ioDir.X() / ioDir.Y()));
    //      int sign = Sign(ratio);
    //      ratio = ratio * sign;
    //      if (ratio > 1)
    //      {
    //        ratio = 1 / ratio;
    //        unsigned int ratioI = Mathi::Round(ratio * iFactor);
    //        oCanonicalVector = Vector2i(Sign(ioDir.X()) * iFactor, Sign(ioDir.Y()) * ratioI);
    //      }
    //      else
    //      {
    //        unsigned int ratioI = Mathi::Round(ratio * iFactor);
    //        oCanonicalVector = Vector2i(Sign(ioDir.X()) * ratioI, Sign(ioDir.Y()) * iFactor);
    //      }
    //      ioDir = MathTools::ToDVec(oCanonicalVector) * ioDir.Length() / Mathd::Sqrt(ratioI*ratioI + iFactor*iFactor);
    //      return;
    //    }
    //  }
    //  ioDir = Vector2d::ZERO;
    //  oCanonicalVector = Vector2i::ZERO;
    //}
    //
    //static void AlignSegment(Vector2d& ioPoint1, Vector2d ioPoint2, unsigned int iFactor)
    //{
    //  
    //}

  };

  //template <>
  //EXL_MATH_API Err MathTools::Stream<float>(Vector2f const& iPoint, Serializer iStreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Unstream<float>(Vector2f& oPoint, Unstreamer& iUnstreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Stream<int>(Vector2i const& iPoint, Serializer iStreamer);

  //template <>
  //EXL_MATH_API Err MathTools::Unstream<int>(Vector2i& oPoint, Unstreamer& iUnstreamer);

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
