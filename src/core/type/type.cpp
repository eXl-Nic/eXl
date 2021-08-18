/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/type.hpp>
#include <core/rtti.hpp>

#ifdef EXL_LUA
#include <luabind/luabind.hpp>
#include <core/lua/luabind_eXl.hpp>
#endif

namespace eXl{

  Rtti const& Type::StaticRtti()
  {
    static Rtti s_Rtti("Type", &RttiObject::StaticRtti());
    return s_Rtti;
  }

  Type const* Type::GetType()
  {
    static ClassType s_Class("Type", StaticRtti(), ClassType::DynamicCast(RttiObject::GetType()));

    return &s_Class;
  }

  const Rtti& Type::GetRtti() const
  {
    return StaticRtti();
  }

  Type* Type::DynamicCast(RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<Type*>(ptr);
    }
    return nullptr;
  }

  const Type* Type::DynamicCast(const RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<Type const*>(ptr);
    }
    return nullptr;
  }

  Type::Type(TypeName iName,size_t iTypeId,size_t iSize,unsigned int iFlags)
    : m_Name(iName)
    , m_TypeId(iTypeId)
    , m_Size(iSize)/*,
    m_Flags(iFlags)*/
  {
    m_Flags |= iFlags;
  }

  void* Type::Alloc()const
  {
    eXl_ASSERT((m_Flags & Type_Is_TagType) == 0);
    return eXl_ALLOC(m_Size);
  }

  void Type::Free(void* iObj)const
  {
    eXl_ASSERT((m_Flags & Type_Is_TagType) == 0);
    eXl_FREE(iObj);
  }

  Err Type::Copy(void const* iData, void*& oData)const
  {
    if(iData != nullptr)
    {
      if(oData != nullptr)
      {
        Destruct(oData);
      }
      else
      {
        oData = Alloc();
      }
      if(oData != nullptr)
      {
        return Copy_Uninit(iData,oData);
      }
    }
    RETURN_FAILURE;
  }

  Err Type::Unstream(void*& oData, Unstreamer* iUnstreamer) const
  {
    if(iUnstreamer != nullptr)
    {
      if(oData != nullptr)
      {
        Destruct(oData);
      }
      else
      {
        oData = Alloc();
      }
      if(oData != nullptr)
      {
        return Unstream_Uninit(oData,iUnstreamer);
      }
    }
    RETURN_FAILURE;
  }
#ifdef EXL_LUA
  Err Type::ConvertFromLua(luabind::object iObj,void*& oData)const
  {
    Err err = Err::Failure;
    lua_State* iState = iObj.interpreter();
    if(iState != nullptr)
    {
      iObj.push(iState);
      unsigned int idx = lua_gettop(iState);
      //DynObject* res = ConvertFromLua(iState,idx,nullptr);
      if(oData != nullptr)
      {
        Destruct(oData);
      }
      else
      {
        oData = Alloc();
      }
      if(oData != nullptr)
      {
        err = ConvertFromLua_Uninit(iState,idx,oData);
        lua_pop(iState,1);
        return err;
      }
    }
    RETURN_FAILURE;
  }
#endif
  bool Type::CanAssignFrom(Type const* iOtherType) const
  {
    if(this == iOtherType
    || (/*(m_TypeId & StorageFlag) &&*/ (iOtherType->m_TypeId == m_TypeId)))
    {
      return true;
    }
    else
      return false;
  }

  Err Type::Assign(Type const* inputType, void const* iData, void* oData) const
  {
    if(iData != nullptr && oData != nullptr && CanAssignFrom(inputType))
    {
      Destruct(oData);
      return Assign_Uninit(inputType,iData,oData);
    }
    RETURN_FAILURE;
  }

  Err Type::Assign_Uninit(Type const* inputType, void const* iData, void* oData) const
  {
    if(iData && oData != nullptr && CanAssignFrom(inputType))
    {
      return Copy_Uninit(iData,oData);
    }
    RETURN_FAILURE;
  }

  void* Type::Construct(void* iObj)const
  {
    eXl_ASSERT(false);
    return nullptr;
  }

  void Type::Destruct(void* iObj)const
  {
    eXl_ASSERT(false);
  }

  Err Type::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer)const
  {
    eXl_ASSERT(false);
    return Err::Error;
  }

  Err Type::Stream(void const* iData, Streamer* iStreamer)const
  {
    eXl_ASSERT(false);
    return Err::Error;
  }

  Err Type::Copy_Uninit(void const* iData, void* oData) const {
    eXl_ASSERT(false);
    return Err::Error;
  }

#ifdef EXL_LUA

  luabind::object Type::ConvertToLua(void const* iObj, lua_State* iState) const
  {
    eXl_ASSERT(false);
    return luabind::object();
  }

  Err Type::ConvertFromLua_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj)const
  {
    eXl_ASSERT(false);
    RETURN_FAILURE;
  }

  void Type::RegisterLua(lua_State* iState) const
  {
    eXl_ASSERT(false);
  }

  luabind::object Type::MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iField) const
  {
    using get_result_type = luabind::object;
    using get_signature = luabind::meta::type_list<get_result_type, luabind::argument const&>;
    using injected_list = typename luabind::detail::inject_dependency_policy< luabind::object, luabind::no_policies >::type;

    luabind::object get_function = luabind::make_function(iState, access_field_gnr(iHolder, iField), get_signature(), injected_list());

    using argument_type = luabind::object;
    using signature_type = luabind::meta::type_list<void, luabind::argument const&, argument_type>;

    luabind::object set_function = luabind::make_function(iState, access_field_gnr(iHolder, iField), signature_type(), luabind::no_policies());

    return luabind::property(get_function, set_function);
  }

  luabind::object Type::MakeElementAccessor(lua_State* iState, ArrayType const* iHolder) const
  {
    using get_result_type = luabind::object;
    using get_signature = luabind::meta::type_list<get_result_type, luabind::argument const&, int>;
    using injected_list = typename luabind::detail::inject_dependency_policy< luabind::object, luabind::no_policies >::type;

    luabind::object get_function = luabind::make_function(iState, access_element_gnr(iHolder), get_signature(), injected_list());

    using argument_type = luabind::object;
    using signature_type = luabind::meta::type_list<void, luabind::argument const&, int, argument_type>;

    luabind::object set_function = luabind::make_function(iState, access_element_gnr(iHolder), signature_type(), luabind::no_policies());

    return luabind::array_access(get_function, set_function);
  }

#endif
}

#include <luabind/typeid.hpp>

const char* luabind::type_id::name() const
{
  return id->GetName().c_str();
}