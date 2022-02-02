/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef EXL_LUA

#include <core/lua/luamanager.hpp>

#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/script/luascriptsystem.hpp>

namespace eXl
{
  Transforms* GetTransforms(World& iWorld)
  {
    return iWorld.GetSystem<Transforms>();
  }

  GameDatabase* GetArchetypeSys(World& iWorld)
  {
    return iWorld.GetSystem<GameDatabase>();
  }

  GfxSystem* GetGfxSystem(World& iWorld)
  {
    return iWorld.GetSystem<GfxSystem>();
  }

  int GetPropertyData(lua_State* iState)
  {
    luabind::default_converter<GameDatabase*> converterSys;
    if (converterSys.match(iState, luabind::by_pointer<GameDatabase>(), -3) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for archetype system");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }
    
    luabind::default_converter<ObjectHandle> converterObject;
    if (converterObject.match(iState, luabind::by_value<ObjectHandle>(), -2) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for object handle");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    luabind::default_converter<PropertySheetName> converterProp;
    if (converterProp.match(iState, luabind::by_value<PropertySheetName>(), -1) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for property sheet name");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    GameDatabase* sys = converterSys.to_cpp(iState, luabind::by_pointer<GameDatabase>(), -3);
    ObjectHandle obj = converterObject.to_cpp(iState, luabind::by_value<ObjectHandle>(), -2);
    PropertySheetName prop = converterProp.to_cpp(iState, luabind::by_value<PropertySheetName>(), -1);

    lua_pop(iState, 3);

    DynObject propSheet = sys->ModifyData(obj, prop);
    if (propSheet.IsValid())
    {
      LuaManager::PushRefToLua(LuaManager::GetCurrentState().GetState(), propSheet.GetType(), propSheet.GetBuffer());
      return 1;
    }
    else
    {
      ConstDynObject propSheet = sys->GetData(obj, prop);
      if (propSheet.IsValid())
      {
        LuaManager::PushCopyToLua(LuaManager::GetCurrentState().GetState(), propSheet.GetType(), propSheet.GetBuffer());
        return 1;
      }
    }

    return 0;
  }

  LUA_REG_FUN(BindEngine)
  {
    luabind::module(iState, "eXl")[
      luabind::class_<ObjectHandle>("ObjectHandle")
        .def(luabind::constructor<>())
        .def(luabind::self == ObjectHandle()),

        luabind::class_<Transforms>("Transforms")
        //.def("AddTransform", &Transforms::AddTransform)
        .def("GetLocalTransform", &Transforms::GetLocalTransform)
        .def("GetWorldTransform", &Transforms::GetWorldTransform)
        .def("HasTransform", &Transforms::HasTransform)
        .def("UpdateTransform", &Transforms::UpdateTransform)
        .def("Attach", &Transforms::Attach)
        .def("Detach", &Transforms::Detach),

        luabind::class_<GfxSpriteComponent>("GfxSpriteComponent")
        .def("SetDesc", &GfxSpriteComponent::SetDesc)
        .def("SetOffset", &GfxSpriteComponent::SetOffset)
        .def("SetSize", &GfxSpriteComponent::SetSize)
        //.def("SetTileset", &GfxSpriteComponent::SetTileset)
        .def("SetTileName", &GfxSpriteComponent::SetTileName)
        .def("SetAnimationSpeed", &GfxSpriteComponent::SetAnimationSpeed)
        .def("SetRotateSprite", &GfxSpriteComponent::SetRotateSprite)
        .def("SetLayer", &GfxSpriteComponent::SetLayer)
        .def("SetTint", &GfxSpriteComponent::SetTint)
        .def("SetFlat", &GfxSpriteComponent::SetFlat),

        luabind::class_<GfxSystem>("GfxSystem")
        .def("CreateSpriteComponent", &GfxSystem::CreateSpriteComponent)
        .def("GetSpriteComponent", &GfxSystem::GetSpriteComponent),

        luabind::class_<World>("World")
        .def("CreateObject", (ObjectHandle (World::*)())&World::CreateObject)
        .def("DeleteObject", &World::DeleteObject)
        //.def("GetTransforms", &GetTransforms)
        //.def("GetArchetypeSys", &GetArchetypeSys)
        //.def("GetGfxSystem", &GetGfxSystem)
        .def("GetTransforms", &World::GetSystem<Transforms>)
        .def("GetGameDatabase", &World::GetSystem<GameDatabase>)
        .def("GetGfxSystem", &World::GetSystem<GfxSystem>),

        luabind::class_<GameDatabase>("GameDatabase"),

        luabind::def("GetWorld", &LuaScriptSystem::GetWorld)
    ];

    luabind::object _G = luabind::globals(iState);
    lua_pushcfunction(iState, &GetPropertyData);
    luabind::object getPropFun(luabind::from_stack(iState, -1));
    _G["eXl"]["GameDatabase"]["GetProperty"] = getPropFun;
    _G["eXl"]["PropertySheetName"] = _G["eXl"]["Name"];

    return 0;
  }
}

#endif