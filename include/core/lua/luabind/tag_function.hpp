/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_TAG_FUNCTION_081129_HPP
#define LUABIND_TAG_FUNCTION_081129_HPP

#include <luabind/config.hpp>
#include <luabind/detail/type_traits.hpp>
#include <luabind/lua_state_fwd.hpp>

namespace luabind {

	template <class Signature, class F>
	struct tagged_function
	{
		tagged_function(F f)
			: f(f)
		{}

		F f;
	};

	namespace detail
	{

		struct invoke_context;
		struct function_object;

#ifndef LUABIND_NO_INTERNAL_TAG_ARGUMENTS
		template <class Signature, class F, typename... PolicyInjectors>
		int invoke(
			lua_State* L, function_object const& self, invoke_context& ctx
			// std::bind operator() is nonconst... is that fixable?
			, tagged_function<Signature, F> /*const*/& tagged
			, Signature, meta::type_list<PolicyInjectors...> const& injectors)
		{
			return invoke(L, self, ctx, tagged.f, Signature(), injectors);
		}
#else
		//template< typename PolicyList, typename Signature, typename F>
		//inline int invoke(lua_State* L, function_object const& self, invoke_context& ctx, F& f)

		template < typename PolicyList, typename Signature, typename F >
		int invoke(lua_State* L, function_object const& self, invoke_context& ctx, tagged_function<Signature, F> /*const*/& tagged)
		{
			return invoke<PolicyList, Signature>(L, self, ctx, tagged.f);
		}
#endif

	} // namespace detail

	template <class Signature, class F>
	tagged_function<deduce_signature_t<Signature>, F >
		tag_function(F f)
	{
		return f;
	}

} // namespace luabind

# endif // LUABIND_TAG_FUNCTION_081129_HPP

