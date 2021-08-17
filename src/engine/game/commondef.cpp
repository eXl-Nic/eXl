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