/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_BUILDING
#define LUABIND_BUILDING
#endif

#include <luabind/detail/object.hpp>
#include <luabind/make_function.hpp>
#include <luabind/detail/conversion_policies/conversion_policies.hpp>
#include <luabind/detail/object.hpp>

namespace luabind {
	namespace detail {

		namespace {

			int function_destroy(lua_State* L)
			{
				function_object* fn = *(function_object**)lua_touserdata(L, 1);
				delete fn;
				return 0;
			}

			void push_function_metatable(lua_State* L)
			{
				lua_pushstring(L, "luabind.function");
				lua_rawget(L, LUA_REGISTRYINDEX);

				if(lua_istable(L, -1))
					return;

				lua_pop(L, 1);

				lua_newtable(L);

				lua_pushstring(L, "__gc");
				lua_pushcclosure(L, &function_destroy, 0);
				lua_rawset(L, -3);

				lua_pushstring(L, "luabind.function");
				lua_pushvalue(L, -2);
				lua_rawset(L, LUA_REGISTRYINDEX);
			}

			// A pointer to this is used as a tag value to identify functions exported
			// by luabind.
			int function_tag = 0;

		} // namespace unnamed

		LUABIND_API bool is_luabind_function(lua_State* L, int index)
		{
			if(!lua_getupvalue(L, index, 2))
				return false;
			bool result = lua_touserdata(L, -1) == &function_tag;
			lua_pop(L, 1);
			return result;
		}

		namespace
		{

			inline bool is_luabind_function(object const& obj)
			{
				obj.push(obj.interpreter());
				bool result = detail::is_luabind_function(obj.interpreter(), -1);
				lua_pop(obj.interpreter(), 1);
				return result;
			}

		} // namespace unnamed

		LUABIND_API void add_overload(
			object const& context, char const* name, object const& fn)
		{
			function_object* f = *touserdata<function_object*>(std::get<1>(getupvalue(fn, 1)));
			f->name = name;

			if(object overloads = context[name])
			{
				if(is_luabind_function(overloads) && is_luabind_function(fn))
				{
					f->next = *touserdata<function_object*>(std::get<1>(getupvalue(overloads, 1)));
					f->keepalive = overloads;
				}
			}

			context[name] = fn;
		}

		LUABIND_API object make_function_aux(lua_State* L, function_object* impl)
		{
			void* storage = lua_newuserdata(L, sizeof(function_object*));
			push_function_metatable(L);
			*(function_object**)storage = impl;
			lua_setmetatable(L, -2);

			lua_pushlightuserdata(L, &function_tag);
			lua_pushcclosure(L, impl->entry, 2);
			stack_pop pop(L, 1);

			return object(from_stack(L, -1));
		}

		void invoke_context::format_error(
			lua_State* L, function_object const* overloads) const
		{
			char const* function_name =
				overloads->name.empty() ? "<unknown>" : overloads->name.c_str();

			if(candidate_index == 0)
			{
				int stacksize = lua_gettop(L);
				lua_pushstring(L, "No matching overload found, candidates:\n");
				int count = 0;
				for(function_object const* f = overloads; f != 0; f = f->next)
				{
					if(count != 0)
						lua_pushstring(L, "\n");
					f->format_signature(L, function_name);
					++count;
				}
				lua_concat(L, lua_gettop(L) - stacksize);
			}
			else
			{
				// Ambiguous
				int stacksize = lua_gettop(L);
				lua_pushstring(L, "Ambiguous, candidates:\n");
				for(int i = 0; i < candidate_index; ++i)
				{
					if(i != 0)
						lua_pushstring(L, "\n");
					candidates[i]->format_signature(L, function_name);
				}
				lua_concat(L, lua_gettop(L) - stacksize);
			}
		}

	} // namespace detail
} // namespace luabind

