#pragma once

#include <engine/enginelib.hpp>
#include <engine/game/movementmanager.hpp>

namespace eXl
{
  class World;
  class GfxSpriteComponent;

	class EXL_ENGINE_API ProjectileSystem
    : public MovementManagerBase
	{
    DECLARE_RTTI(ProjectileSystem, ComponentManager);
	public:

    struct Desc
    {
      bool registerNav = true;
      bool rotateSprite = false;
      float size = 1.0;
      PhysicKind kind = PhysicKind::Ghost;
      ContactFilterCallback contactCb;
    };

    static ObjectHandle Build(World& iWorld, Vector3f const& iPosition, Desc const& iDesc);

		void AddProjectile(ObjectHandle iObj, Desc const& iDesc, Vector3f const& iInitialSpeed);
    void DeleteComponent(ObjectHandle iObj) override;

    void Tick(float iDelta);

	protected:

    struct Entry
    {
      Desc m_Desc;
      ObjectHandle m_Handle;
      uint32_t m_CurState = 0;
      ObjectTable<KinematicEntry>::Handle m_KinematicEntry;
      GfxSpriteComponent* m_GfxComp = nullptr;
    };

    ObjectTable<Entry> m_Entries;
    UnorderedMap<ObjectHandle, ObjectTable<Entry>::Handle> m_Projectiles;
	};
}