/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/dynobject.hpp>
#if EXL_LUA
#include <luabind/class.hpp>

namespace eXl
{
  class dynobject_holder : public luabind::detail::instance_holder
  {
  public:
    dynobject_holder(DynObject iObject, bool iIsConst);

    std::pair<void*, int> get(luabind::detail::cast_graph const& casts, luabind::detail::class_id target) const override;

    explicit operator bool() const
    {
      return m_Object.IsValid() ? true : false;
    }

    void release() override
    {
      m_Ref = m_Object.Ref();
      m_Object.Release();
    }

  private:
    DynObject m_Object;
    DynObject m_Ref;
  };

  struct type_constructor
  {
    type_constructor(Type const* iType)
      : m_Type(iType)
    {}

    Type const* m_Type;

    void operator()(luabind::argument const& self_) const;
  };

  struct type_constructor_registration : luabind::detail::registration
  {
    type_constructor_registration(Type const* iType)
      : m_Type(iType)
    {}

    void register_(lua_State* iState) const;

    Type const* m_Type;
  };

  struct EXL_CORE_API type_field_registration : luabind::detail::registration
  {
    type_field_registration(TupleType const* iType, uint32_t iFieldIdx)
      : m_Type(iType)
      , m_FieldIdx(iFieldIdx)
    {}

    void register_(lua_State* iState) const;

    TupleType const* m_Type;
    uint32_t m_FieldIdx;
  };

  struct access_field_gnr
  {
    access_field_gnr(Type const* iHolder, uint32_t iFieldIdx)
      : m_FieldHolder(iHolder)
      , m_FieldIdx(iFieldIdx)

    {}

    luabind::object operator()(luabind::argument const& self_) const;

    void operator()(luabind::argument const& self_, luabind::object const& value) const;

    Type const* m_FieldHolder;
    uint32_t m_FieldIdx;
  };

  struct access_element_gnr
  {
    access_element_gnr(ArrayType const* iHolder)
      : m_SequenceType(iHolder)

    {}

    luabind::object operator()(luabind::argument const& self_, int iIndex) const;
    void operator()(luabind::argument const& self_, int iIndex, luabind::object const& iObj) const;

    ArrayType const* m_SequenceType;
  };

  struct EXL_CORE_API access_element_registration : luabind::detail::registration
  {
    access_element_registration(ArrayType const* iHolder)
      : m_SequenceType(iHolder)

    {}

    void register_(lua_State* iState) const;

    ArrayType const* m_SequenceType;
  };

  template <typename T, typename Result_Type = T>
  struct access_field
  {
    access_field(Type const* iHolder, uint32_t iFieldIdx)
      : m_FieldHolder(iHolder)
      , m_FieldIdx(iFieldIdx)

    {}

    Result_Type operator()(luabind::argument const& self_) const
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
      T* fieldPtr = objRef.GetField<T>(m_FieldIdx);

      if (fieldPtr == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
        lua_error(self_.interpreter());
      }

      return *fieldPtr;
    }

    void operator()(luabind::argument const& self_, T const& value) const
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
      T* fieldPtr = objRef.GetField<T>(m_FieldIdx);

      if (fieldPtr == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
        lua_error(self_.interpreter());
      }

      *fieldPtr = value;
    }
    Type const* m_FieldHolder;
    uint32_t m_FieldIdx;
  };

  template <typename T, typename Result_Type = T>
  struct access_element
  {
    access_element(ArrayType const* iSequenceType)
      : m_SequenceType((Type const*)iSequenceType)

    {}

    Result_Type operator()(luabind::argument const& self_, int32_t element) const
    {
      luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
      std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_SequenceType));
      if (res.first == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Incorrect argument for field accessor");
        lua_error(self_.interpreter());
      }
      DynObject objRef;
      objRef.SetType(m_SequenceType, res.first);
      DynObject elementRef;
      objRef.GetElement(element, elementRef);
      T* elementPtr = elementRef.IsValid() ? elementRef.CastBuffer<T>() : nullptr;

      if (elementPtr == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
        lua_error(self_.interpreter());
      }

      return *elementPtr;
    }

    void operator()(luabind::argument const& self_, int32_t element, T const& value) const
    {
      luabind::detail::object_rep* self = luabind::touserdata<luabind::detail::object_rep>(self_);
      std::pair<void*, int> res = self->get_instance(luabind::detail::allocate_class_id(m_SequenceType));
      if (res.first == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Incorrect argument for field accessor");
        lua_error(self_.interpreter());
      }
      DynObject objRef;
      objRef.SetType(m_SequenceType, res.first);

      DynObject elementRef;
      objRef.GetElement(element, elementRef);
      T* elementPtr = elementRef.IsValid() ? elementRef.CastBuffer<T>() : nullptr;

      if (elementPtr == nullptr)
      {
        lua_pushliteral(self_.interpreter(), "Invalid for field accessor");
        lua_error(self_.interpreter());
      }

      *elementPtr = value;
    }
    Type const* m_SequenceType;
  };
}
#endif