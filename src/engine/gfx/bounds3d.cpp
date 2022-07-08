/*
Copyright 2022 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <engine/gfx/bounds3d.hpp>

namespace eXl
{
  Box3D Box3D::Merge(Box3D const& iOther) const
  {
    if (IsEmpty())
    {
      return iOther;
    }
    if (iOther.IsEmpty())
    {
      return *this;
    }

    Vec3 minPt;
    Vec3 maxPt;

    minPt.x = glm::min(m_Center.x - m_HalfExtent.x, iOther.m_Center.x - iOther.m_HalfExtent.x);
    minPt.y = glm::min(m_Center.y - m_HalfExtent.y, iOther.m_Center.y - iOther.m_HalfExtent.y);
    minPt.z = glm::min(m_Center.z - m_HalfExtent.z, iOther.m_Center.z - iOther.m_HalfExtent.z);

    maxPt.x = glm::max(m_Center.x + m_HalfExtent.x, iOther.m_Center.x + iOther.m_HalfExtent.x);
    maxPt.y = glm::max(m_Center.y + m_HalfExtent.y, iOther.m_Center.y + iOther.m_HalfExtent.y);
    maxPt.z = glm::max(m_Center.z + m_HalfExtent.z, iOther.m_Center.z + iOther.m_HalfExtent.z);

    return FromExtremas(minPt, maxPt);
  }

  Box3D Box3D::Merge(Vec3 const& iPt) const
  {
    if (IsEmpty())
    {
      return Box3D{iPt, One<Vec3>() * FLT_MIN};
    }

    Vec3 minPt;
    Vec3 maxPt;

    minPt.x = glm::min(m_Center.x - m_HalfExtent.x, iPt.x);
    minPt.y = glm::min(m_Center.y - m_HalfExtent.y, iPt.y);
    minPt.z = glm::min(m_Center.z - m_HalfExtent.z, iPt.z);

    maxPt.x = glm::max(m_Center.x + m_HalfExtent.x, iPt.x);
    maxPt.y = glm::max(m_Center.y + m_HalfExtent.y, iPt.y);
    maxPt.z = glm::max(m_Center.z + m_HalfExtent.z, iPt.z);

    return FromExtremas(minPt, maxPt);
  }

  Box3D operator*(Mat4 const& iMat, Box3D const& iBox)
  {
    Vec3 minPt = iMat * Vec4(iBox.Min(), 1);
    Vec3 maxPt = minPt;

    auto UpdateMinMax = [&minPt, &maxPt](Vec3 const& iNewPt)
    {
      minPt.x = glm::min(minPt.x, iNewPt.x);
      minPt.y = glm::min(minPt.y, iNewPt.y);
      minPt.z = glm::min(minPt.z, iNewPt.z);

      maxPt.x = glm::max(maxPt.x, iNewPt.x);
      maxPt.y = glm::max(maxPt.y, iNewPt.y);
      maxPt.z = glm::max(maxPt.z, iNewPt.z);
    };

    UpdateMinMax(iMat * Vec4(iBox.m_Center.x + iBox.m_HalfExtent.x, iBox.m_Center.y - iBox.m_HalfExtent.y, iBox.m_Center.z - iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x - iBox.m_HalfExtent.x, iBox.m_Center.y + iBox.m_HalfExtent.y, iBox.m_Center.z - iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x + iBox.m_HalfExtent.x, iBox.m_Center.y + iBox.m_HalfExtent.y, iBox.m_Center.z - iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x - iBox.m_HalfExtent.x, iBox.m_Center.y - iBox.m_HalfExtent.y, iBox.m_Center.z + iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x + iBox.m_HalfExtent.x, iBox.m_Center.y - iBox.m_HalfExtent.y, iBox.m_Center.z + iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x - iBox.m_HalfExtent.x, iBox.m_Center.y + iBox.m_HalfExtent.y, iBox.m_Center.z + iBox.m_HalfExtent.z, 1));
    UpdateMinMax(iMat * Vec4(iBox.m_Center.x + iBox.m_HalfExtent.x, iBox.m_Center.y + iBox.m_HalfExtent.y, iBox.m_Center.z + iBox.m_HalfExtent.z, 1));

    return Box3D::FromExtremas(minPt, maxPt);
  }
}