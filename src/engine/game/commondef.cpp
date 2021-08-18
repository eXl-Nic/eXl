/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/commondef.hpp>
#include <engine/script/luascriptsystem.hpp>

namespace eXl
{
  namespace DunAtk
  {
    GameDataView<Vector3f>* GetVelocities(World& iWorld)
    {
      GameDatabase* database = iWorld.GetSystem<GameDatabase>();
      if (database)
      {
        return database->GetView<Vector3f>(VelocityName());
      }
      return nullptr;
    }

    Matrix4f const& GetProjectionMatrix()
    {
      static Matrix4f s_Proj = []
      {
        Matrix4f newMatrix;
        newMatrix.MakeIdentity();
        newMatrix.m_Matrix[2][1] = 1.0 / Mathf::Sqrt(2.0);
        return newMatrix;
      }();

      return s_Proj;
    }

    Matrix4f GetRotationMatrix(Vector2f const& iDir)
    {
      Matrix4f newMatrix;
      newMatrix.MakeIdentity();
      *reinterpret_cast<Vector2f*>(newMatrix.m_Data + 0) = iDir;
      *reinterpret_cast<Vector2f*>(newMatrix.m_Data + 4) = Vector2f(-iDir.Y(), iDir.X());

      return newMatrix;
    }

    TriggerComponentDesc::TriggerComponentDesc() = default;
    TriggerComponentDesc::~TriggerComponentDesc() = default;
    TriggerComponentDesc::TriggerComponentDesc(const TriggerComponentDesc&) = default;
    TriggerComponentDesc::TriggerComponentDesc(TriggerComponentDesc&&) = default;
  }
}