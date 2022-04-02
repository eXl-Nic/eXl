/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "math.hpp"

namespace eXl
{
  template <class Real>
  class AABB2D
  {
  public:
    AABB2D()
    {
      m_Data[0] = m_Data[1] = Zero<glm::vec<2,Real>>();
    }
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
      m_Data[0] = glm::vec<2,Real>(iMinX,iMinY);
      m_Data[1] = glm::vec<2,Real>(iMaxX,iMaxY);
    }

    static AABB2D FromCenterAndSize(glm::vec<2,Real> iCenter,glm::vec<2,Real> iSize)
    {
      AABB2D ret;
      ret.m_Data[0] = iCenter - iSize / 2.0;
      ret.m_Data[1] = iCenter + iSize / 2.0;
      return ret;
    }

    AABB2D(glm::vec<2,Real> iMin,glm::vec<2,Real> iSize)
    {
      m_Data[0] = iMin;
      m_Data[1] = iMin + iSize;
    }

    template <class OtherReal>
    explicit AABB2D(AABB2D<OtherReal> const& iOther)
    {
      m_Data[0] = glm::vec<2,Real>(iOther.m_Data[0]);
      m_Data[1] = glm::vec<2,Real>(iOther.m_Data[1]);
    }

    bool Empty() const
    {
      return m_Data[0].x == m_Data[1].x || m_Data[0].y == m_Data[1].y;
    }

    bool CircleTest(Vec2 const& iPos, Real iRadius)
    {
      if(Contains(iPos))
      {
        return true;
      }
      else
      {
        //Test segment X == X0
        Real dist = m_Data[0].x - iPos.x;   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real y = Math<Real>::Sqrt(dist) + iPos.y;
          if(Math<Real>::ZeroTolerance() < m_Data[1].y - y && y - m_Data[0].y > Math<Real>::ZeroTolerance())
            return true;
        }
        //Test segment X == X1
        dist = m_Data[1].x - iPos.x;   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real y = Math<Real>::Sqrt(dist) + iPos.y;
          if(Math<Real>::ZeroTolerance() < m_Data[1].y - y && y - m_Data[0].y > Math<Real>::ZeroTolerance())
            return true;
        }

