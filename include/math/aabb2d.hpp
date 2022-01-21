/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "math.hpp"
#include "vector2.hpp"

namespace eXl
{
  template <class Real>
  class AABB2D
  {
  public:
    AABB2D(){}
    AABB2D(AABB2D const& iOther)
    {
      m_Data[0] = iOther.m_Data[0];
      m_Data[1] = iOther.m_Data[1];
    }
    AABB2D& operator=(AABB2D const& iOther)
    {
      m_Data[0] = iOther.m_Data[0];
      m_Data[1] = iOther.m_Data[1];
      return *this;
    }
    AABB2D(Real iMinX, Real iMinY, Real iMaxX, Real iMaxY)
    {
      m_Data[0] = Vector2<Real>(iMinX,iMinY);
      m_Data[1] = Vector2<Real>(iMaxX,iMaxY);
    }

    static AABB2D FromCenterAndSize(Vector2<Real> iCenter,Vector2<Real> iSize)
    {
      AABB2D ret;
      ret.m_Data[0] = iCenter - iSize / 2.0;
      ret.m_Data[1] = iCenter + iSize / 2.0;
      return ret;
    }

    AABB2D(Vector2<Real> iMin,Vector2<Real> iSize)
    {
      m_Data[0] = iMin;
      m_Data[1] = iMin + iSize;
    }

    template <class OtherReal>
    explicit AABB2D(AABB2D<OtherReal> const& iOther)
    {
      m_Data[0] = Vector2<Real>(iOther.m_Data[0]);
      m_Data[1] = Vector2<Real>(iOther.m_Data[1]);
    }

    bool Empty() const
    {
      return m_Data[0].X() == m_Data[1].X() || m_Data[0].Y() == m_Data[1].Y();
    }

    bool CircleTest(Vector2f const& iPos, Real iRadius)
    {
      if(Contains(iPos))
      {
        return true;
      }
      else
      {
        //Test segment X == X0
        Real dist = m_Data[0].X() - iPos.X();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real y = Math<Real>::Sqrt(dist) + iPos.Y();
          if(Math<Real>::ZERO_TOLERANCE < m_Data[1].Y() - y && y - m_Data[0].Y() > Math<Real>::ZERO_TOLERANCE)
            return true;
        }
        //Test segment X == X1
        dist = m_Data[1].X() - iPos.X();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real y = Math<Real>::Sqrt(dist) + iPos.Y();
          if(Math<Real>::ZERO_TOLERANCE < m_Data[1].Y() - y && y - m_Data[0].Y() > Math<Real>::ZERO_TOLERANCE)
            return true;
        }

