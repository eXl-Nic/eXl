/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/resourcehandletype.hpp>
#include <core/type/typemanager.hpp>
#include <core/resource/resource.hpp>
#ifdef EXL_LUA
#include <core/lua/luaconverter.hpp>
#include <core/lua/luamanager.hpp>
#include <core/lua/luabind/detail/instance_holder.hpp>
#endif

namespace eXl
{
  IMPLEMENT_RTTI(ResourceHandleType);

  ResourceHandleType::ResourceHandleType()
    :CoreType("eXl::ResourceHandleType",
          0,
          sizeof(ResourceHandle<Resource>),
          Type_Is_CoreType)
    ,m_Rtti(Resource::StaticRtti())
  {
    //m_Flags |= Dynamic;
  }

  ResourceHandleType::ResourceHandleType(Rtti const& iRtti, Type const* iResourceType)
    : CoreType(TypeName(AString("eXl::ResourceHandle_For") + iRtti.GetName().data()),
          0,
      sizeof(ResourceHandle<Resource>),
          Type_Is_CoreType)
    , m_Rtti(iRtti)
    , m_ResourceType(iResourceType)
  {
    //m_Flags |= Dynamic;
  }
  
  void* ResourceHandleType::Alloc()const
  {
    return TypeTraits::Alloc<ResourceHandle<Resource>>();
  }

  void ResourceHandleType::Free(void* iObj)const
  {
    TypeTraits::Free<ResourceHandle<Resource>>(iObj);
  }

  void* ResourceHandleType::Construct(void* iObj)const
  {
    void* obj = TypeTraits::DefaultCTor<ResourceHandle<Resource>>(iObj);
    
    return obj;
  }
  
  void ResourceHandleType::Destruct(void* iObj)const
  {
    TypeTraits::DTor<ResourceHandle<Resource>>(iObj);
  }
  
  Err ResourceHandleType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer)const
  {
    Resource::UUID ID;
    Err err = UnstreamResourceHandle(ID, m_Rtti, *iUnstreamer);

    if (err)
    {
      TypeTraits::DefaultCTor<ResourceHandle<Resource>>(oData);
      reinterpret_cast<ResourceHandle<Resource>*>(oData)->SetUUID(ID);
    }

    return err;
  }
  
  Err ResourceHandleType::Stream(void const* iData, Streamer* iStreamer)const
  {
    ResourceHandle<Resource> const* handle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    
    return StreamResourceHandle(handle->GetUUID(), m_Rtti, *iStreamer);
  }

  Resource::UUID const& ResourceHandleType::GetUUID(void const* iData) const
  {
    static Resource::UUID s_Dummy;
    eXl_ASSERT_REPAIR_RET(iData != nullptr, s_Dummy);

    ResourceHandle<Resource> const* handle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    return handle->GetUUID();
  }

  void ResourceHandleType::SetUUID(void* iData, Resource::UUID const& iUUID) const
  {
    eXl_ASSERT_REPAIR_RET(iData != nullptr, );

    ResourceHandle<Resource>* handle = reinterpret_cast<ResourceHandle<Resource>*>(iData);
    handle->SetUUID(iUUID);
  }

  Err ResourceHandleType::Copy_Uninit(void const* iData, void* oData) const
  {
    if (iData == nullptr || oData == nullptr)
    {
      RETURN_FAILURE;
    }
    ResourceHandle<Resource> const* handleOrig = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
    ResourceHandle<Resource>* handleDest = reinterpret_cast<ResourceHandle<Resource>*>(oData);
    new (handleDest) ResourceHandle<Resource>(*handleOrig);
     
    RETURN_SUCCESS;
  }
#ifdef EXL_LUA
  
  luabind::object ResourceHandleType::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    //if(iState==nullptr)
      return luabind::object();
    //return eXl::LuaConverter<RttiOPtr>::ConvertToLua(iObj,this,iState);
  }
  
  Err ResourceHandleType::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    //if(iState==nullptr)
      RETURN_FAILURE;
    //if(oObj==nullptr)
    //  RETURN_FAILURE;
    //
    //eXl::LuaConverter<RttiOPtr>::ConvertFromLua(this,oObj,iState,ioIndex);
    //
    //RETURN_SUCCESS;
  }
