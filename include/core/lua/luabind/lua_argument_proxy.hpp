/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUA_ARGUMENT_PROXY_HPP_INCLUDED
#define LUA_ARGUMENT_PROXY_HPP_INCLUDED

#include <luabind/lua_include.hpp>
#include <luabind/lua_proxy_interface.hpp>
#include <luabind/lua_index_proxy.hpp>
#include <luabind/from_stack.hpp>

namespace luabind {

	namespace adl {

		class argument : public lua_proxy_interface<argument>
		{
		public:
			argument(from_stack const& stack_reference)
				: m_interpreter(stack_reference.interpreter), m_index(stack_reference.index)
			{
				if(m_index < 0) m_index = lua_gettop(m_interpreter) - m_index + 1;
			}

			template<class T>
			index_proxy<argument> operator[](T const& key) const
			{
				return index_proxy<argument>(*this, m_interpreter, key);
			}

			void push(lua_State* L) const
			{
				lua_pushvalue(L, m_index);
			}

			lua_State* interpreter() const
			{
				return m_interpreter;
			}

		private:
			lua_State* m_interpreter;
			int m_index;
		};

	}

	using adl::argument;

	template<>
	struct lua_proxy_traits<argument>
	{
		using is_specialized = std::true_type;

		static lua_State* interpreter(argument const& value)
		{
			return value.interpreter();
		}

		static void unwrap(lua_State* interpreter, argument const& value)
		{
			value.push(interpreter);
		}

		static bool check(...)
		{
			return true;
		}
	};

}

#endif

