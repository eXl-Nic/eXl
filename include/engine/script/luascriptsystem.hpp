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
#include <engine/common/gamedata.hpp>
#include <engine/common/coroutine.hpp>
#include <boost/optional.hpp>
#include <core/path.hpp>

namespace eXl
{
  class LuaScript;
  class LuaEventHandler;
  class LuaCoroutine;
  class LuaFunctionLibrary;

  class EXL_ENGINE_API LuaScriptSystem : public ComponentManager
  {
    DECLARE_RTTI(LuaScriptSystem, ComponentManager);
  public:

    LuaScriptSystem();
    ~LuaScriptSystem();

    void Register(World& iWorld) override;

    void LoadScript(const LuaScript& iScript);

    void AddHandler(ObjectHandle, const LuaEventHandler& iHandler);
    void AddCoroutine(ObjectHandle, const LuaCoroutine& iHandler);

    void PauseCoroutine(ObjectHandle);
    void ResumeCoroutine(ObjectHandle);

    void DeleteComponent(ObjectHandle) override;

    void Tick();

    static World* GetWorld_Static();
  protected:
    
    struct Impl;

    std::unique_ptr<Impl> m_Impl;
  };
}
#endif