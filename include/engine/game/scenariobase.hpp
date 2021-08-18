/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/common/app.hpp>
#include <engine/net/network.hpp>
#include <engine/map/map.hpp>

namespace eXl
{
  class EXL_ENGINE_API Scenario_Base : public Scenario
  {
  public:

    void Init(World& iWorld) override;

    void StartServer(World& iWorld);

    void StartClient(World& iWorld, String const& iURL);

    void StartLocal(World& iWorld);

    void SetMap(ResourceHandle<MapResource> iMap)
    {
      m_Map = iMap;
    }

    ObjectHandle GetMainChar()
    {
      return m_MainChar;
    }

    MapResource::InstanceData const& GetMapData() { return m_InstatiatedMap; }

  protected:

    ResourceHandle<MapResource> m_Map;
    MapResource::InstanceData m_InstatiatedMap;

    ObjectHandle m_MainChar;
    Network::NetRole m_CurMode = Network::NetRole::None;

    ObjectHandle SpawnCharacter(World& iWorld, Network::NetRole iRole);
    Vector2f m_SpawnPos;

    void ProcessInputs(World& iWorld);
    uint32_t dirMask = 0;
    bool keyChanged = false;
  };
}