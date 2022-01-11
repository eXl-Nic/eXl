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
  class Tileset;
  class GfxSpriteComponent;

  class CharacterAnimation;

	class EXL_ENGINE_API CharacterSystem
    : public MovementManagerBase
	{
    DECLARE_RTTI(CharacterSystem, ComponentManager);
	public:

    enum class CharStateFlags
    {
      Idle = 1 << 2,
      Walking = 1 << 3
    };

    enum class ControlKind
    {
      Navigation,
      Remote,
      Predicted,
    };

    struct Desc
    {
      CharacterAnimation const* animation = nullptr;
      ControlKind controlKind;
      float maxSpeed = 1.0;
      float size = 1.0;
      PhysicKind kind;
    };

    static void Build(World& iWorld, ObjectHandle iObject, Vector3f const& iPosition, Desc const& iDesc);
    void Register(World& iWorld) override;

		void AddCharacter(ObjectHandle iObj, Desc const& iDesc);
    void DeleteComponent(ObjectHandle iObj) override;

		void SetSpeed(ObjectHandle iObj, float iSpeed);
		void SetCurDir(ObjectHandle iObj, Vector3f const& iDir);
    void SetState(ObjectHandle iObj, uint32_t iState);

    uint32_t GetCurrentState(ObjectHandle iObj);
    static uint32_t GetStateFromDir(Vector3f const& iDir, bool iMoving);
    Vector3f GetCurrentFacingDirection(ObjectHandle iObj);
    Desc const* GetDesc(ObjectHandle iObj);

    bool GrabObject(ObjectHandle iGrabber, ObjectHandle iGrabbed);
    bool ReleaseObject(ObjectHandle iGrabber, ObjectHandle iGrabbed);

    void Tick(float iDelta);

	protected:

    struct Entry
    {
      Desc m_Desc;
      uint32_t m_CurState = 0;
      KinematicEntryHandle m_KinematicEntry;
      CharacterAnimation const* m_Animation;
    };

    Optional<DenseGameDataStorage<Entry>> m_Characters;
  public:
    template <typename Functor>
    void Iterate(Functor&& iFn)
    {
      m_Characters->Iterate([&iFn](ObjectHandle iObject, Entry const& iEntry)
        {
          iFn(iObject, iEntry.m_Desc);
        });
    }
	};
}