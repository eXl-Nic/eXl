/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace eXl
{
  template <class Real>
  class Segment;
}

#include "vector2.hpp"

namespace eXl
{
  template <class Real>
  class Segment
  {
  public:

    Vector2<Real> m_Ext1;
    Vector2<Real> m_Ext2;

    bool operator == (Segment const& iOther) const
    {
      return (m_Ext1 == iOther.m_Ext1 && m_Ext2 == iOther.m_Ext2) 
         || (m_Ext1 == iOther.m_Ext2 && m_Ext2 == iOther.m_Ext1);
    }

    bool operator != (Segment const& iOther) const
    {
      return ! (*this == iOther);
    }

    enum IntersectResult
    {
      PointFound = 1 << 0,
      PointOnSegment1 = 1 << 1,
      PointOnSegment2 = 1 << 2,
      PointOnSegments = (1 << 3) - 1,
      AlignedSegments = 1 << 4,
      ConfoundSegments = 1 << 5
    };

    static Real Cross(Vector2<Real> const& iSegPt1, Vector2<Real> const& iSegPt2)
    {
      return iSegPt1.X() * iSegPt2.Y() - iSegPt1.Y() * iSegPt2.X();
    }

    static bool IsOnSegment(Vector2<Real> const& iSegPt1, Vector2<Real> const& iSegPt2, Vector2<Real> const& iPoint, Real iEpsilon = Math<Real>::EPSILON)
    {
      Vector2<Real> dir = iSegPt2 - iSegPt1;
      //Real length = dir.Normalize();
      Real cross = Cross(iPoint - iSegPt1, dir);
      if(Math<Real>::Abs(cross) > iEpsilon)
        return false;
      Real proj = (iPoint - iSegPt1).Dot(dir);
      if(proj < -iEpsilon || proj > dir.Dot(dir) + iEpsilon)
        return false;

      return true;
      //Real proj = (iPoint - iSegPt1).Dot(dir);
     // return ( ((iSegPt1 + dir * proj) - iPoint).Length() <= iEpsilon && proj >= 0.0 && proj <= length);
    }


    static bool IsOnSegmentStrict(Vector2<Real> const& iSegPt1, Vector2<Real> const& iSegPt2, Vector2<Real> const& iPoint, Real iEpsilon = Math<Real>::EPSILON)
    {
      Vector2<Real> dir = iSegPt2 - iSegPt1;
      //Real length = dir.Normalize();
      Real cross = Cross(iPoint - iSegPt1, dir);
      if(Math<Real>::Abs(cross) > iEpsilon)
        return false;
      Real proj = (iPoint - iSegPt1).Dot(dir);
      if(proj < iEpsilon || proj > dir.Dot(dir) - iEpsilon)
        return false;
      //return ( ((iSegPt1 + dir * proj) - iPoint).Length() <= iEpsilon && proj > iEpsilon && proj < (length - iEpsilon));
      return true;

    }

    /**
      >0 for iPoint left of the line through iSegPt1 to iSegPt2
      =0 for iPoint on the line
      <0 for iPoint right of the line
    */
    static Real IsLeft(Vector2<Real> const& iSegPt1, Vector2<Real> const& iSegPt2, Vector2<Real> const& iPoint)
    {
      return Cross(iSegPt2 - iSegPt1, iPoint - iSegPt1);
      //return ( (iSegPt2.X() - iSegPt1.X()) * (iPoint.Y() - iSegPt1.Y())
      //       - (iSegPt2.Y() - iSegPt1.Y()) * (iPoint.X() - iSegPt1.X()) );
    }

    struct SortByAngle
    {
      inline SortByAngle(Vector2<Real> const& iOrigPoint) :m_OrigPoint(iOrigPoint){}

      inline bool operator()(Vector2<Real> const& iPt1, Vector2<Real> const& iPt2) const
      {
        //test iPt1 < iPt2 <==> iPt2 left of m_OrigPoint - iPt1
        return (Segment<Real>::IsLeft(m_OrigPoint, iPt1, iPt2) > 0);
      }

      Vector2<Real> const& m_OrigPoint;
    };

