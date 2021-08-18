/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_DETAIL_CONSTRUCTOR_081018_HPP
#define LUABIND_DETAIL_CONSTRUCTOR_081018_HPP

#include <luabind/get_main_thread.hpp>
#include <luabind/lua_argument_proxy.hpp>
#include <luabind/wrapper_base.hpp>
#include <luabind/detail/inheritance.hpp>

namespace luabind {
	namespace detail {

		inline void inject_backref(lua_State*, void*, void*)
		{}

		template <class T>
		void inject_backref(lua_State* L, T* p, wrap_base*)
		{
			weak_ref(get_main_thread(L), L, 1).swap(wrap_access::ref(*p));
		}

		template< class T, class Pointer, class Signature, class Arguments, class ArgumentIndices >
		struct construct_aux_helper;

		template< class T, class Pointer, class Signature, typename... Arguments, unsigned int... ArgumentIndices >
		struct construct_aux_helper< T, Pointer, Signature, meta::type_list< Arguments... >, meta::index_list< ArgumentIndices... > >
		{
			using holder_type = pointer_holder<Pointer, T>;

			void operator()(argument const& self_, Arguments... args) const
			{
				object_rep* self = touserdata<object_rep>(self_);

				std::unique_ptr<T> instance(new T(args...));
				inject_backref(self_.interpreter(), instance.get(), instance.get());

				void* naked_ptr = instance.get();
				Pointer ptr(instance.release());

				void* storage = self->allocate(sizeof(holder_type));

				self->set_instance(new (storage) holder_type(std::move(ptr), registered_class<T>::get_id(), naked_ptr));
			}
		};


		template< class T, class Pointer, class Signature >
		struct construct :
			public construct_aux_helper <
			T,
			Pointer,
			Signature, typename meta::sub_range< Signature, 2, meta::size<Signature>::value >::type,
			typename meta::make_index_range<0, meta::size<Signature>::value - 2>::type >
		{
		};

	}	// namespace detail

}	// namespace luabind

# endif // LUABIND_DETAIL_CONSTRUCTOR_081018_HPP

