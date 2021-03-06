/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/common/app.hpp>
#include <engine/map/map.hpp>
#include <engine/game/character.hpp>

namespace eXl
{
  class CharacterAnimation;

  class EXL_ENGINE_API Scenario_Base : public Scenario
  {
    DECLARE_RTTI(Scenario_Base, Scenario);
  public:

    Scenario_Base();
    Scenario_Base(Scenario_Base const&) = delete;
    ~Scenario_Base();

    void Init(World& iWorld) override;

    void StartLocal(World& iWorld);

    void SetMap(ResourceHandle<MapResource> iMap)
    {
      m_Map = iMap;
    }

    void SetMainChar(ResourceHandle<Archetype> iChar)
    {
      m_MainCharacter = iChar;
    }

    ObjectHandle GetMainChar()
    {
      return m_MainChar;
    }

    MapResource::InstanceData const& GetMapData() { return m_InstatiatedMap; }

    CharacterAnimation const& GetDefaultAnimation()
    {
      return *m_DefaultAnim;
    }

    ResourceHandle<MapResource> const& GetMapHandle() const { return m_Map; }
    ResourceHandle<Archetype> const& GetMainCharHandle() const { return m_MainCharacter; }

  protected:

    ResourceHandle<MapResource> m_Map;
    ResourceHandle<Archetype> m_MainCharacter;
    MapResource::InstanceData m_InstatiatedMap;

    ObjectHandle m_MainChar;

    ObjectHandle SpawnCharacter(World& iWorld, Vec3 const& iPos, EngineCommon::CharacterControlKind iControl, ObjectCreationInfo const& iInfo = ObjectCreationInfo());
    Vec2 m_SpawnPos;

    UniquePtr<CharacterAnimation> m_DefaultAnim;

    void ProcessInputs(World& iWorld);
    uint32_t dirMask = 0;
    bool keyChanged = false;
    Vec2i curMousePos;
  };
}