/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/lua/luabind_eXl.hpp>
#include <core/lua/luamanager.hpp>

#include <core/type/arraytype.hpp>

namespace eXl
{
  dynobject_holder::dynobject_holder(DynObject iObject, bool iIsConst)
    : instance_holder(iIsConst)
    , m_Object(std::move(iObject))

  {
  }

  std::pair<void*, int> dynobject_holder::get(luabind::detail::cast_graph const& casts, luabind::detail::class_id target) const
  {
    DynObject const& refHolder = m_Object.IsValid() ? m_Object : m_Ref;
    if (!refHolder.IsValid())
    {
      return std::pair<void*, int>(nullptr, 0);
    }
    if (luabind::detail::allocate_class_id(refHolder.GetType()) == target)
    {
      return std::make_pair((void*)refHolder.GetBuffer(), 0);
    }
    return std::pair<void*, int>(nullptr, 0);
  }

  void type_constructor::operator()(luabind::argument const& self_) const
  {
    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);

    DynObject obj;
    obj.SetType(m_Type, m_Type->Build(), true);
    void* naked_ptr = obj.GetBuffer();
    luabind::detail::inject_backref(self_.interpreter(), naked_ptr, naked_ptr);

    void* storage = self->allocate(sizeof(dynobject_holder));

    self->set_instance(new (storage) dynobject_holder(std::move(obj), false));
  }

  void type_constructor_registration::register_(lua_State* iState) const
  {
    using signature_type = luabind::meta::type_list<void, luabind::argument const&>;
    luabind::object fn = luabind::make_function(iState, type_constructor(m_Type), signature_type(), luabind::no_policies());
    luabind::detail::add_overload(luabind::object(luabind::from_stack(iState, -1)), "__init", fn);
  }

  void type_field_registration::register_(lua_State* iState) const
  {
    luabind::object context(luabind::from_stack(iState, -1));
    TypeFieldName fieldName;
    Type const* fieldType = m_Type->GetFieldDetails(m_FieldIdx, fieldName);
    context[fieldName.c_str()] = fieldType->MakePropertyAccessor(iState, m_Type, m_FieldIdx);
  }

  luabind::object access_field_gnr::operator()(luabind::argument const& self_) const
  {
    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
    std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_FieldHolder));
    if (res.first == nullptr)
    {
      lua_pushliteral(self_.interpreter(), "Incorrect argument for field accessor");
      lua_error(self_.interpreter());
    }

    DynObject objRef;
    objRef.SetType(m_FieldHolder, res.first);

    DynObject fieldRef;
    
    if (!objRef.GetField(m_FieldIdx, fieldRef))
    {
      lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
      lua_error(self_.interpreter());
    }
    
    return LuaManager::GetLuaRef(self_.interpreter(), fieldRef);
  }

  void access_field_gnr::operator()(luabind::argument const& self_, luabind::object const& value) const
  {
    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
    std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_FieldHolder));
    if (res.first == nullptr)
    {
      lua_pushliteral(self_.interpreter(), "Incorrect argument for field accessor");
      lua_error(self_.interpreter());
    }

    DynObject objRef;
    objRef.SetType(m_FieldHolder, res.first);

    DynObject dstField;

    if (!objRef.GetField(m_FieldIdx, dstField))
    {
      lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
      lua_error(self_.interpreter());
    }

    DynObject srcField = LuaManager::GetObjectRef(value, dstField.GetType());
    if (!srcField.IsValid())
    {
      lua_pushliteral(self_.interpreter(), "Invalid field for assignment");
      lua_error(self_.interpreter());
    }

    dstField.GetType()->Assign(srcField.GetType(), srcField.GetBuffer(), dstField.GetBuffer());
  }

  void access_element_registration::register_(lua_State* iState) const
  {
    luabind::object context(luabind::from_stack(iState, -1));
    context["_eXl_AccessElement"] = m_SequenceType->GetElementType()->MakeElementAccessor(iState, m_SequenceType);
  }

  luabind::object access_element_gnr::operator()(luabind::argument const& self_, int iIndex) const
  {
    Type const* elementType = m_SequenceType->GetElementType();
    
    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
    std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_SequenceType));
    if (res.first == nullptr)
    {
      lua_pushliteral(self_.interpreter(), "Incorrect argument for element accessor");
      lua_error(self_.interpreter());
    }

    DynObject objRef;
    objRef.SetType(m_SequenceType, res.first);

    DynObject elementRef;
    if (!objRef.GetElement(iIndex, elementRef))
    {
      lua_pushliteral(self_.interpreter(), "Invalid for element accessor");
      lua_error(self_.interpreter());
    }

    return LuaManager::GetLuaRef(self_.interpreter(), elementRef);
  }

  void access_element_gnr::operator()(luabind::argument const& self_, int iIndex, luabind::object const& iValue) const
  {
    Type const* elementType = m_SequenceType->GetElementType();

    luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
    std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_SequenceType));
    if (res.first == nullptr)
    {
      lua_pushliteral(self_.interpreter(), "Incorrect argument for element accessor");
      lua_error(self_.interpreter());
    }

    DynObject objRef;
    objRef.SetType(m_SequenceType, res.first);

    DynObject elementRef;
    if (!objRef.GetElement(iIndex, elementRef))
    {
      lua_pushliteral(self_.interpreter(), "Invalid for element accessor");
      lua_error(self_.interpreter());
    }

    DynObject valueRef = LuaManager::GetObjectRef(iValue, m_SequenceType->GetElementType());

    if (!valueRef.IsValid())
    {
      lua_pushliteral(self_.interpreter(), "Invalid for element accessor");
      lua_error(self_.interpreter());
    }

    elementRef.GetType()->Assign(valueRef.GetType(), valueRef.GetBuffer(), elementRef.GetBuffer());
  }

}