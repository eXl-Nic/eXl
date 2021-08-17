#pragma once

#include <engine/game/ability.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luascriptbehaviour.hpp>

#define EXL_REFLECT_PROPERTY \
EXL_REFLECT; \
static PropertySheetName PropertyName()

namespace eXl
{
  class ComponentManifest;

  namespace DunAtk
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

    Matrix4f const& GetProjectionMatrix();
    Matrix4f GetRotationMatrix(Vector2f const& iDir);

    EXL_ENGINE_API ComponentName GfxSpriteComponentName();
    EXL_ENGINE_API ComponentName PhysicsComponentName();
    EXL_ENGINE_API ComponentName TriggerComponentName();
    EXL_ENGINE_API PropertySheetName VelocityName();
    EXL_ENGINE_API PropertySheetName GfxSpriteDescName();
    EXL_ENGINE_API PropertySheetName PhysicsInitDataName();

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

      Vector3f m_FireDir;
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

      PhysicsShapeType m_Type;
      Vector3f m_Dims = Vector3f::ONE;
      Vector3f m_Offset = Vector3f::ZERO;
    };

    struct EXL_ENGINE_API PhysicsCompTestData
    {
      EXL_REFLECT;
      PhysicsType m_Type;
      PhysicsCollisionCategory m_Category = PhysicsCollisionCategory::Default;
      Vector<PhysicsShape> m_Shapes;
    };

    struct EXL_ENGINE_API TriggerComponentDesc
    {
      TriggerComponentDesc();
      ~TriggerComponentDesc();
      TriggerComponentDesc(const TriggerComponentDesc&);
      TriggerComponentDesc(TriggerComponentDesc&&);

      EXL_REFLECT;
      ResourceHandle<LuaScriptBehaviour> m_Script;
      PhysicsShape m_Shape;
    };

    struct EXL_ENGINE_API TerrainCarver
    {
      PhysicsShape m_Shape;
      bool m_AffectNavMesh = true;

      EXL_REFLECT_PROPERTY;
    };

    GameDataView<Vector3f>* GetVelocities(World& iWorld);
  }

  EXL_REFLECT_ENUM(DunAtk::PhysicsType, eXl__DunAtk__PhysicsType, EXL_ENGINE_API);
  EXL_REFLECT_ENUM(DunAtk::PhysicsShapeType, eXl__DunAtk__PhysicsShapeType, EXL_ENGINE_API);
  EXL_REFLECT_ENUM(DunAtk::PhysicsCollisionCategory, eXl__DunAtk__PhysicsCollisionCategory, EXL_ENGINE_API);
}

#include <engine/physics/physicsdef.hpp>

namespace eXl
{
  namespace DunAtk
  {
    constexpr uint32_t s_BasePhFlags = PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation;
    //constexpr uint32_t s_BasePhFlags = 1<<4 | 1<<13 | 1<<14;
  }
}