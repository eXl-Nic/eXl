/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_DETAIL_MAKE_INSTANCE_090310_HPP
# define LUABIND_DETAIL_MAKE_INSTANCE_090310_HPP

# include <luabind/detail/inheritance.hpp>
# include <luabind/detail/object_rep.hpp>

#include <core/type/typemanager_get.hpp>

namespace luabind {
	namespace detail {

		template <class T>
		std::pair<class_id, void*> get_dynamic_class_aux(lua_State* L, T const* p, std::true_type)
		{
			lua_pushliteral(L, "__luabind_class_id_map");
			lua_rawget(L, LUA_REGISTRYINDEX);
			class_id_map& class_ids = *static_cast<class_id_map*>(lua_touserdata(L, -1));
			lua_pop(L, 1);

			return std::make_pair(class_ids.get_local(eXl::TypeManager::GetType<T>()), reinterpret_cast<void*>(const_cast<T*>(p)));
		}

		template <class T>
		std::pair<class_id, void*> get_dynamic_class_aux(lua_State*, T const* p, std::false_type)
		{
			return std::make_pair(registered_class<T>::get_id(), (void*)p);
		}

		template <class T>
		std::pair<class_id, void*> get_dynamic_class(lua_State* L, T* p)
		{
			return get_dynamic_class_aux(L, p, std::is_polymorphic<T>());
		}

		template <class T>
		class_rep* get_pointee_class(class_map const& classes, T*)
		{
			return classes.get(registered_class<T>::get_id());
		}

		template <class P>
		class_rep* get_pointee_class(lua_State* L, P const& p, class_id dynamic_id)
		{
			lua_pushliteral(L, "__luabind_class_map");
			lua_rawget(L, LUA_REGISTRYINDEX);

			class_map const& classes = *static_cast<class_map*>(lua_touserdata(L, -1));

			lua_pop(L, 1);

			class_rep* cls = classes.get(dynamic_id);

			if(!cls) {
				cls = get_pointee_class(classes, get_pointer(p));
			}

			return cls;
		}

		// Create an appropriate instance holder for the given pointer like object.
		template <class P>
		void make_pointer_instance(lua_State* L, P p)
		{
			std::pair<class_id, void*> dynamic = get_dynamic_class(L, get_pointer(p));

			class_rep* cls = get_pointee_class(L, p, dynamic.first);

			if(!cls)
			{
				throw std::runtime_error("Trying to use unregistered class: " + std::string(type_id(eXl::TypeManager::GetType<P>()).name()));
			}

			object_rep* instance = push_new_instance(L, cls);

			using value_type = typename std::remove_reference<P>::type;
			using holder_type = pointer_holder<value_type>;

			void* storage = instance->allocate(sizeof(holder_type));

			try
			{
				new (storage) holder_type(std::move(p), dynamic.first, dynamic.second);
			}
			catch(...)
			{
				instance->deallocate(storage);
				lua_pop(L, 1);
				throw;
			}

			instance->set_instance(static_cast<holder_type*>(storage));
		}


		template< typename ValueType >
		void make_value_instance(lua_State* L, ValueType&& val, std::true_type /* is smart ptr */)
		{
			if(get_pointer(val)) {
				std::pair<class_id, void*> dynamic = get_dynamic_class(L, get_pointer(val));
				class_rep* cls = get_pointee_class(L, val, dynamic.first);

				using pointee_type = decltype(*get_pointer(val));

				if(!cls) {
					throw std::runtime_error("Trying to use unregistered class: " + std::string(type_id(eXl::TypeManager::GetType<pointee_type>()).name()));
				}

				object_rep* instance = push_new_instance(L, cls);

				using value_type = typename std::remove_reference<ValueType>::type;
				using holder_type = pointer_like_holder<value_type>;

				void* storage = instance->allocate(sizeof(holder_type));

				try {
					new (storage) holder_type(L, std::forward<ValueType>(val), dynamic.first, dynamic.second);
				}
				catch(...) {
					instance->deallocate(storage);
					lua_pop(L, 1);
					throw;
				}

				instance->set_instance(static_cast<holder_type*>(storage));
			} else {
				lua_pushnil(L);
			}
		}

		template< typename ValueType >
		void make_value_instance(lua_State* L, ValueType&& val, std::false_type /* smart ptr */)
		{
			const auto value_type_id = detail::registered_class<ValueType>::get_id();
			class_rep* cls = get_pointee_class(L, &val, value_type_id);

			if(!cls) {
				throw std::runtime_error("Trying to use unregistered class: " + std::string(type_id(eXl::TypeManager::GetType<ValueType>()).name()));
			}

			object_rep* instance = push_new_instance(L, cls);

			using value_type = typename std::remove_reference<ValueType>::type;
			using holder_type = value_holder<value_type>;

			void* storage = instance->allocate(sizeof(holder_type));

			try {
				new (storage) holder_type(L, std::forward<ValueType>(val));
			}
			catch(...) {
				instance->deallocate(storage);
				lua_pop(L, 1);
				throw;
			}

			instance->set_instance(static_cast<holder_type*>(storage));
		}

		template< typename ValueType >
		void make_value_instance(lua_State* L, ValueType&& val)
		{
			make_value_instance(L, std::forward<ValueType>(val), has_get_pointer<ValueType>());
		}

	} // namespace detail
} // namespace luabind

#endif // LUABIND_DETAIL_MAKE_INSTANCE_090310_HPP

