/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/commondef.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  namespace EngineCommon
  {
    GameDataView<Vec3>* GetVelocities(World& iWorld)
    {
      GameDatabase* database = iWorld.GetSystem<GameDatabase>();
      if (database)
      {
        return database->GetView<Vec3>(VelocityName());
      }
      return nullptr;
    }

    Mat4 const& GetProjectionMatrix()
    {
      static Mat4 s_Proj = []
      {
        Mat4 newMatrix = Identity<Mat4>();
        newMatrix[2][1] = 1.0 / Mathf::Sqrt(2.0);
        return newMatrix;
      }();

      return s_Proj;
    }

    Mat4 GetRotationMatrix(Vec2 const& iDir)
    {
      Mat4 newMatrix = Identity<Mat4>();
      newMatrix[0] = Vec4(iDir, 0, 0);
      newMatrix[1] = Vec4(-iDir.y, iDir.x, 0, 0);

      return newMatrix;
    }

    TriggerComponentDesc::TriggerComponentDesc() = default;
    TriggerComponentDesc::~TriggerComponentDesc() = default;
    TriggerComponentDesc::TriggerComponentDesc(const TriggerComponentDesc&) = default;
    TriggerComponentDesc::TriggerComponentDesc(TriggerComponentDesc&&) = default;
  }

  namespace EngineCommon
  {
    AABB2Df PhysicsShape::Compute2DBox() const
    {
      if (m_Type == PhysicsShapeType::Box)
      {
        return AABB2Df::FromCenterAndSize(MathTools::As2DVec(m_Offset)
          , MathTools::As2DVec(m_Dims));
      }
      return AABB2Df(m_Offset.x - m_Dims.x, m_Offset.y - m_Dims.x
        , m_Offset.y - m_Dims.x, m_Offset.y - m_Dims.x);
    }

    float PhysicsShape::ComputeBoundingCircle2DRadius() const
    {
      if (m_Type == PhysicsShapeType::Box)
      {
        return length(Vec2(m_Offset + m_Dims * 0.5));
      }
      return length(m_Offset) + m_Dims.x;
    }

    AABB2Df ObjectShapeData::Compute2DBox() const
    {
      if (m_Shapes.empty())
      {
        return AABB2Df();
      }
      AABB2Df box = m_Shapes[0].Compute2DBox();
      for (uint32_t i = 1; i < m_Shapes.size(); ++i)
      {
        box.Absorb(m_Shapes[i].Compute2DBox());
      }
      return box;
    }

    float ObjectShapeData::ComputeBoundingCircle2DRadius() const
    {
      if (m_Shapes.empty())
      {
        return 0;
      }
      float maxRadius = m_Shapes[0].ComputeBoundingCircle2DRadius();
      for (uint32_t i = 1; i < m_Shapes.size(); ++i)
      {
        float radius = m_Shapes[i].ComputeBoundingCircle2DRadius();
        maxRadius = Mathf::Max(radius, maxRadius);
      }

      return maxRadius;
    }
  }
}