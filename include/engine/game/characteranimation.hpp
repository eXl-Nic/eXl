#pragma once

#include <engine/enginelib.hpp>
#include <engine/game/character.hpp>
#include <engine/game/ability.hpp>

namespace eXl
{
  struct CharacterAnimEntry
  {
    GfxSpriteComponent* m_GfxComp = nullptr;
    CharacterSystem::StateFlags m_CurDir;
    boost::optional<GameCueName> m_CurrentActionCue;
    bool m_Walking;

    
  };

  using CharacterAnimTable = ObjectTable<CharacterAnimEntry>;
  using CharacterAnimHandle = CharacterAnimTable::Handle;

  class EXL_ENGINE_API CharacterAnimation
  {
  public:

    void OnWalkingStateChange(ObjectHandle iObj, uint32_t iNewState);
    void OnCueChange(ObjectHandle iObject, GameCueChange const& iChange);

    void Register(World& iWorld);
    void Tick(World& iWorld);

    void AddCharacter(ObjectHandle);
    void RemoveCharacter(ObjectHandle);

  private:
    void UpdateAnimation(ObjectHandle iObj, CharacterAnimEntry&);

    UnorderedMap<ObjectHandle, CharacterAnimHandle> m_Objects;
    CharacterAnimTable m_Entries;
    World* m_World = nullptr;
  };
}