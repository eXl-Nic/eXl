/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once 

#include <math/math.hpp>
#include <engine/enginelib.hpp>

namespace eXl
{
  struct Box3D;

  struct EXL_ENGINE_API BoundingSphere
  {
    static BoundingSphere FromBox(Box3D const& iSphere);

    Vec3 m_Center;
    float m_Radius;
  };

  struct EXL_ENGINE_API Box3D
  {
    Vec3 m_Center;
    Vec3 m_HalfExtent = Vec3(0,0,0);

    static Box3D FromSphere(BoundingSphere const& iSphere)
    {
      return Box3D{ iSphere.m_Center, One<Vec3>() * iSphere.m_Radius };
    }

    static Box3D FromExtremas(Vec3 const& iMin, Vec3 const& iMax)
    {
      return Box3D{ (iMin + iMax) * 0.5f, (iMax - iMin) * 0.5f };
    }

    bool IsEmpty() const { return m_HalfExtent == Zero<Vec3>(); }
    Box3D Merge(Box3D const& other) const;
    Box3D Merge(Vec3 const& iPoint) const;
    Vec3 Min() const { return m_Center - m_HalfExtent; }
    Vec3 Max() const { return m_Center + m_HalfExtent; }
    Vec3 Size() const { return m_HalfExtent * 2; }
    void GetPoints(Vec3* oPoints) const;

  };


  EXL_ENGINE_API Box3D operator*(Mat4 const& iMat, Box3D const& iBox);

  inline BoundingSphere BoundingSphere::FromBox(Box3D const& iBox)
  {
    return BoundingSphere{ iBox.m_Center, length(iBox.m_HalfExtent) };
  }

  struct EXL_ENGINE_API Plane
  {
    Vec3 m_PlaneNormal;
    float m_Dist;

    static Plane FromPointAndNormal(Vec3 const& iNormal, Vec3 const& iPoint)
    {
      Plane plane;
      plane.m_PlaneNormal = glm::normalize(iNormal);
      plane.m_Dist = -dot(iPoint, plane.m_PlaneNormal);

      return plane;
    }

    float SignedDistanceToPlane(Vec3 const& iPoint) const
    {
       return dot(iPoint, m_PlaneNormal) + m_Dist;
    }
  };
}