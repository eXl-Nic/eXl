/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef EXL_LUA

#include <core/lua/luamanager.hpp>
#include <core/lua/luascript.hpp>
#include <engine/script/eventsystem.hpp>
#include <boost/optional.hpp>
#include <core/path.hpp>

namespace eXl
{
  class LuaScriptBehaviour;

  class EXL_ENGINE_API LuaScriptSystem : public ComponentManager
  {
    DECLARE_RTTI(LuaScriptSystem, ComponentManager);
  public:

    LuaScriptSystem();
    ~LuaScriptSystem();

    void LoadScript(const LuaScriptBehaviour& iBehaviour);

    void AddBehaviour(ObjectHandle, const LuaScriptBehaviour& iBehaviour);
    void DeleteComponent(ObjectHandle) override;

    static World* GetWorld_Static();
  protected:
    
    struct ScriptEntry
    {
      ResourceHandle<LuaScriptBehaviour> m_ScriptHandle;
      luabind::object m_ScriptObject;
      luabind::object m_InitFunction;
      UnorderedMap<Name, luabind::object> m_ScriptFunctions;
    };

    ObjectTable<ScriptEntry> m_Scripts;
    using ScriptHandle = ObjectTableHandle<ScriptEntry>;
    UnorderedMap<Resource::UUID, ScriptHandle> m_LoadedScripts;

    ScriptHandle LoadScript_Internal(const LuaScriptBehaviour& iBehaviour);

    LuaWorld m_LuaWorld;
  };
}
#endif