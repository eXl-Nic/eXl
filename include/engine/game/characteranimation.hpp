/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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