#if 0
  int array_iter::Iterate(lua_State* iState)
  {
    int idx = lua_upvalueindex(1);

    luabind::default_converter<LuaArrayIterator*> converter;
    if (converter.match(iState, luabind::by_pointer<LuaArrayIterator>(), idx) < 0)
    {
      lua_pushliteral(iState, "Incorrect argument for game data iterator");
      return lua_error(iState);
    }

    LuaArrayIterator* iter = converter.to_cpp(iState, luabind::by_pointer<LuaArrayIterator>(), idx);
    if (iter->m_Cur >= iter->m_Type->GetArraySize(iter->m_Data))
    {
      lua_pushnil(iState);
      return 1;
    }

    LuaManager::PushRefToLua(iState
      , iter->m_Type->GetElementType()
      , iter->m_Type->GetElement(iter->m_Data, iter->m_Cur)
      , iter->m_IsConst);
    ++iter->m_Cur;
    return 1;
  }

  luabind::object array_iter::operator()(luabind::argument const& self_) const
  {
    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
    std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_Type));
    if (res.first == nullptr)
    {
      lua_pushliteral(self_.interpreter(), "Incorrect argument for array length");
      lua_error(self_.interpreter());
    }

    LuaArrayIterator iterator;
    iterator.m_Data = res.first;
    iterator.m_IsConst = self->is_const();
    iterator.m_Type = m_Type;

    luabind::object iterObj(self_.interpreter(), iterator);
    iterObj.push(self_.interpreter());

    lua_pushcclosure(self_.interpreter(), &Iterate, 1);

    return luabind::object(luabind::from_stack(self_.interpreter(), -1));
  }

  void array_iter_registration::register_(lua_State* iState) const
  {
    using signature_type = luabind::meta::type_list<luabind::object, luabind::argument const&>;
    luabind::object fn = luabind::make_function(iState, array_iter(m_Type), signature_type(), luabind::no_policies());
    luabind::detail::add_overload(luabind::object(luabind::from_stack(iState, -1)), "Elements", fn);
  }
