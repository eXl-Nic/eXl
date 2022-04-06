/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/scripttrigger.hpp>

#ifdef EXL_LUA

#include <engine/script/luascriptsystem.hpp>
#include <engine/physics/physicsys.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ScriptTriggerSystem)

  ScriptTrigger::ScriptTrigger(World& iWorld)
    : m_World(iWorld)
  {
    m_BehaviourName = "Trigger";
    m_EnterName = m_BehaviourName + "::Enter";
    m_LeaveName = m_BehaviourName + "::Leave";
  }

  void ScriptTrigger::OnEnter(const Vector<ObjectPair>& iNewPairs)
  {
    EventSystem& events = *m_World.GetSystem<EventSystem>();
    for (auto const& pair : iNewPairs)
    {
      events.Dispatch<void>(pair.first, m_EnterName, pair.first, pair.second);
    }
  }

  void ScriptTrigger::OnLeave(const Vector<ObjectPair>& iNewPairs)
  {
    EventSystem& events = *m_World.GetSystem<EventSystem>();
    for (auto const& pair : iNewPairs)
    {
      events.Dispatch<void>(pair.first, m_LeaveName, pair.first, pair.second);
    }
  }

  void ScriptTriggerSystem::Register(World& iWorld)
  {
    WorldSystem::Register(iWorld);
    PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
    eXl_ASSERT_REPAIR_RET(phSys != nullptr, );
    m_Handle = phSys->AddTriggerCallback(std::make_unique<ScriptTrigger>(iWorld));
  }
}

#endif