        //TestSegment Y == Y0
        dist = m_Data[0].y - iPos.y;   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real x = Math<Real>::Sqrt(dist) + iPos.x;
          if(Math<Real>::ZeroTolerance() < m_Data[1].x - x && x - m_Data[0].x > Math<Real>::ZeroTolerance())
            return true;
        }
        //Test segment Y == Y1
        dist = m_Data[1].y - iPos.y;   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          Real x = Math<Real>::Sqrt(dist) + iPos.x;
          if(Math<Real>::ZeroTolerance() < m_Data[1].x - x && x - m_Data[0].x > Math<Real>::ZeroTolerance())
            return true;
        }
      }
      return false;
    }

    void Rotate(Real iAngle)
    {
      glm::vec<2,Real> center = GetCenter();
      glm::vec<2,Real> size = GetSize();
      glm::vec<2,Real> points[4];
      points[0] = size * -0.5;
      points[1] = glm::vec<2,Real>(size.x * -0.5, size.y *  0.5) ;
      points[2] = glm::vec<2,Real>(size.x *  0.5, size.y * -0.5) ;
      points[3] = size * 0.5;
      points[0] = glm::vec<2,Real>(points[0].x * Math<Real>::Cos(iAngle) + points[0].y * Math<Real>::Sin(iAngle)
                         , points[0].y * Math<Real>::Cos(iAngle) - points[0].x * Math<Real>::Sin(iAngle));
      points[1] = glm::vec<2,Real>(points[1].x * Math<Real>::Cos(iAngle) + points[1].y * Math<Real>::Sin(iAngle)
                         , points[1].y * Math<Real>::Cos(iAngle) - points[1].x * Math<Real>::Sin(iAngle));
      points[2] = glm::vec<2,Real>(points[2].x * Math<Real>::Cos(iAngle) + points[2].y * Math<Real>::Sin(iAngle)
                         , points[2].y * Math<Real>::Cos(iAngle) - points[2].x * Math<Real>::Sin(iAngle));
      points[3] = glm::vec<2,Real>(points[3].x * Math<Real>::Cos(iAngle) + points[3].y * Math<Real>::Sin(iAngle)
                         , points[3].y * Math<Real>::Cos(iAngle) - points[3].x * Math<Real>::Sin(iAngle));

      m_Data[0] = m_Data[1] = points[0];
      for (unsigned int i = 1; i < 4; ++i)
      {
        m_Data[0].x = Math<Real>::Min(m_Data[0].x, points[i].x);
        m_Data[0].y = Math<Real>::Min(m_Data[0].y, points[i].y);
        m_Data[1].x = Math<Real>::Max(m_Data[1].x, points[i].x);
        m_Data[1].y = Math<Real>::Max(m_Data[1].y, points[i].y);
      }
      m_Data[0] += center;
      m_Data[1] += center;
    }

    glm::vec<2,Real> GetCenter()const
    {
      return (m_Data[0] + m_Data[1])/Real(2);
    }

    glm::vec<2,Real> GetSize()const
    {
      return m_Data[1] - m_Data[0];
    }

    void Inflate(Real iCoeff)
    {
      glm::vec<2,Real> center = (m_Data[1] + m_Data[0]) / 2;
      glm::vec<2,Real> size = m_Data[1] - m_Data[0];
      size = size * iCoeff;
      m_Data[0] = center - size / 2.0;
      m_Data[1] = center + size / 2.0;
    }

    Real MinX()const{return m_Data[0].x;}
    Real MinY()const{return m_Data[0].y;}
    Real MaxX()const{return m_Data[1].x;}
    Real MaxY()const{return m_Data[1].y;}
    
    Real& MinX(){return m_Data[0].x;}
    Real& MinY(){return m_Data[0].y;}
    Real& MaxX(){return m_Data[1].x;}
    Real& MaxY(){return m_Data[1].y;}

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
		  return m_Data[0].x == m_Data[1].x || m_Data[0].y == m_Data[1].y;
	  }

    void SetCommonBox(AABB2D<Real> const& iBox1,AABB2D<Real> const& iBox2)
    {
      m_Data[0].x = Math<Real>::Max(iBox1.m_Data[0].x,iBox2.m_Data[0].x);
      m_Data[0].y = Math<Real>::Max(iBox1.m_Data[0].y,iBox2.m_Data[0].y);
      m_Data[1].x = Math<Real>::Max(Math<Real>::Min(iBox1.m_Data[1].x,iBox2.m_Data[1].x),m_Data[0].x);
      m_Data[1].y = Math<Real>::Max(Math<Real>::Min(iBox1.m_Data[1].y,iBox2.m_Data[1].y),m_Data[0].y);
    }

    void Absorb(glm::vec<2,Real> const& iPoint)
    {
      m_Data[0].x = Math<Real>::Min(m_Data[0].x, iPoint.x);
      m_Data[0].y = Math<Real>::Min(m_Data[0].y, iPoint.y);
      m_Data[1].x = Math<Real>::Max(m_Data[1].x, iPoint.x);
      m_Data[1].y = Math<Real>::Max(m_Data[1].y, iPoint.y);
    }

    void Absorb(AABB2D<Real> const& iBox)
    {
      m_Data[0].x = Math<Real>::Min(m_Data[0].x,iBox.m_Data[0].x);
      m_Data[0].y = Math<Real>::Min(m_Data[0].y,iBox.m_Data[0].y);
      m_Data[1].x = Math<Real>::Max(m_Data[1].x,iBox.m_Data[1].x);
      m_Data[1].y = Math<Real>::Max(m_Data[1].y,iBox.m_Data[1].y);
    }

    bool IsInside(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZeroTolerance()) const
    {
      return (m_Data[0].x - iOther.m_Data[0].x >= iEpsilon && m_Data[0].y - iOther.m_Data[0].y >= iEpsilon
           && iOther.m_Data[1].x - m_Data[1].x >= iEpsilon && iOther.m_Data[1].y - m_Data[1].y >= iEpsilon );
    }

    bool Intersect(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZeroTolerance()) const
    {
      return iOther.m_Data[1].x - m_Data[0].x >= iEpsilon && m_Data[1].x - iOther.m_Data[0].x >= iEpsilon
          && iOther.m_Data[1].y - m_Data[0].y >= iEpsilon && m_Data[1].y - iOther.m_Data[0].y >= iEpsilon;
    }

    //0 No, 1 Left, 2 Right, 3 Down, 4 Up
    int Touch(AABB2D const& iOther, Real iEpsilon = Math<Real>::ZeroTolerance()) const
    {
      if(iOther.m_Data[1].y - m_Data[0].y >= iEpsilon && m_Data[1].y - iOther.m_Data[0].y >= iEpsilon)
      {
        Real val = m_Data[0].x - iOther.m_Data[1].x;
        if(val <= iEpsilon && val >= -iEpsilon)
          return 1;
        
        val = m_Data[1].x - iOther.m_Data[0].x;
        if(val <= iEpsilon && val >= -iEpsilon)
          return 2;
      }
      if(iOther.m_Data[1].x - m_Data[0].x >= iEpsilon && m_Data[1].x - iOther.m_Data[0].x >= iEpsilon)
      {
        Real val = m_Data[0].y - iOther.m_Data[1].y;
      
        if(val <= iEpsilon && val >= -iEpsilon)
          return 3;

        val = m_Data[1].y - iOther.m_Data[0].y;
        if(val <= iEpsilon && val >= -iEpsilon)
          return 4;
      }
      return 0;
    }

    Optional<glm::vec<2,Real>> SegmentTest(glm::vec<2,Real> const& iOrigin, glm::vec<2,Real> const& iDir, Real iEpsilon = Math<Real>::ZeroTolerance())
    {
      Real const scaleX = 1.0 / iDir.x;
      Real const scaleY = 1.0 / iDir.y;
      Real const signX = Math<Real>::Sign(scaleX);
      Real const signY = Math<Real>::Sign(scaleY);

      Real const nearTimeX = ((signX > 0 ? m_Data[0].x : m_Data[1].x) - iOrigin.x) * scaleX;
      Real const nearTimeY = ((signY > 0 ? m_Data[0].y : m_Data[1].y) - iOrigin.y) * scaleY;
      Real const farTimeX = ((signX > 0 ? m_Data[1].x : m_Data[0].x) - iOrigin.x) * scaleX;
      Real const farTimeY = ((signY > 0 ? m_Data[1].y : m_Data[0].y) - iOrigin.y) * scaleY;

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

    bool Contains(glm::vec<2,Real>const& iPoint, Real iEpsilon = Math<Real>::ZeroTolerance()) const
    {
      return iPoint.x - m_Data[0].x >= iEpsilon && m_Data[1].x - iPoint.x >= iEpsilon
          && iPoint.y - m_Data[0].y >= iEpsilon && m_Data[1].y - iPoint.y >= iEpsilon;
    }

    // Neg value -> distance to min, Pos value -> distance to max, 0, inside
    glm::vec<2, Real> Classify(glm::vec<2,Real> const& iPoint, Real iEpsilon = Math<Real>::ZeroTolerance()) const
    {
      glm::vec<2, Real> res(iPoint.x - m_Data[0].x, iPoint.y - m_Data[0].y);
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

    glm::vec<2,Real> GetDistX(AABB2D const& iOther) const
    {
      return glm::vec<2,Real>(iOther.m_Data[0].x - m_Data[0].x,m_Data[1].x - iOther.m_Data[1].x);
    }

    glm::vec<2,Real> GetDistY(AABB2D const& iOther) const
    {
      return glm::vec<2,Real>(iOther.m_Data[0].y - m_Data[0].y,m_Data[1].y - iOther.m_Data[1].y);
    }

    union
    {
      glm::vec<2,Real> m_Data[2];
      struct
      {
        glm::vec<2,Real> m_Min;
        glm::vec<2,Real> m_Max;
      };
    };
  };

  typedef AABB2D<int> AABB2Di; 
  typedef AABB2D<float> AABB2Df;
  typedef AABB2D<double> AABB2Dd;
}