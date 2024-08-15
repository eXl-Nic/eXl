/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef EXL_LUA

#include <core/lua/luamanager.hpp>
#include <core/type/tagtype.hpp>

#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/common/coroutine.hpp>
#include <engine/script/eventsystem.hpp>
#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <engine/game/character.hpp>
#include <engine/game/archetype.hpp>

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

  static int GetPropertyData(lua_State* iState, bool iConst)
  {
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

    World* world = LuaScriptSystem::GetWorld_Static();
    if (world == nullptr)
    {
      lua_pushliteral(iState, "Calling lua functions outside of the script system");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }
    //GameDatabase* sys = converterSys.to_cpp(iState, luabind::by_pointer<GameDatabase>(), -3);
    GameDatabase* sys = world->GetSystem<GameDatabase>();
    ObjectHandle obj = converterObject.to_cpp(iState, luabind::by_value<ObjectHandle>(), -2);
    PropertySheetName prop = converterProp.to_cpp(iState, luabind::by_value<PropertySheetName>(), -1);

    lua_pop(iState, 2);
    if (iConst)
    {
      ConstDynObject propSheet = sys->GetData(obj, prop);
      if (propSheet.IsValid())
      {
        LuaManager::PushRefToLua(LuaManager::GetCurrentState().GetState(), propSheet.GetType(), propSheet.GetBuffer());
        return 1;
      }
    }
    else
    {
      DynObject propSheet = sys->ModifyData(obj, prop);
      if (propSheet.IsValid())
      {
        LuaManager::PushRefToLua(LuaManager::GetCurrentState().GetState(), propSheet.GetType(), propSheet.GetBuffer(), false);
        return 1;
      }
    }

    return 0;
  }

  static int ReadPropertyData(lua_State* iState)
  {
    return GetPropertyData(iState, true);
  }

  static int AccessPropertyData(lua_State* iState)
  {
    return GetPropertyData(iState, false);
  }

  int LuaTriggerEvent(lua_State* iState)
  {
    int numArgs = lua_gettop(iState);

    if (numArgs < 2)
    {
      lua_pop(iState, numArgs);
      lua_pushliteral(iState, "Incorrect number of arguments for TriggerEvent");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    luabind::default_converter<ObjectHandle> converterObject;
    if (converterObject.match(iState, luabind::by_value<ObjectHandle>(), 1) < 0)
    {
      lua_pop(iState, numArgs);
      lua_pushliteral(iState, "Incorrect argument for object handle");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    luabind::default_converter<Name> converterName;
    if (converterName.match(iState, luabind::by_value<Name>(), 2) < 0)
    {
      lua_pop(iState, numArgs);
      lua_pushliteral(iState, "Incorrect argument for property sheet name");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    World* world = LuaScriptSystem::GetWorld_Static();

    EventSystem* sys = world->GetSystem<EventSystem>();
    ObjectHandle obj = converterObject.to_cpp(iState, luabind::by_value<ObjectHandle>(), 1);
    Name eventName = converterName.to_cpp(iState, luabind::by_value<Name>(), 2);

    FunDesc const* desc = sys->GetFunDesc(eventName);

    if (desc == nullptr)
    {
      lua_pop(iState, numArgs);
      lua_pushliteral(iState, "Incorrect event name");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Event " << eventName << " does not exist \n" << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    if (desc->GetArgs().size()!= (numArgs - 2))
    {
      lua_pop(iState, numArgs);
      lua_pushliteral(iState, "Incorrect number of arguments for event");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Event " << eventName << " needs " << desc->GetArgs().size() << " arguments\n" 
        << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    EventSystem::HandlerEntry const* entry = sys->GetEventHandlerInternal(obj, eventName);
    if (entry == nullptr)
    {
      lua_pop(iState, numArgs);
      if (desc->GetRetType() == nullptr)
      {
        return 0;
      }
      else
      {
        lua_pushnil(iState);
        return 1;
      }
    }

    ArgsBuffer const& buffType(desc->GetType());
    DynObject argsObj;
    argsObj.SetType(&buffType, buffType.Alloc(), true);

    Vector<uint8_t const*> args;

    Err res = LuaManager::ArgsFromLua(iState, 3, desc->GetArgs(), argsObj, args);

    lua_pop(iState, numArgs);
    if (!res)
    {
      lua_pushliteral(iState, "Could not retrieve arguments");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }


    DynObject output;
    if(desc->GetRetType() != nullptr)
    {
      output.SetType(desc->GetRetType(), desc->GetRetType()->Build(), true);
    }

    entry->m_Handler(*world, obj, eventName, args.data(), output, entry->m_Payload);

    if (desc->GetRetType() == nullptr)
    {
      return 0;
    }
    else
    {
      LuaManager::PushCopyToLua(iState, desc->GetRetType(), output.GetBuffer());
      return 1;
    }
  }

  int InstantiateArchetype_Script(lua_State* iState)
  {
    luabind::default_converter<ObjectHandle> converterObject;
    if (converterObject.match(iState, luabind::by_value<ObjectHandle>(), -3) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for object handle");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    luabind::default_converter<Archetype const*> converterArch;
    if (converterArch.match(iState, luabind::by_const_pointer<Archetype>(), -2) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for property sheet name");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    World* world = LuaScriptSystem::GetWorld_Static();
    if (world == nullptr)
    {
      lua_pushliteral(iState, "Calling lua functions outside of the script system");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }
    //GameDatabase* sys = converterSys.to_cpp(iState, luabind::by_pointer<GameDatabase>(), -3);
    GameDatabase* sys = world->GetSystem<GameDatabase>();
    ObjectHandle obj = converterObject.to_cpp(iState, luabind::by_value<ObjectHandle>(), -3);
    Archetype const* archetype = converterArch.to_cpp(iState, luabind::by_const_pointer<Archetype>(), -2);

    if (!world->IsObjectValid(obj))
    {
      lua_pushstring(iState, "Invalid object");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    if (archetype == nullptr)
    {
      lua_pushstring(iState, "Invalid archetype");
      Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      return lua_error(iState);
    }

    CustomizationData data;
    if (!lua_isnil(iState, -1) && lua_istable(iState, -1))
    {
      luabind::table customTable(luabind::from_stack(iState, -1));
      for (auto iter = luabind::iterator(customTable); iter != luabind::iterator(); ++iter)
      {
        String prop;
        void* nameBuffer = &prop;
        if (!TypeManager::GetType<String>()->ConvertFromLua(iter.key(), nameBuffer))
        {
          Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Invalid key in customization table" << "\n"  << LuaManager::StackDump(iState);
          continue;
        }

        ConstDynObject const& archProp = archetype->GetProperty(prop);
        if (!archProp.IsValid())
        {
          Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Property " << prop <<" not found in archetype" << "\n" << LuaManager::StackDump(iState);
          continue;
        }

        TupleType const* type = archProp.GetType()->IsTuple();
        if (!type)
        {
          continue;
        }
        
        for (auto iterFields = luabind::iterator(*iter); iterFields != luabind::iterator(); ++iterFields)
        {
          String field;
          void* fieldBuffer = &field;
          if (!TypeManager::GetType<String>()->ConvertFromLua(iterFields.key(), fieldBuffer))
          {
            Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Invalid field in customization table" << "\n" << LuaManager::StackDump(iState);
            continue;
          }
          Type const* fieldType = nullptr;
          uint32_t offset;
          if (type->ResolveFieldPath(field, offset, fieldType))
          {
            DynObject newCustomization;
            newCustomization.SetType(fieldType, fieldType->Alloc(), true);
            luabind::detail::stack_pop pop(iState, 1);
            luabind::object obj(*iterFields);
            obj.push(iState);
            uint32_t top = lua_gettop(iState);
            fieldType->ConvertFromLua_Uninit(iState, top, newCustomization.GetBuffer());

            auto propTable = data.m_PropertyCustomization.insert(std::make_pair(PropertySheetName(prop), CustomizationData::FieldsMap())).first;
            propTable->second.insert(std::make_pair(field, std::move(newCustomization)));

          }
          else
          {
            Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << "Unknown field "<< field <<" in customization table for " << prop << "\n" << LuaManager::StackDump(iState);
            continue;
          }
        }
      }
    }

    lua_pop(iState, 3);

    sys->InstantiateArchetype(obj, archetype, &data);
    
    return 0;
  }

  struct LuaGameDataIter
  {
    GameDatabase::IterRange m_Range;
  };

  struct LuaGameDataConstIter
  {
    GameDatabase::ConstIterRange m_Range;
  };

  DECLARE_ENGINE_TYPE(LuaGameDataIter);
  DECLARE_ENGINE_TYPE(LuaGameDataConstIter);

  IMPLEMENT_TAG_TYPE(LuaGameDataIter);
  IMPLEMENT_TAG_TYPE(LuaGameDataConstIter);

  namespace
  {
    int IterNext(lua_State* iState)
    {
      int idx = lua_upvalueindex(1);
      
      luabind::default_converter<LuaGameDataIter*> converter;
      if (converter.match(iState, luabind::by_pointer<LuaGameDataIter>(), idx) < 0)
      {
        lua_pushliteral(iState, "Incorrect argument for game data iterator");
        Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
        return lua_error(iState);
      }

      LuaGameDataIter* iter = converter.to_cpp(iState, luabind::by_pointer<LuaGameDataIter>(), idx);
      if (iter->m_Range.first == iter->m_Range.second)
      {
        lua_pushnil(iState);
        return 1;
      }

      auto entry = *iter->m_Range.first;
      
      if (!entry.first.IsAssigned()
        || !entry.second.IsValid())
      {
        lua_pushnil(iState);
        return 1;
      }

      luabind::object arg(iState, entry.first);
      arg.push(iState);
      LuaManager::PushRefToLua(LuaManager::GetCurrentState().GetState(), 
        entry.second.GetType(), (void*)entry.second.GetBuffer(), false);
      ++iter->m_Range.first;
      return 2;
    }

    int IterNextConst(lua_State* iState)
    {
      int idx = lua_upvalueindex(1);

      luabind::default_converter<LuaGameDataConstIter*> converter;
      if (converter.match(iState, luabind::by_pointer<LuaGameDataConstIter>(), idx) < 0)
      {
        lua_pushliteral(iState, "Incorrect argument for game data iterator");
        Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
        return lua_error(iState);
      }

      LuaGameDataConstIter* iter = converter.to_cpp(iState, luabind::by_pointer<LuaGameDataConstIter>(), idx);
      if (iter->m_Range.first == iter->m_Range.second)
      {
        lua_pushnil(iState);
        return 1;
      }

      auto entry = *iter->m_Range.first;

      if (!entry.first.IsAssigned()
        || !entry.second.IsValid())
      {
        lua_pushnil(iState);
        return 1;
      }

      luabind::object arg(iState, entry.first);
      arg.push(iState);
      LuaManager::PushRefToLua(LuaManager::GetCurrentState().GetState(),
        entry.second.GetType(), (void*)entry.second.GetBuffer(), true);
      ++iter->m_Range.first;
      return 2;
    }

    int GameDatabaseIter(lua_State* iState)
    {
      //luabind::default_converter<GameDatabase*> converterSys;
      //if (converterSys.match(iState, luabind::by_pointer<GameDatabase>(), -2) < 0)
      //{
      //  lua_pushliteral(iState, "Incorrect argument for archetype system");
      //  Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      //  return lua_error(iState);
      //}
      const char* propNameStr = lua_tostring( iState, lua_upvalueindex( 1 ) );
      PropertySheetName prop( propNameStr );
      
      //luabind::default_converter<PropertySheetName> converterProp;
      //if (converterProp.match(iState, luabind::by_value<PropertySheetName>(), -1) < 0)
      //{
      //  lua_pushliteral(iState, "Incorrect argument for property sheet name");
      //  Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      //  return lua_error(iState);
      //}

      //GameDatabase* sys = converterSys.to_cpp(iState, luabind::by_pointer<GameDatabase>(), -2);
      World* world = LuaScriptSystem::GetWorld_Static();
      if (world == nullptr)
      {
        lua_pushliteral(iState, "Calling lua functions outside of the script system");
        Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
        return lua_error(iState);
      }
      GameDatabase* sys = world->GetSystem<GameDatabase>();
      //PropertySheetName prop = converterProp.to_cpp(iState, luabind::by_value<PropertySheetName>(), -1);

      LuaGameDataIter ret;
      ret.m_Range = sys->IterateOverData(PropertySheetName(prop));

      luabind::object arg(iState, ret);
      arg.push(iState);

      lua_pushcclosure(iState, &IterNext, 1);

      return 1;
    }

    int GameDatabaseConstIter(lua_State* iState)
    {
      //luabind::default_converter<GameDatabase*> converterSys;
      //if (converterSys.match(iState, luabind::by_pointer<GameDatabase>(), -2) < 0)
      //{
      //  lua_pushliteral(iState, "Incorrect argument for archetype system");
      //  Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      //  return lua_error(iState);
      //}

      //luabind::default_converter<PropertySheetName> converterProp;
      //if (converterProp.match(iState, luabind::by_value<PropertySheetName>(), -1) < 0)
      //{
      //  lua_pushliteral(iState, "Incorrect argument for property sheet name");
      //  Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
      //  return lua_error(iState);
      //}
      const char* propNameStr = lua_tostring(iState, lua_upvalueindex(1));
      PropertySheetName prop(propNameStr);

      //GameDatabase* sys = converterSys.to_cpp(iState, luabind::by_pointer<GameDatabase>(), -2);
      World* world = LuaScriptSystem::GetWorld_Static();
      if (world == nullptr)
      {
        lua_pushliteral(iState, "Calling lua functions outside of the script system");
        Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(iState);
        return lua_error(iState);
      }
      GameDatabase* sys = world->GetSystem<GameDatabase>();
      //PropertySheetName prop = converterProp.to_cpp(iState, luabind::by_value<PropertySheetName>(), -1);

      LuaGameDataConstIter ret;
      ret.m_Range = sys->IterateOverDataConst(PropertySheetName(prop));

      luabind::object arg(iState, ret);
      arg.push(iState);

      lua_pushcclosure(iState, &IterNextConst, 1);

      return 1;
    }

    struct access_property_gnr_const
    {
      access_property_gnr_const(const PropertySheetName& iName)
        : m_Name(iName)
      {}

      luabind::object operator()(luabind::argument const& iObject) const
      {
        World* world = eXl::LuaScriptSystem::GetWorld_Static();
        if (world != nullptr)
        {
          lua_State* state = iObject.interpreter();
          GameDatabase& db = *world->GetSystem<GameDatabase>();
          ObjectHandle obj;
          luabind::default_converter<ObjectHandle> converterObject;
          luabind::detail::stack_pop pop(state, 1);
          iObject.push(state);
          if (converterObject.match(state, luabind::by_value<ObjectHandle>(), -1) < 0)
          {
            lua_pushliteral(state, "Incorrect argument for property sheet access");
            Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(state);
            lua_error(state);
          }
          else
          {
            ObjectHandle obj = converterObject.to_cpp(state, luabind::by_value<ObjectHandle>(), -1);
            ConstDynObject prop = db.GetData(obj, m_Name);

            return LuaManager::GetLuaRef(state, prop);
          }
        }

        return luabind::object();
      }

      const PropertySheetName m_Name;
    };

    struct access_property_gnr
    {
      access_property_gnr(const PropertySheetName& iName)
        : m_Name(iName)
      {}

      luabind::object operator()(luabind::argument const& iObject) const
      {
        World* world = eXl::LuaScriptSystem::GetWorld_Static();
        if (world != nullptr)
        {
          lua_State* state = iObject.interpreter();
          GameDatabase& db = *world->GetSystem<GameDatabase>();
          ObjectHandle obj;
          luabind::default_converter<ObjectHandle> converterObject;
          luabind::detail::stack_pop pop(state, 1);
          iObject.push(state);
          if (converterObject.match(state, luabind::by_value<ObjectHandle>(), -1) < 0)
          {
            lua_pushliteral(state, "Incorrect argument for property sheet access");
            Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(state);
            lua_error(state);
          }
          else
          {
            ObjectHandle obj = converterObject.to_cpp(state, luabind::by_value<ObjectHandle>(), -1);
            DynObject prop = db.ModifyData(obj, m_Name);

            return LuaManager::GetLuaRef(state, prop);
          }
        }

        return luabind::object();
      }

      void operator()(luabind::argument const& iObject, luabind::object const& value) const 
      {
        World* world = eXl::LuaScriptSystem::GetWorld_Static();
        if (world != nullptr) 
        {
          lua_State* state = iObject.interpreter();
          GameDatabase& db = *world->GetSystem<GameDatabase>();
          ObjectHandle obj;
          luabind::default_converter<ObjectHandle> converterObject;
          luabind::detail::stack_pop(state, 1);
          iObject.push(state);
          if (converterObject.match(state, luabind::by_value<ObjectHandle>(), -1) < 0)
          {
            lua_pushliteral(state, "Incorrect argument for property sheet access");
            Log_Manager::Log(CoreLog::LUA_ERR_STREAM) << LuaManager::StackDump(state);
            lua_error(state);
          }
          else
          {
            ObjectHandle obj = converterObject.to_cpp(state, luabind::by_value<ObjectHandle>(), -1);
            DynObject dstProp = db.ModifyData(obj, m_Name);

            DynObject srcProp = LuaManager::GetObjectRef(value, dstProp.GetType());
            if (!srcProp.IsValid())
            {
              lua_pushliteral(state, "Invalid field for assignment");
              lua_error(state);
            }

            dstProp.GetType()->Assign(srcProp.GetType(), srcProp.GetBuffer(), dstProp.GetBuffer());
          }
        }

      }

      const PropertySheetName m_Name;
    };
  }

  LUA_REG_FUN(BindEngine)
  {
    luabind::module(iState, "eXl")[
      luabind::class_<ObjectHandle>()
        .def(luabind::constructor<>())
        .def(luabind::self == ObjectHandle()),

        luabind::class_<Transforms>()
        //.def("AddTransform", &Transforms::AddTransform)
        .def("GetLocalTransform", &Transforms::GetLocalTransform)
        .def("GetWorldTransform", &Transforms::GetWorldTransform)
        .def("HasTransform", &Transforms::HasTransform)
        .def("UpdateTransform", &Transforms::UpdateTransform)
        .def("Attach", &Transforms::Attach)
        .def("Detach", &Transforms::Detach),

        luabind::namespace_("GfxSpriteComponent")[
          luabind::def("SetDesc", &GfxSpriteComponent::SetDesc),
          luabind::def("SetOffset", &GfxSpriteComponent::SetOffset),
          luabind::def("SetSize", &GfxSpriteComponent::SetSize),
          luabind::def("SetTileset", &GfxSpriteComponent::SetTileset),
          luabind::def("SetTileName", &GfxSpriteComponent::SetTileName),
          luabind::def("SetAnimationSpeed", &GfxSpriteComponent::SetAnimationSpeed),
          luabind::def("SetRotateSprite", &GfxSpriteComponent::SetRotateSprite),
          luabind::def("SetLayer", &GfxSpriteComponent::SetLayer),
          luabind::def("SetTint", &GfxSpriteComponent::SetTint),
          luabind::def("SetFlat", &GfxSpriteComponent::SetFlat)
        ],

        luabind::class_<Archetype>(),

        luabind::class_<GfxSystem>()
        .def("CreateSpriteComponent", &GfxSystem::CreateSpriteComponent),
        //.def("GetSpriteComponent", &GfxSystem::GetSpriteComponent),

        luabind::class_<CharacterSystem>()
        .def("GetCurrentFacingDirection", &CharacterSystem::GetCurrentFacingDirection)
        .def("GetCurrentState", &CharacterSystem::GetCurrentState)
        .def("SetCurDir", &CharacterSystem::SetCurDir)
        .def("SetSpeed", &CharacterSystem::SetSpeed),

        luabind::class_<World>()
        .def("CreateObject", (ObjectHandle (World::*)())&World::CreateObject)
        .def("DeleteObject", &World::DeleteObject)
        .def("GetTransforms", &World::GetSystem<Transforms>)
        .def("GetGfxSystem", &World::GetSystem<GfxSystem>)
        .def("GetCharactersSystem", &World::GetSystem<CharacterSystem>),

        luabind::class_<CoroutineAPI>()
        .def("Pause", &CoroutineAPI::Pause)
        .def("Stop", &CoroutineAPI::Stop)
        .def("Yield", &CoroutineAPI::Yield),

        luabind::class_<LuaGameDataIter>(),

        luabind::class_<LuaGameDataConstIter>(),

        luabind::class_<GameDatabase>(),

        luabind::def("GetWorld", &LuaScriptSystem::GetWorld_Static)
    ];

    luabind::object _G = luabind::globals(iState);

    //lua_pushcfunction(iState, &ReadPropertyData);
    //luabind::object readPropFun(luabind::from_stack(iState, -1));
    //_G["eXl"]["ReadProperty"] = readPropFun;
    //lua_pop(iState, 1);
    //
    //lua_pushcfunction(iState, &AccessPropertyData);
    //luabind::object accessPropFun(luabind::from_stack(iState, -1));
    //_G["eXl"]["AccessProperty"] = accessPropFun;
    //lua_pop(iState, 1);

    lua_pushcfunction(iState, &LuaTriggerEvent);
    luabind::object dispatchEventFun(luabind::from_stack(iState, -1));
    _G["eXl"]["DispatchEvent"] = dispatchEventFun;
    lua_pop(iState, 1);

    _G["eXl"]["PropertySheetName"] = _G["eXl"]["Name"];

    lua_pushcfunction(iState, &InstantiateArchetype_Script);
    luabind::object instantiateArch(luabind::from_stack(iState, -1));
    _G["eXl"]["InstantiateArchetype"] = instantiateArch;
    lua_pop(iState, 1);

    //lua_pushcfunction(iState, &GameDatabaseIter);
    //luabind::object iterDbFun(luabind::from_stack(iState, -1));
    //_G["eXl"]["GameDataIterateMutable"] = iterDbFun;
    //lua_pop(iState, 1);
    //lua_pushcfunction(iState, &GameDatabaseConstIter);
    //luabind::object iterConstDbFun(luabind::from_stack(iState, -1));
    //_G["eXl"]["GameDataIterate"] = iterConstDbFun;
    //lua_pop(iState, 1);

    World* world = LuaScriptSystem::GetWorld_Static();
    {
      lua_createtable(iState, 0, world->GetConfig().m_Properties.GetProperties().size());
      luabind::object propTables(luabind::from_stack(iState, -1));
      _G["eXl"]["GameData"] = propTables;
      lua_pop(iState, 1);
    }
    luabind::object declNamespace = _G["eXl"]["GameData"];
    for (auto const& name : world->GetConfig().m_Properties.GetProperties())
    {
      if(Type const* propType = world->GetConfig().m_Properties.GetTypeFromName(name))
      {
        if (!propType->IsCoreType())
        {
          propType->RegisterLua(iState);
        }
          
        using get_signature = luabind::meta::type_list<luabind::object, luabind::argument const&>;
        luabind::object get_function = luabind::make_function(iState, access_property_gnr_const(name), get_signature(), luabind::no_policies());

        using access_signature = luabind::meta::type_list<luabind::object, luabind::argument const&>;
        luabind::object access_function = luabind::make_function(iState, access_property_gnr(name), access_signature(), luabind::no_policies());

        using set_signature = luabind::meta::type_list<void, luabind::argument const&, luabind::object>;
        luabind::object set_function = luabind::make_function(iState, access_property_gnr(name), set_signature(), luabind::no_policies());

        lua_pushstring(iState, name.c_str());
        lua_pushcclosure(iState, &GameDatabaseIter, 1);
        luabind::object iterDbFun(luabind::from_stack(iState, -1));
        lua_pop(iState, 1);
        
        lua_pushstring(iState, name.c_str());
        lua_pushcclosure(iState, &GameDatabaseConstIter, 1);
        luabind::object iterConstDbFun(luabind::from_stack(iState, -1));
        lua_pop(iState, 1);
        
        {
          lua_createtable(iState, 0, 5);
          luabind::object classTableUtils(luabind::from_stack(iState, -1));
          lua_pop(iState, 1);
          classTableUtils["Get"] = get_function;
          classTableUtils["Set"] = set_function;
          classTableUtils["Modify"] = access_function;
          classTableUtils["Iter"] = iterConstDbFun;
          classTableUtils["IterModify"] = iterDbFun;

          declNamespace[name.c_str()] = classTableUtils;
        }
          
      }
    }

    return 0;
  }
}

#endif