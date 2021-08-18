/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_FUNCTION_CONVERTER_HPP_INCLUDED
#define LUABIND_FUNCTION_CONVERTER_HPP_INCLUDED
#include <functional>
#include <luabind/detail/deduce_signature.hpp>
#include <luabind/detail/conversion_policies/conversion_base.hpp>
#include <luabind/make_function.hpp>
#include <luabind/detail/call_function.hpp>
#include <luabind/detail/type_traits.hpp>

namespace luabind {

	template <typename R = object>
	struct function
	{
		using result_type = R;

		function(luabind::object const& obj)
			: m_func(obj)
		{
		}

		template< typename... Args>
		R operator() (Args&&... args)
		{
			return call_function<R>(m_func, std::forward<Args>(args)...);
		}

	private:
		object m_func;
	};

	namespace detail {

		template< typename T >
		struct is_function : public std::false_type {};

		template< typename T >
		struct is_function< std::function< T > > : public std::true_type {};

		//template< typename R, typename... Args, typename WrappedType >
		//struct deduce_signature <std::function< R(Args...) >, WrappedType >
		//{
		//	using type = meta::type_list< R, Args... >;
		//};

	}


	template <typename F>
	struct default_converter<F, typename std::enable_if<detail::is_function<remove_const_reference_t<F>>::value>::type>
	{
		using is_native = std::true_type;

		enum { consumed_args = 1 };

		template <class U>
		void converter_postcall(lua_State*, U const&, int)
		{}

		template <class U>
		static int match(lua_State* L, U, int index)
		{
			if(lua_type(L, index) == LUA_TFUNCTION)
				return 0;
			if(luaL_getmetafield(L, index, "__call")) {
				lua_pop(L, 1);
				return 1;
			}
			return no_match;
		}

		template <class U>
		F to_cpp(lua_State* L, U, int index)
		{
			// If you get a compiler error here, you are probably trying to
			// get a function pointer from Lua. This is not supported:
			// you must use a type which is constructible from a
			// luabind::function, e.g. std::function or boost::function.
			return function<typename F::result_type>(object(from_stack(L, index)));
		}

		void to_lua(lua_State* L, F value)
		{
			make_function(L, value).push(L);
		}
	};

} // namespace luabind


#endif // LUABIND_FUNCTION_CONVERTER_HPP_INCLUDED

