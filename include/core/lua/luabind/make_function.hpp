/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_MAKE_FUNCTION_081014_HPP
#define LUABIND_MAKE_FUNCTION_081014_HPP

#include <luabind/config.hpp>
#include <luabind/detail/object.hpp>
#include <luabind/detail/call.hpp>
#include <luabind/detail/deduce_signature.hpp>
#include <luabind/detail/format_signature.hpp>

namespace luabind {

	namespace detail
	{
# ifndef LUABIND_NO_EXCEPTIONS
		LUABIND_API void handle_exception_aux(lua_State* L);
# endif

		// MSVC complains about member being sensitive to alignment (C4121)
		// when F is a pointer to member of a class with virtual bases.
# ifdef _MSC_VER
#  pragma pack(push)
#  pragma pack(16)
# endif

		template <class F, class Signature, class InjectorList>
		struct function_object_impl : function_object
		{
			function_object_impl(F f)
				: function_object(&entry_point), f(f)
			{}

			int call(lua_State* L, invoke_context& ctx) /*const*/
			{
#ifndef LUABIND_NO_INTERNAL_TAG_ARGUMENTS
				return invoke(L, *this, ctx, f, Signature(), InjectorList());
#else
				return invoke<InjectorList, Signature>(L, *this, ctx, f);
#endif
			}

			void format_signature(lua_State* L, char const* function) const
			{
				detail::format_signature(L, function, Signature());
			}

			static bool invoke_defer(lua_State* L, function_object_impl* impl, invoke_context& ctx, int& results)
			{
				bool exception_caught = false;

				try {
#ifndef LUABIND_NO_INTERNAL_TAG_ARGUMENTS
					results = invoke(L, *impl, ctx, impl->f, Signature(), InjectorList());
#else
					results = invoke<InjectorList, Signature>(L, *impl, ctx, impl->f);
#endif
				}
				catch(...) {
					exception_caught = true;
					handle_exception_aux(L);
				}

				return exception_caught;
			}

			static int entry_point(lua_State* L)
			{
				function_object_impl const* impl_const = *(function_object_impl const**)lua_touserdata(L, lua_upvalueindex(1));

				// TODO: Can this be done differently?
				function_object_impl* impl = const_cast<function_object_impl*>(impl_const);
				invoke_context ctx;
				int results = 0;

# ifndef LUABIND_NO_EXCEPTIONS
				bool exception_caught = invoke_defer(L, impl, ctx, results);
				if(exception_caught) lua_error(L);
# else
#ifndef LUABIND_NO_INTERNAL_TAG_ARGUMENTS
				results = invoke(L, *impl, ctx, impl->f, Signature(), InjectorList());
#else
				results = invoke<InjectorList, Signature>(L, *impl, ctx, impl->f);
#endif
# endif
				if(!ctx) {
					ctx.format_error(L, impl);
					lua_error(L);
				}

				return results;
			}

			F f;
		};

# ifdef _MSC_VER
#  pragma pack(pop)
# endif

		LUABIND_API object make_function_aux(lua_State* L, function_object* impl);
		LUABIND_API void add_overload(object const&, char const*, object const&);

	} // namespace detail

	template <class F, typename... SignatureElements, typename... PolicyInjectors >
	object make_function(lua_State* L, F f, meta::type_list< SignatureElements... >, meta::type_list< PolicyInjectors... >)
	{
		return detail::make_function_aux(L, new detail::function_object_impl<F, meta::type_list< SignatureElements... >, meta::type_list< PolicyInjectors...> >(f));
	}

	template <class F, typename... PolicyInjectors >
	object make_function(lua_State* L, F f, meta::type_list< PolicyInjectors... >)
	{
		return make_function(L, f, deduce_signature_t<F>(), meta::type_list< PolicyInjectors... >());
	}

	template <class F>
	object make_function(lua_State* L, F f)
	{
		return make_function(L, f, deduce_signature_t<F>(), no_policies());
	}

} // namespace luabind

#endif // LUABIND_MAKE_FUNCTION_081014_HPP