#endif

  struct EXL_CORE_API resourcehandle_registration : luabind::detail::registration
  {
    resourcehandle_registration(ResourceHandleType const* iHolder)
      : m_HandleType(iHolder)

    {}

    static int SetHandleRsc(lua_State* iState)
    {
      int idx = lua_upvalueindex(1);

      luabind::default_converter<Type const*> converter;
      if (converter.match(iState, luabind::by_const_pointer<Type>(), idx) < 0)
      {
        lua_pushliteral(iState, "Incorrect argument for handle setter");
        return lua_error(iState);
      }
      ResourceHandleType const* handleType = ResourceHandleType::DynamicCast(converter.to_cpp(iState, luabind::by_const_pointer<Type>(), idx));

      luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(luabind::object(luabind::from_stack(iState, -2)));
      std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(handleType));
      if (res.first == nullptr)
      {
        lua_pushliteral(iState, "Incorrect argument for handle setter");
        lua_error(iState);
      }

      luabind::default_converter<Resource const*> rscConverter;
      if (rscConverter.match(iState, luabind::by_const_pointer<Resource>(), -1) < 0)
      {
        lua_pushliteral(iState, "Incorrect argument for handle setter");
        return lua_error(iState);
      }

      lua_pop(iState, 2);

      ResourceHandle<Resource>* handle = reinterpret_cast<ResourceHandle<Resource>*>(res.first);
      Resource const* rsc = rscConverter.to_cpp(iState, luabind::by_const_pointer<Resource>(), -1);

      if (rsc == nullptr)
      {
        handle->Set(nullptr);
      }
      else if (rsc->GetRtti().IsKindOf(handleType->GetRtti()))
      {
        handle->Set(rsc);
      }
      else
      {
        lua_pushfstring(iState, "Incorrect resource type %s for handle of type %s", rsc->GetHeader().m_LoaderName.c_str(), handleType->GetRtti().GetName().data());
        return lua_error(iState);
      }
      return 0;
    }

    static int GetOrLoadHandleRsc(lua_State* iState)
    {
      int idx = lua_upvalueindex(1);

      luabind::default_converter<Type const*> converter;
      if (converter.match(iState, luabind::by_const_pointer<Type>(), idx) < 0)
      {
        lua_pushliteral(iState, "Incorrect argument for handle setter");
        return lua_error(iState);
      }
      ResourceHandleType const* handleType = ResourceHandleType::DynamicCast(converter.to_cpp(iState, luabind::by_const_pointer<Type>(), idx));

      luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(luabind::object(luabind::from_stack(iState, -1)));
      std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(handleType));
      if (res.first == nullptr)
      {
        lua_pushliteral(iState, "Incorrect argument for handle setter");
        lua_error(iState);
      }

      ResourceHandle<Resource>* handle = reinterpret_cast<ResourceHandle<Resource>*>(res.first);
      Resource const* rsc = handle->GetOrLoad();

      lua_pop(iState, 1);

      if (rsc == nullptr)
      {
        lua_pushnil(iState);
        return 1;
      }
      else
      {
        //luabind::detail::class_rep* cls = LuaManager::GetClassRepFromType(iState, handleType->GetResourceType());
        //luabind::detail::object_rep* instance = luabind::detail::push_new_instance(iState, cls);
        //
        //void* storage = instance->allocate(sizeof(luabind::detail::pointer_holder<Resource*>));
        //luabind::detail::pointer_holder<Resource const*>* holder = new (storage) luabind::detail::pointer_holder<Resource const*>(rsc, clsId, (void*)rsc);
        //
        //instance->set_instance(holder);
        LuaManager::PushRefToLua(iState, handleType->GetResourceType(), rsc);
        eXl_ASSERT(!lua_isnil(iState, -1));

        return 1;
      }
    }

    void register_(lua_State* iState) const
    {
      luabind::object context(luabind::from_stack(iState, -1));
      {
        luabind::detail::stack_pop pop(iState, 1);
        luabind::object typeObj(iState, m_HandleType);
        typeObj.push(iState);

        lua_pushcclosure(iState, &SetHandleRsc, 1);

        context["Set"] = luabind::object(luabind::from_stack(iState, -1));
      }
      {
        luabind::detail::stack_pop pop(iState, 1);
        luabind::object typeObj(iState, m_HandleType);
        typeObj.push(iState);

        lua_pushcclosure(iState, &GetOrLoadHandleRsc, 1);

        context["GetOrLoad"] = luabind::object(luabind::from_stack(iState, -1));
      }
    }

    Type const* m_HandleType;
  };

  void ResourceHandleType::RegisterLua(lua_State* iState) const
  {
    luabind::detail::class_base newClass;
    newClass.init(this, luabind::detail::allocate_class_id(this), nullptr, luabind::detail::allocate_class_id(nullptr));
    newClass.add_member(new type_constructor_registration(this));
    newClass.add_default_member(new type_constructor_registration(this));

    newClass.add_member(new resourcehandle_registration(this));

    RegisterScope(iState, newClass);
  }
#endif
  Err ResourceHandleType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    ResourceHandle<Resource> const* handle1 = reinterpret_cast<ResourceHandle<Resource> const*>(iVal1);
    ResourceHandle<Resource> const* handle2 = reinterpret_cast<ResourceHandle<Resource> const*>(iVal2);

    oRes = handle1->GetUUID() == handle2->GetUUID() ? CompEqual : CompDifferent;

    RETURN_SUCCESS;
  }

  bool ResourceHandleType::CanAssignFrom(Type const* iOtherType) const
  {
    if(iOtherType == this)
      return true;
    else
    {
      ResourceHandleType const* otherObject = ResourceHandleType::DynamicCast(iOtherType);
      if(otherObject)
      {
        if(otherObject->GetMinimalRtti().IsKindOf(m_Rtti))
        {
          return true;
        }
      }
      else
      {
        return false;
      }
    }
    return false;
  }

  Err ResourceHandleType::Assign_Uninit(Type const* inputType, void const* iData, void* oData) const
  {
    if(iData != nullptr && oData != nullptr && CanAssignFrom(inputType))
    {
      ResourceHandle<Resource> const* sourceHandle = reinterpret_cast<ResourceHandle<Resource> const*>(iData);
      ResourceHandle<Resource>* newHandle = new(oData) ResourceHandle<Resource>;
      newHandle->SetUUID(sourceHandle->GetUUID());
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }
}