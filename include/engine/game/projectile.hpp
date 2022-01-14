/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

    void Register(World& iWorld) override;

    static ObjectHandle Build(World& iWorld, Vector3f const& iPosition, Desc const& iDesc);

		void AddProjectile(ObjectHandle iObj, Desc const& iDesc, Vector3f const& iInitialSpeed);
    void DeleteComponent(ObjectHandle iObj) override;

    void Tick(float iDelta);

	protected:

    struct Entry
    {
      Desc m_Desc;
      uint32_t m_CurState = 0;
      ObjectTable<KinematicEntry>::Handle m_KinematicEntry;
      GfxSpriteComponent* m_GfxComp = nullptr;
    };

    Optional<DenseGameDataStorage<Entry>> m_Entries;
    UnorderedMap<ObjectHandle, ObjectTable<Entry>::Handle> m_Projectiles;
	};
}