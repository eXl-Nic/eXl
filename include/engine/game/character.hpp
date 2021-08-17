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
      CharacterAnimation* animation = nullptr;
      ControlKind controlKind;
      float maxSpeed = 1.0;
      float size = 1.0;
      PhysicKind kind;
    };

    static ObjectHandle Build(World& iWorld, Vector3f const& iPosition, Desc const& iDesc);

		void AddCharacter(ObjectHandle iObj, Desc const& iDesc);
    void DeleteComponent(ObjectHandle iObj) override;

		void SetSpeed(ObjectHandle iObj, float iSpeed);
		void SetCurDir(ObjectHandle iObj, Vector3f const& iDir);
    void SetState(ObjectHandle iObj, uint32_t iState);

    uint32_t GetCurrentState(ObjectHandle iObj);
    static uint32_t GetStateFromDir(Vector3f const& iDir, bool iMoving);
    Vector3f GetCurrentFacingDirection(ObjectHandle iObj);

    bool GrabObject(ObjectHandle iGrabber, ObjectHandle iGrabbed);
    bool ReleaseObject(ObjectHandle iGrabber, ObjectHandle iGrabbed);

    void Tick(float iDelta);

	protected:

    struct Entry
    {
      Desc m_Desc;
      ObjectHandle m_Handle;
      uint32_t m_CurState = 0;
      KinematicEntryHandle m_KinematicEntry;
      CharacterAnimation* m_Animation;
    };

    ObjectTable<Entry> m_Entries;
    UnorderedMap<ObjectHandle, ObjectTable<Entry>::Handle> m_Characters;
	};
}