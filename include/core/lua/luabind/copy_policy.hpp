/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_COPY_POLICY_081021_HPP
# define LUABIND_COPY_POLICY_081021_HPP

# include <luabind/detail/policy.hpp>

namespace luabind {
	namespace detail {

		struct copy_converter
		{
			template <class T>
			void to_lua(lua_State* L, T const& x)
			{
				value_converter().to_lua(L, x);
			}

			template <class T>
			void to_lua(lua_State* L, T* x)
			{
				if(!x)
					lua_pushnil(L);
				else
					to_lua(L, *x);
			}
		};

		struct copy_policy
		{
			template <class T, class Direction>
			struct specialize
			{
				static_assert(std::is_same<Direction, cpp_to_lua>::value, "Copy policy only supports cpp -> lua");
				using type = copy_converter;
			};
		};

	} // namespace detail

		// Caution: If we use the aliased type "policy_list" here, MSVC crashes.
	template< unsigned int N >
	using copy_policy = meta::type_list< converter_policy_injector< N, detail::copy_policy > >;

} // namespace luabind

#endif // LUABIND_COPY_POLICY_081021_HPP

