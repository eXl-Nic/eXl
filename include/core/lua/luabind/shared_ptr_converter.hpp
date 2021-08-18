/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_SHARED_PTR_CONVERTER_090211_HPP
# define LUABIND_SHARED_PTR_CONVERTER_090211_HPP

#include <luabind/detail/policy.hpp>
#include <luabind/detail/conversion_policies/value_converter.hpp>    // for default_converter, etc
#include <luabind/get_main_thread.hpp>  // for get_main_thread
#include <luabind/handle.hpp>           // for handle
#include <luabind/detail/decorate_type.hpp>  // for decorated_type
#include <memory>

#ifndef _LIBCPP_NO_RTTI

namespace luabind {

	namespace detail
	{

		struct shared_ptr_deleter
		{
			shared_ptr_deleter(lua_State* L, int index)
				: life_support(get_main_thread(L), L, index)
			{}

			void operator()(void const*)
			{
				handle().swap(life_support);
			}

			handle life_support;
		};

	} // namespace detail

	template <class T>
	struct default_converter<std::shared_ptr<T> >
		: default_converter<T*>
	{
		using is_native = std::false_type;

		template <class U>
		int match(lua_State* L, U, int index)
		{
			return default_converter<T*>::match(L, decorate_type_t<T*>(), index);
		}

		template <class U>
		std::shared_ptr<T> to_cpp(lua_State* L, U, int index)
		{
			T* raw_ptr = default_converter<T*>::to_cpp(L, decorate_type_t<T*>(), index);

			if(!raw_ptr) {
				return std::shared_ptr<T>();
			} else {
				return std::shared_ptr<T>(raw_ptr, detail::shared_ptr_deleter(L, index));
			}
		}

		void to_lua(lua_State* L, std::shared_ptr<T> const& p)
		{
			if(detail::shared_ptr_deleter* d = std::get_deleter<detail::shared_ptr_deleter>(p))
			{
				d->life_support.push(L);
			} else {
				detail::value_converter().to_lua(L, p);
			}
		}

		template <class U>
		void converter_postcall(lua_State*, U const&, int)
		{}
	};

	template <class T>
	struct default_converter<std::shared_ptr<T> const&>
		: default_converter<std::shared_ptr<T> >
	{};

} // namespace luabind

#endif // LUABIND_SHARED_PTR_CONVERTER_090211_HPP

#endif