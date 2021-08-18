/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_FUNCTION2_081014_HPP
# define LUABIND_FUNCTION2_081014_HPP

# include <luabind/make_function.hpp>
# include <luabind/scope.hpp>
# include <luabind/detail/call_function.hpp>

namespace luabind {

	namespace detail
	{

		template <class F, class PolicyInjectors>
		struct function_registration : registration
		{
			function_registration(char const* name, F f)
				: name(name)
				, f(f)
			{}

			void register_(lua_State* L) const
			{
				object fn = make_function(L, f, PolicyInjectors());
				add_overload(object(from_stack(L, -1)), name, fn);
			}

			char const* name;
			F f;
		};

		LUABIND_API bool is_luabind_function(lua_State* L, int index);

	} // namespace detail

	template <class F, typename... PolicyInjectors>
	scope def(char const* name, F f, policy_list<PolicyInjectors...> const&)
	{
		return scope(std::unique_ptr<detail::registration>(
			new detail::function_registration<F, policy_list<PolicyInjectors...>>(name, f)));
	}

	template <class F>
	scope def(char const* name, F f)
	{
		return def(name, f, no_policies());
	}

} // namespace luabind

#endif // LUABIND_FUNCTION2_081014_HPP