    static Real NearestPointSeg(Vector2<Real> const& iPoint1, Vector2<Real> const& iPoint2, Vector2<Real> const& iPos, Vector2<Real>& oVect)
    {
      Vector2<Real> segDir = iPoint2 - iPoint1;
      Real segLength = segDir.Normalize();

      Vector2<Real> pt1Dir = iPoint1 - iPos;

      Real pt1DirProj = pt1Dir.Dot(segDir);
      if (pt1DirProj > Mathd::ZERO_TOLERANCE)
      {
        oVect = pt1Dir;
      }
      else if (-pt1DirProj > (segLength - Mathd::ZERO_TOLERANCE))
      {
        oVect = iPoint2 - iPos;
      }
      else
      {
        oVect = pt1Dir - (pt1DirProj * segDir);
      }

      return oVect.Normalize();
    }

    static unsigned int Intersect(Vector2<Real> const& iSeg1Pt1, Vector2<Real> const& iSeg1Pt2, Vector2<Real> const& iSeg2Pt1, Vector2<Real> const& iSeg2Pt2)
    {
      Vector2<Real> dummy;
      return Intersect(iSeg1Pt1, iSeg1Pt2, iSeg2Pt1, iSeg2Pt2, dummy);
    }

    static unsigned int Intersect(Vector2<Real> const& iSeg1Pt1, Vector2<Real> const& iSeg1Pt2, Vector2<Real> const& iSeg2Pt1, Vector2<Real> const& iSeg2Pt2, Vector2<Real>& oPoint, Real iEpsilon = Math<Real>::EPSILON)
    {
      if((iSeg1Pt1 == iSeg2Pt1 && iSeg1Pt2 == iSeg2Pt2)
      || (iSeg1Pt1 == iSeg2Pt2 && iSeg1Pt2 == iSeg2Pt1))
      {
        oPoint = iSeg1Pt1;
        return AlignedSegments | ConfoundSegments | PointOnSegments;
      }

      unsigned int oRes = 0;
      Vector2<Real> dir1 = iSeg1Pt2 - iSeg1Pt1;
      Vector2<Real> dir2 = iSeg2Pt2 - iSeg2Pt1;
      //Real length1 = dir1.Normalize();
      //Real length2 = dir2.Normalize();
      Real cross = dir1.X() * dir2.Y() - dir1.Y() * dir2.X();
      if (Math<Real>::FAbs(cross) > iEpsilon)
      {
        oRes |= PointFound;
        //Real eqn1[3];
        //if (Math<Real>::FAbs(dir1.X()) > iEpsilon)
        //{
        //  eqn1[0] = -1.0 * dir1.Y() / dir1.X();
        //  eqn1[1] = 1.0;
        //  eqn1[2] = -iSeg1Pt1.X() * eqn1[0] - iSeg1Pt1.Y();
        //}
        //else
        //{
        //  eqn1[0] = 1.0;
        //  eqn1[1] = 0.0;
        //  eqn1[2] = -iSeg1Pt1.X();
        //}
        //
        //Real eqn2[3];
        //if (Math<Real>::FAbs(dir2.X()) > iEpsilon)
        //{
        //  eqn2[0] = -1.0 * dir2.Y() / dir2.X();
        //  eqn2[1] = 1.0;
        //  eqn2[2] = -iSeg2Pt1.X() * eqn2[0] - iSeg2Pt1.Y();
        //}
        //else
        //{
        //  eqn2[0] = 1.0;
        //  eqn2[1] = 0.0;
        //  eqn2[2] = -iSeg2Pt1.X();
        //}
        //oPoint = Vector2<Real>((eqn1[1] * eqn2[2] - eqn1[2] * eqn2[1]) / (eqn1[0] * eqn2[1] - eqn1[1] * eqn2[0]),
        //  (eqn1[2] * eqn2[0] - eqn1[0] * eqn2[2]) / (eqn1[0] * eqn2[1] - eqn1[1] * eqn2[0]));


        Real proj1 = ((iSeg2Pt1.X() - iSeg1Pt1.X()) * (iSeg2Pt2.Y() - iSeg2Pt1.Y()) 
          - (iSeg2Pt1.Y() - iSeg1Pt1.Y()) * (iSeg2Pt2.X() - iSeg2Pt1.X())) / cross;

        if (proj1 >= 0.0 && proj1 <= 1.0)
        {
          oRes |= PointOnSegment1;
        }

        Real proj2 = ((iSeg2Pt1.X() - iSeg1Pt1.X()) * (iSeg1Pt2.Y() - iSeg1Pt1.Y()) 
          - (iSeg2Pt1.Y() - iSeg1Pt1.Y()) * (iSeg1Pt2.X() - iSeg1Pt1.X())) / cross;

        if (proj2 >= 0.0 && proj2 <= 1.0)
        {
          oRes |= PointOnSegment2;
        }

        oPoint = iSeg1Pt1 + proj1 * dir1;
      }
      else
      {
        oRes |= AlignedSegments;

        if((iSeg1Pt1 == iSeg2Pt1 && iSeg1Pt2 != iSeg2Pt2)
        || (iSeg1Pt1 == iSeg2Pt2 && iSeg1Pt2 != iSeg2Pt1))
        {
          oRes |= PointOnSegments;
          oPoint = iSeg1Pt1;
        }
        if((iSeg1Pt2 == iSeg2Pt1 && iSeg1Pt1 != iSeg2Pt2)
             || (iSeg1Pt2 == iSeg2Pt2 && iSeg1Pt1 != iSeg2Pt1))
        {
          if(oRes & PointOnSegments)
            oRes |= ConfoundSegments;
          else
            oRes |= PointOnSegments;
          oPoint = iSeg1Pt2;
        }
        if((iSeg2Pt1 != iSeg1Pt1 && iSeg2Pt1 != iSeg1Pt2 && IsOnSegmentStrict(iSeg1Pt1, iSeg1Pt2, iSeg2Pt1, iEpsilon)) 
        || (iSeg2Pt2 != iSeg1Pt1 && iSeg2Pt2 != iSeg1Pt2 && IsOnSegmentStrict(iSeg1Pt1, iSeg1Pt2, iSeg2Pt2, iEpsilon)))
        {
          oRes |= ConfoundSegments;
        }
      }

      return oRes;
    }

