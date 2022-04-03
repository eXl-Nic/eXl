/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/game/ability.hpp>
#include <engine/common/gamedata.hpp>
#include <engine/script/luascriptbehaviour.hpp>
#include <engine/map/map.hpp>

#define EXL_REFLECT_PROPERTY \
EXL_REFLECT; \
static PropertySheetName PropertyName()

namespace eXl
{
  class ComponentManifest;

  namespace EngineCommon
  {
    EXL_ENGINE_API ComponentManifest const& GetComponents();
    EXL_ENGINE_API PropertiesManifest GetBaseProperties();

    EXL_ENGINE_API GameTagName ActionLock();
    EXL_ENGINE_API GameTagName WalkingTag();
    EXL_ENGINE_API GameTagName AnimLocked();

    constexpr uint16_t s_CollideAllMask = -1;

    constexpr uint16_t s_DefaultCategory = 1;

    constexpr uint16_t s_WallCategory = 2;
    constexpr uint16_t s_WallMask = s_CollideAllMask;

    constexpr uint16_t s_CharacterCategory = 4;
    constexpr uint16_t s_CharacterMask = s_CollideAllMask;

    constexpr uint16_t s_TriggerCategory = 8;
    constexpr uint16_t s_TriggerMask = s_CollideAllMask & ~(s_WallCategory | s_TriggerCategory);

    constexpr uint16_t s_ProjectileCategory = 16;
    constexpr uint16_t s_ProjectileMask = s_CollideAllMask;

    constexpr uint16_t s_MovementSensorMask = s_CollideAllMask & ~(s_TriggerCategory | s_ProjectileCategory);

    constexpr uint32_t s_WorldToPixel = 8;

    Mat4 const& GetProjectionMatrix();
    Mat4 GetRotationMatrix(Vec2 const& iDir);

    EXL_ENGINE_API ComponentName GfxSpriteComponentName();
    EXL_ENGINE_API ComponentName PhysicsComponentName();
    EXL_ENGINE_API ComponentName TriggerComponentName();
    EXL_ENGINE_API ComponentName CharacterComponentName();
    EXL_ENGINE_API PropertySheetName VelocityName();
    EXL_ENGINE_API PropertySheetName GfxSpriteDescName();

    struct EXL_ENGINE_API GrabData
    {
      EXL_REFLECT_PROPERTY;

      bool canBeLifted = false;
      bool canBeThrown = false;
      bool breakOnRelease = false;
      float throwDamage = 1;
    };

    struct EXL_ENGINE_API HealthData
    {
      EXL_REFLECT_PROPERTY;

      float currentHealth = 1;
      float maxHealth = 1;
    };

    struct EXL_ENGINE_API TurretData
    {
      EXL_REFLECT_PROPERTY;

      Vec3 m_FireDir;
      float m_FireRate;
      bool m_AutoTarget;
      ResourceHandle <Archetype> m_ProjectileType;
    };

    enum class PhysicsShapeType
    {
      Sphere,
      Box
    };

    enum class PhysicsType
    {
      Static,
      Kinematic,
      Dynamic
    };

    enum class PhysicsCollisionCategory
    {
      Default,
      Wall,
      Character,
      Trigger,
      Projectile,
      None,
    };

    struct EXL_ENGINE_API PhysicsShape
    {
      EXL_REFLECT;

      AABB2Df Compute2DBox() const;

      // NB: Counts offset as additional radius
      float ComputeBoundingCircle2DRadius() const;

      PhysicsShapeType m_Type;
      Vec3 m_Dims = One<Vec3>();
      Vec3 m_Offset = Zero<Vec3>();
    };

    struct EXL_ENGINE_API ObjectShapeData
    {
      EXL_REFLECT_PROPERTY;

      AABB2Df Compute2DBox() const;
      float ComputeBoundingCircle2DRadius() const;

      bool IsSimpleBox() const
      {
        return m_Shapes.size() == 1 && m_Shapes[0].m_Type == PhysicsShapeType::Box
          && m_Shapes[0].m_Offset == Zero<Vec3>();
      }

      bool IsSimpleSphere() const
      {
        return m_Shapes.size() == 1 && m_Shapes[0].m_Type == PhysicsShapeType::Sphere
          && m_Shapes[0].m_Offset == Zero<Vec3>();
      }

      SmallVector<PhysicsShape, 1> m_Shapes;
    };

    struct EXL_ENGINE_API PhysicBodyData
    {
      EXL_REFLECT_PROPERTY;
      PhysicsType m_Type;
      PhysicsCollisionCategory m_Category = PhysicsCollisionCategory::Default;
    };

    struct EXL_ENGINE_API TriggerComponentDesc
    {
      TriggerComponentDesc();
      ~TriggerComponentDesc();
      TriggerComponentDesc(const TriggerComponentDesc&);
      TriggerComponentDesc(TriggerComponentDesc&&);

      EXL_REFLECT_PROPERTY;
      ResourceHandle<LuaScriptBehaviour> m_Script;
    };

    enum class CharacterControlKind
    {
      Navigation,
      PlayerControl,
      Remote
    };

    struct EXL_ENGINE_API CharacterDesc
    {
      EXL_REFLECT_PROPERTY;

      CharacterControlKind m_Control = CharacterControlKind::Navigation;
      float m_MaxSpeed = 10;
    };

    struct EXL_ENGINE_API TerrainCarver
    {
      TerrainTypeName m_TerrainType;

      EXL_REFLECT_PROPERTY;
    };

    GameDataView<Vec3>* GetVelocities(World& iWorld);
  }

  EXL_REFLECT_ENUM(EngineCommon::PhysicsType, eXl__EngineCommon__PhysicsType, EXL_ENGINE_API);
  EXL_REFLECT_ENUM(EngineCommon::PhysicsShapeType, eXl__EngineCommon__PhysicsShapeType, EXL_ENGINE_API);
  EXL_REFLECT_ENUM(EngineCommon::PhysicsCollisionCategory, eXl__EngineCommon__PhysicsCollisionCategory, EXL_ENGINE_API);
  EXL_REFLECT_ENUM(EngineCommon::CharacterControlKind, eXl__EngineCommon__CharacterControlKind, EXL_ENGINE_API);
}

#include <engine/physics/physicsdef.hpp>

namespace eXl
{
  namespace EngineCommon
  {
    constexpr uint32_t s_BasePhFlags = PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation;
  }
}