        //TestSegment Y == Y0
        dist = m_Data[0].Y() - iPos.Y();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real x = Math<Real>::Sqrt(dist) + iPos.X();
          if(Math<Real>::ZERO_TOLERANCE < m_Data[1].X() - x && x - m_Data[0].X() > Math<Real>::ZERO_TOLERANCE)
            return true;
        }
        //Test segment Y == Y1
        dist = m_Data[1].Y() - iPos.Y();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real x = Math<Real>::Sqrt(dist) + iPos.X();
          if(Math<Real>::ZERO_TOLERANCE < m_Data[1].X() - x && x - m_Data[0].X() > Math<Real>::ZERO_TOLERANCE)
            return true;
        }
      }
      return false;
    }

    void Rotate(Real iAngle)
    {
      Vector2<Real> center = GetCenter();
      Vector2<Real> size = GetSize();
      Vector2<Real> points[4];
      points[0] = size * -0.5;
      points[1] = Vector2<Real>(size.X() * -0.5, size.Y() *  0.5) ;
      points[2] = Vector2<Real>(size.X() *  0.5, size.Y() * -0.5) ;
      points[3] = size * 0.5;
      points[0] = Vector2<Real>(points[0].X() * Math<Real>::Cos(iAngle) + points[0].Y() * Math<Real>::Sin(iAngle)
                         , points[0].Y() * Math<Real>::Cos(iAngle) - points[0].X() * Math<Real>::Sin(iAngle));
      points[1] = Vector2<Real>(points[1].X() * Math<Real>::Cos(iAngle) + points[1].Y() * Math<Real>::Sin(iAngle)
                         , points[1].Y() * Math<Real>::Cos(iAngle) - points[1].X() * Math<Real>::Sin(iAngle));
      points[2] = Vector2<Real>(points[2].X() * Math<Real>::Cos(iAngle) + points[2].Y() * Math<Real>::Sin(iAngle)
                         , points[2].Y() * Math<Real>::Cos(iAngle) - points[2].X() * Math<Real>::Sin(iAngle));
      points[3] = Vector2<Real>(points[3].X() * Math<Real>::Cos(iAngle) + points[3].Y() * Math<Real>::Sin(iAngle)
                         , points[3].Y() * Math<Real>::Cos(iAngle) - points[3].X() * Math<Real>::Sin(iAngle));

      m_Data[0] = m_Data[1] = points[0];
      for (unsigned int i = 1; i < 4; ++i)
      {
        m_Data[0].X() = Math<Real>::Min(m_Data[0].X(), points[i].X());
        m_Data[0].Y() = Math<Real>::Min(m_Data[0].Y(), points[i].Y());
        m_Data[1].X() = Math<Real>::Max(m_Data[1].X(), points[i].X());
        m_Data[1].Y() = Math<Real>::Max(m_Data[1].Y(), points[i].Y());
      }
      m_Data[0] += center;
      m_Data[1] += center;
    }

    Vector2<Real> GetCenter()const
    {
      return (m_Data[0] + m_Data[1])/2;
    }

    Vector2<Real> GetSize()const
    {
      return m_Data[1] - m_Data[0];
    }

    void Inflate(Real iCoeff)
    {
      Vector2<Real> center = (m_Data[1] + m_Data[0]) / 2;
      Vector2<Real> size = m_Data[1] - m_Data[0];
      size = size * iCoeff;
      m_Data[0] = center - size / 2.0;
      m_Data[1] = center + size / 2.0;
    }

    Real MinX()const{return m_Data[0].X();}
    Real MinY()const{return m_Data[0].Y();}
    Real MaxX()const{return m_Data[1].X();}
    Real MaxY()const{return m_Data[1].Y();}
    
    Real& MinX(){return m_Data[0].X();}
    Real& MinY(){return m_Data[0].Y();}
    Real& MaxX(){return m_Data[1].X();}
    Real& MaxY(){return m_Data[1].Y();}

    bool operator ==(AABB2D const& iOther) const
    {
      return m_Data[0]==iOther.m_Data[0] && m_Data[1] == iOther.m_Data[1];
    }

    bool operator !=(AABB2D const& iOther) const
    {
      return !((*this) == iOther);
    }

	  bool Empty()
	  {
		  return m_Data[0].X() == m_Data[1].X() || m_Data[0].Y() == m_Data[1].Y();
	  }

    void SetCommonBox(AABB2D<Real> const& iBox1,AABB2D<Real> const& iBox2)
    {
      m_Data[0].X() = Math<Real>::Max(iBox1.m_Data[0].X(),iBox2.m_Data[0].X());
      m_Data[0].Y() = Math<Real>::Max(iBox1.m_Data[0].Y(),iBox2.m_Data[0].Y());
      m_Data[1].X() = Math<Real>::Max(Math<Real>::Min(iBox1.m_Data[1].X(),iBox2.m_Data[1].X()),m_Data[0].X());
      m_Data[1].Y() = Math<Real>::Max(Math<Real>::Min(iBox1.m_Data[1].Y(),iBox2.m_Data[1].Y()),m_Data[0].Y());
    }

    void Absorb(Vector2<Real> const& iPoint)
    {
      m_Data[0].X() = Math<Real>::Min(m_Data[0].X(), iPoint.X());
      m_Data[0].Y() = Math<Real>::Min(m_Data[0].Y(), iPoint.Y());
      m_Data[1].X() = Math<Real>::Max(m_Data[1].X(), iPoint.X());
      m_Data[1].Y() = Math<Real>::Max(m_Data[1].Y(), iPoint.Y());
    }

    void Absorb(AABB2D<Real> const& iBox)
    {
      m_Data[0].X() = Math<Real>::Min(m_Data[0].X(),iBox.m_Data[0].X());
      m_Data[0].Y() = Math<Real>::Min(m_Data[0].Y(),iBox.m_Data[0].Y());
      m_Data[1].X() = Math<Real>::Max(m_Data[1].X(),iBox.m_Data[1].X());
      m_Data[1].Y() = Math<Real>::Max(m_Data[1].Y(),iBox.m_Data[1].Y());
    }

    bool IsInside(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZERO_TOLERANCE) const
    {
      return (m_Data[0].X() - iOther.m_Data[0].X() >= iEpsilon && m_Data[0].Y() - iOther.m_Data[0].Y() >= iEpsilon
           && iOther.m_Data[1].X() - m_Data[1].X() >= iEpsilon && iOther.m_Data[1].Y() - m_Data[1].Y() >= iEpsilon );
    }

    bool Intersect(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZERO_TOLERANCE) const
    {
      return iOther.m_Data[1].X() - m_Data[0].X() >= iEpsilon && m_Data[1].X() - iOther.m_Data[0].X() >= iEpsilon
          && iOther.m_Data[1].Y() - m_Data[0].Y() >= iEpsilon && m_Data[1].Y() - iOther.m_Data[0].Y() >= iEpsilon;
    }

    //0 No, 1 Left, 2 Right, 3 Down, 4 Up
    int Touch(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZERO_TOLERANCE) const
    {
      if(iOther.m_Data[1].Y() - m_Data[0].Y() >= iEpsilon && m_Data[1].Y() - iOther.m_Data[0].Y() >= iEpsilon)
      {
        Real val = m_Data[0].X() - iOther.m_Data[1].X();
        if(val <= iEpsilon && val >= -iEpsilon)
          return 1;
        
        val = m_Data[1].X() - iOther.m_Data[0].X();
        if(val <= iEpsilon && val >= -iEpsilon)
          return 2;
      }
      if(iOther.m_Data[1].X() - m_Data[0].X() >= iEpsilon && m_Data[1].X() - iOther.m_Data[0].X() >= iEpsilon)
      {
        Real val = m_Data[0].Y() - iOther.m_Data[1].Y();
      
        if(val <= iEpsilon && val >= -iEpsilon)
          return 3;

        val = m_Data[1].Y() - iOther.m_Data[0].Y();
        if(val <= iEpsilon && val >= -iEpsilon)
          return 4;
      }
      return 0;
    }

    Optional<Vector2<Real>> SegmentTest(Vector2<Real> const& iOrigin, Vector2<Real> const& iDir, Real iEpsilon = Math<Real>::ZERO_TOLERANCE)
    {
      Real const scaleX = 1.0 / iDir.X();
      Real const scaleY = 1.0 / iDir.Y();
      Real const signX = Math<Real>::Sign(scaleX);
      Real const signY = Math<Real>::Sign(scaleY);

      Real const nearTimeX = ((signX > 0 ? m_Data[0].X() : m_Data[1].X()) - iOrigin.X()) * scaleX;
      Real const nearTimeY = ((signY > 0 ? m_Data[0].Y() : m_Data[1].Y()) - iOrigin.Y()) * scaleY;
      Real const farTimeX = ((signX > 0 ? m_Data[1].X() : m_Data[0].X()) - iOrigin.X()) * scaleX;
      Real const farTimeY = ((signY > 0 ? m_Data[1].Y() : m_Data[0].Y()) - iOrigin.Y()) * scaleY;

      if (nearTimeX > farTimeY || nearTimeY > farTimeX) 
      {
        return {};
      }

      Real const nearTime = nearTimeX > nearTimeY ? nearTimeX : nearTimeY;
      Real const farTime = farTimeX < farTimeY ? farTimeX : farTimeY;

      if (nearTime > 1 - iEpsilon || farTime < -iEpsilon) 
      {
        return {};
      }

      return iOrigin + iDir * nearTime;
    }

    bool Contains(Vector2<Real>const& iPoint, Real iEpsilon = Math<Real>::ZERO_TOLERANCE) const
    {
      return iPoint.X() - m_Data[0].X() >= iEpsilon && m_Data[1].X() - iPoint.X() >= iEpsilon
          && iPoint.Y() - m_Data[0].Y() >= iEpsilon && m_Data[1].Y() - iPoint.Y() >= iEpsilon;
    }

    // Neg value -> distance to min, Pos value -> distance to max, 0, inside
    Vector2f Classify(Vector2<Real>const& iPoint, Real iEpsilon = Math<Real>::ZERO_TOLERANCE) const
    {
      Vector2f res(iPoint.X() - m_Data[0].X(), iPoint.Y() - m_Data[0].Y());
      for(uint32_t axis = 0; axis < 2; ++axis)
      {
        if(!(res.m_Data[axis] < -iEpsilon))
        {
          res.m_Data[axis] = m_Data[1].m_Data[axis] - iPoint.m_Data[axis];
          if(res.m_Data[axis] < -iEpsilon)
          {
            res.m_Data[axis] *= -1;
          }
          else
          {
            res.m_Data[axis] = 0;
          }
        }
      }

      return res;
    }

    Vector2<Real> GetDistX(AABB2D const& iOther) const
    {
      return Vector2<Real>(iOther.m_Data[0].X() - m_Data[0].X(),m_Data[1].X() - iOther.m_Data[1].X());
    }

    Vector2<Real> GetDistY(AABB2D const& iOther) const
    {
      return Vector2<Real>(iOther.m_Data[0].Y() - m_Data[0].Y(),m_Data[1].Y() - iOther.m_Data[1].Y());
    }

    union
    {
      Vector2<Real> m_Data[2];
      struct
      {
        Vector2<Real> m_Min;
        Vector2<Real> m_Max;
      };
    };
  };

  typedef AABB2D<int> AABB2Di; 
  typedef AABB2D<float> AABB2Df;
  typedef AABB2D<double> AABB2Dd;
}