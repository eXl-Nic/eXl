#include <engine/game/scripttrigger.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <engine/physics/physicsys.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ScriptTriggerSystem)

  ScriptTrigger::ScriptTrigger(World& iWorld)
    : m_World(iWorld)
    , m_Scripts(*m_World.GetSystem<LuaScriptSystem>())
  {
    m_BehaviourName = "Trigger";
    m_EnterName = "Enter";
    m_LeaveName = "Leave";
  }

  void ScriptTrigger::OnEnter(const Vector<ObjectPair>& iNewPairs)
  {
    for (auto const& pair : iNewPairs)
    {
      m_Scripts.CallBehaviour<void>(pair.first, m_BehaviourName, m_EnterName, pair.first, pair.second);
    }
  }

  void ScriptTrigger::OnLeave(const Vector<ObjectPair>& iNewPairs)
  {
    for (auto const& pair : iNewPairs)
    {
      m_Scripts.CallBehaviour<void>(pair.first, m_BehaviourName, m_LeaveName, pair.first, pair.second);
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