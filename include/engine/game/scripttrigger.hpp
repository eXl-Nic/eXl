/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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