    static unsigned int NearestPointOnSeg1(Vector2<Real> const& iSeg1Pt1, Vector2<Real> const& iSeg1Pt2, Vector2<Real> const& iSeg2Pt1, Vector2<Real> const& iSeg2Pt2, Vector2<Real>& oPoint)
    {
      unsigned int oRes = 0;
      Vector2<Real> dir1 = iSeg1Pt2 - iSeg1Pt1;
      Vector2<Real> dir2 = iSeg2Pt2 - iSeg2Pt1;
      Real length1 = dir1.Normalize();
      Real length2 = dir2.Normalize();
      Real cross = dir1.X() * dir2.Y() - dir1.Y() * dir2.X();
      if (Math<Real>::FAbs(cross) > Math<Real>::EPSILON)
      {
        oRes |= PointFound;
        Real eqn1[3];
        if (Math<Real>::FAbs(dir1.X()) > Math<Real>::EPSILON)
        {
          eqn1[0] = -1.0 * dir1.Y() / dir1.X();
          eqn1[1] = 1.0;
          eqn1[2] = -iSeg1Pt1.X() * eqn1[0] - iSeg1Pt1.Y();
        }
        else
        {
          eqn1[0] = 1.0;
          eqn1[1] = 0.0;
          eqn1[2] = -iSeg1Pt1.X();
        }

        Real eqn2[3];
        if (Math<Real>::FAbs(dir2.X()) > Math<Real>::EPSILON)
        {
          eqn2[0] = -1.0 * dir2.Y() / dir2.X();
          eqn2[1] = 1.0;
          eqn2[2] = -iSeg2Pt1.X() * eqn2[0] - iSeg2Pt1.Y();
        }
        else
        {
          eqn2[0] = 1.0;
          eqn2[1] = 0.0;
          eqn2[2] = -iSeg2Pt1.X();
        }
        oPoint = Vector2<Real>((eqn1[1] * eqn2[2] - eqn1[2] * eqn2[1]) / (eqn1[0] * eqn2[1] - eqn1[1] * eqn2[0]),
          (eqn1[2] * eqn2[0] - eqn1[0] * eqn2[2]) / (eqn1[0] * eqn2[1] - eqn1[1] * eqn2[0]));
        Real proj1 = (oPoint - iSeg1Pt1).Dot(dir1);
        if (proj1 < 0.0)
          oPoint = iSeg1Pt1;
        else if (proj1 > length1)
          oPoint = iSeg1Pt2;
        else
          oRes |= PointOnSegment1;
        Real proj2 = (oPoint - iSeg2Pt1).Dot(dir2);
        if (proj2 >= 0.0 && proj2 <= length2)
          oRes |= PointOnSegment2;
      }
      else
      {
        oRes = AlignedSegments;
      }

      return oRes;
    }

