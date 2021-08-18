/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_EXCEPTION_HANDLER_050601_HPP
#define LUABIND_EXCEPTION_HANDLER_050601_HPP

#include <luabind/config.hpp>           // for LUABIND_API
#include <type_traits>
#include <luabind/lua_include.hpp>
#include <luabind/detail/meta.hpp>

namespace luabind {

# ifndef LUABIND_NO_EXCEPTIONS

	namespace detail {

		struct LUABIND_API exception_handler_base
		{
			exception_handler_base()
				: next(0)
			{}

			virtual ~exception_handler_base() {}
			virtual void handle(lua_State*) const = 0;

			void try_next(lua_State*) const;

			exception_handler_base* next;
		};

		template<class E, class Handler>
		struct exception_handler : exception_handler_base
		{
			using argument = E const&;

			exception_handler(Handler handler)
				: handler(handler)
			{}

			void handle(lua_State* L) const
			{
				try
				{
					try_next(L);
				}
				catch(argument e)
				{
					handler(L, e);
				}
			}

			Handler handler;
		};

		LUABIND_API void handle_exception_aux(lua_State* L);
		LUABIND_API void register_exception_handler(exception_handler_base*);

	} // namespace detail

# endif

	template<class E, class Handler>
	void register_exception_handler(Handler handler, meta::type<E>* = 0)
	{
# ifndef LUABIND_NO_EXCEPTIONS
		detail::register_exception_handler(
			new detail::exception_handler<E, Handler>(handler)
		);
# endif
	}

} // namespace luabind

#endif // LUABIND_EXCEPTION_HANDLER_050601_HPP

