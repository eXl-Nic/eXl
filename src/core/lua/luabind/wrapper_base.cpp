// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LUABIND_BUILDING
#define LUABIND_BUILDING
#endif

#include <luabind/config.hpp>
#include <luabind/lua_include.hpp>
#include <luabind/function.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/stack_utils.hpp>

#include <core/rtti.hpp>

namespace luabind {

  eXl::Rtti const& wrap_base::StaticRtti()
  {
    static eXl::Rtti s_Rtti("wrap_base", &eXl::RttiObject::StaticRtti());
    return s_Rtti;
  }

  eXl::Type const* wrap_base::GetType()
  {
    static eXl::ClassType s_Class("wrap_base", StaticRtti(), eXl::ClassType::DynamicCast(eXl::RttiObject::GetType()));

    return &s_Class;
  }

  const eXl::Rtti& wrap_base::GetRtti() const
  {
    return StaticRtti();
  }

  wrap_base* wrap_base::DynamicCast(eXl::RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<wrap_base*>(ptr);
    }
    return nullptr;
  }

  const wrap_base* wrap_base::DynamicCast(const eXl::RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<wrap_base const*>(ptr);
    }
    return nullptr;
  }

	namespace detail {

		LUABIND_API void do_call_member_selection(lua_State* L, char const* name)
		{
			object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, -1));
			assert(obj);

			lua_pushstring(L, name);
			lua_gettable(L, -2);
			lua_replace(L, -2);

			if(!is_luabind_function(L, -1))
				return;

			// this (usually) means the function has not been
			// overridden by lua, call the default implementation
			lua_pop(L, 1);
			obj->crep()->get_default_table(L); // push the crep table
			lua_pushstring(L, name);
			lua_gettable(L, -2);
			lua_remove(L, -2); // remove the crep table
		}
	}
}