    static Real SegmentsDist(Vector2<Real> const& iSeg1Pt1, Vector2<Real> const& iSeg1Pt2, Vector2<Real> const& iSeg2Pt1, Vector2<Real> const& iSeg2Pt2, Vector2<Real>& oDir)
    {
      unsigned int res = Intersect(iSeg1Pt1, iSeg1Pt2, iSeg2Pt1, iSeg2Pt2, oDir);
      if(res == PointOnSegments
      || res & ConfoundSegments)
      {
        oDir = Vector2<Real>::ZERO;
        return 0;
      }
      else
      {
        Vector2<Real> dir;
        Real dist = NearestPointSeg(iSeg1Pt1, iSeg1Pt2, iSeg2Pt1, oDir);
        Real newDist;
        if ((newDist = NearestPointSeg(iSeg1Pt1, iSeg1Pt2, iSeg2Pt2, dir)) < dist){ oDir = dir; dist = newDist; }
        if ((newDist = NearestPointSeg(iSeg2Pt1, iSeg2Pt2, iSeg1Pt1, dir)) < dist){ oDir = dir; dist = newDist; }
        if ((newDist = NearestPointSeg(iSeg2Pt1, iSeg2Pt2, iSeg1Pt2, dir)) < dist){ oDir = dir; dist = newDist; }
        return dist;
      }
    }

    Real Cross() const
    {
      return Cross(m_Ext1, m_Ext2);
    }

    bool IsOnSegment(Vector2<Real> const& iPoint, Real iEpsilon = Math<Real>::EPSILON) const
    {
      return IsOnSegment(m_Ext1, m_Ext2, iPoint, iEpsilon);
    }

    Real IsLeft(Vector2<Real> const& iPoint) const
    {
      return IsLeft(m_Ext1, m_Ext2, iPoint);
    }

    struct SortSegmentByAngle
    {
      inline SortSegmentByAngle(Vector2<Real> const& iOrigPoint) :m_OrigPoint(iOrigPoint){}

      inline bool operator()(Segment<Real> const& iSeg) const
      {
        //test iPt1 < iPt2 <==> iPt2 left of m_OrigPoint - iPt1
        return (Segment<Real>::IsLeft(m_OrigPoint, iSeg.m_Ext1, iSeg.m_Ext2) > 0);
      }

      Vector2<Real> const& m_OrigPoint;
    };

    Real NearestPointSeg(Vector2<Real> const& iPos, Vector2<Real>& oVect) const
    {
      return NearestPointSeg(m_Ext1, m_Ext2, iPos, oVect);
    }

    unsigned int Intersect(Segment<Real> const& iSeg2) const
    {
      Vector2<Real> dummy;
      return Intersect(iSeg2, dummy);
    }

    unsigned int Intersect(Segment<Real> const& iSeg2, Vector2<Real>& oPoint) const
    {
      return Intersect(m_Ext1, m_Ext2, iSeg2.m_Ext1, iSeg2.m_Ext2, oPoint);
    }

    inline unsigned int NearestPointOnSeg(Segment<Real> const& iSeg2, Vector2<Real>& oPoint) const
    {
      return NearestPointOnSeg1(m_Ext1, m_Ext2, iSeg2.m_Ext1, iSeg2.m_Ext2, oPoint);
    }
    inline Real SegmentsDist(Segment<Real> const& iSeg2, Vector2<Real>& oDir) const
    {
      return SegmentsDist(m_Ext1, m_Ext2, iSeg2.m_Ext1, iSeg2.m_Ext2, oDir);
    }
  };

  typedef Segment<float> Segmentf;
  typedef Segment<double> Segmentd;
  typedef Segment<int> Segmenti;

}
