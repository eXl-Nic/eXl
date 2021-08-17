#pragma once

#include <engine/physics/trigger.hpp>

namespace eXl
{
  class LuaScriptSystem;

  struct ScriptTrigger : TriggerCallback
  {
    ScriptTrigger(World& iWorld);

    void OnEnter(const Vector<ObjectPair>& iNewPairs);
    void OnLeave(const Vector<ObjectPair>& iNewPairs);

    World& m_World;
    LuaScriptSystem& m_Scripts;
  private:
    Name m_BehaviourName;
    Name m_EnterName;
    Name m_LeaveName;
  };

  class ScriptTriggerSystem : public WorldSystem
  {
    DECLARE_RTTI(ScriptTriggerSystem, WorldSystem);
  public:
    void Register(World& iWorld);

    TriggerCallbackHandle GetScriptCallbackhandle() const { return m_Handle; }
  protected:
    TriggerCallbackHandle m_Handle;
  };
}