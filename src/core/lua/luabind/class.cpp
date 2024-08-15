// Copyright (c) 2004 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LUABIND_BUILDING
#define LUABIND_BUILDING
#endif

#include <luabind/lua_include.hpp>

#include <luabind/config.hpp>
#include <luabind/class.hpp>
#include <luabind/nil.hpp>
#include <luabind/detail/debug.hpp>

#include <core/type/type.hpp>

#include <cstring>
#include <iostream>

namespace eXl
{
  EXL_CORE_API void OnTheFlyRegisterType(lua_State* L, Type const* iType);
}

namespace luabind {
	namespace detail {


		namespace
		{
			struct cast_entry
			{
				cast_entry(class_id src, class_id target, cast_function cast)
					: src(src)
					, target(target)
					, cast(cast)
				{}

				class_id src;
				class_id target;
				cast_function cast;
			};

		} // namespace unnamed

		struct class_registration : registration
		{
			class_registration(/*char const* name*/);

			void register_(lua_State* L) const;

			mutable std::string m_name;

			mutable std::map<const char*, int, detail::ltstr> m_static_constants;

			using base_desc = std::pair<type_id, cast_function>;
			mutable std::vector<base_desc> m_bases;

			type_id  m_type;
			class_id m_id;
			class_id m_wrapper_id;
			type_id  m_wrapper_type;
			std::vector<cast_entry> m_casts;

      scope m_extScope;
			scope m_scope;
			scope m_members;
			scope m_default_members;
		};

		class_registration::class_registration(/*char const* name*/)
		{
			//m_name = name;
		}

		void class_registration::register_(lua_State* L) const
		{
			LUABIND_CHECK_STACK(L);

			assert(lua_type(L, -1) == LUA_TTABLE);
      std::string name = m_type.get_id()->GetName();
      size_t searchStart = 0;
      while ((searchStart = name.find(':', searchStart)) != std::string::npos)
      {
        name[searchStart] = '_';
      }
      m_name = name;
			lua_pushstring(L, m_name.c_str());

			detail::class_rep* crep;

			detail::class_registry* r = detail::class_registry::get_registry(L);
			// create a class_rep structure for this class.
			// allocate it within lua to let lua collect it on
			// lua_close(). This is better than allocating it
			// as a static, since it will then be destructed
			// when the program exits instead.
			// warning: we assume that lua will not
			// move the userdata memory.
			lua_newuserdata(L, sizeof(detail::class_rep));
			crep = reinterpret_cast<detail::class_rep*>(lua_touserdata(L, -1));

			new(crep) detail::class_rep(
				m_type
				, m_name.c_str()
				, L
			);

			// register this new type in the class registry
			r->add_class(m_type, crep);

			lua_pushstring(L, "__luabind_class_map");
			lua_rawget(L, LUA_REGISTRYINDEX);
			class_map& classes = *static_cast<class_map*>(
				lua_touserdata(L, -1));
			lua_pop(L, 1);

			classes.put(m_id, crep);

			bool const has_wrapper = m_wrapper_id != registered_class<null_type>::get_id();

			if(has_wrapper)
				classes.put(m_wrapper_id, crep);

			crep->m_static_constants.swap(m_static_constants);

			detail::class_registry* registry = detail::class_registry::get_registry(L);

			crep->get_default_table(L);
			m_scope.register_(L);
			m_default_members.register_(L);
			lua_pop(L, 1);

			crep->get_table(L);
			m_members.register_(L);
			lua_pop(L, 1);

			lua_pushstring(L, "__luabind_cast_graph");
			lua_gettable(L, LUA_REGISTRYINDEX);
			cast_graph* const casts = static_cast<cast_graph*>(lua_touserdata(L, -1));
			lua_pop(L, 1);

			lua_pushstring(L, "__luabind_class_id_map");
			lua_gettable(L, LUA_REGISTRYINDEX);
			class_id_map* const class_ids = static_cast<class_id_map*>(lua_touserdata(L, -1));
			lua_pop(L, 1);

			class_ids->put(m_id, m_type);

			if(has_wrapper) {
				class_ids->put(m_wrapper_id, m_wrapper_type);
			}

			for(auto const& e : m_casts) {
				casts->insert(e.src, e.target, e.cast);
			}

			for(const auto& base_pair : m_bases) {
				LUABIND_CHECK_STACK(L);

				// the baseclass' class_rep structure
				detail::class_rep* bcrep = registry->find_class(base_pair.first);

				detail::class_rep::base_info base;
				base.pointer_offset = 0;
				base.base = bcrep;

				crep->add_base_class(base);

				// copy base class table
				crep->get_table(L);
				bcrep->get_table(L);
				lua_pushnil(L);

				while(lua_next(L, -2))
				{
					lua_pushvalue(L, -2); // copy key
					lua_gettable(L, -5);

					if(!lua_isnil(L, -1))
					{
						lua_pop(L, 2);
						continue;
					}

					lua_pop(L, 1);
					lua_pushvalue(L, -2); // copy key
					lua_insert(L, -2);

					lua_settable(L, -5);
				}
				lua_pop(L, 2);

				// copy base class detaults table
				crep->get_default_table(L);
				bcrep->get_default_table(L);
				lua_pushnil(L);

				while(lua_next(L, -2))
				{
					lua_pushvalue(L, -2); // copy key
					lua_gettable(L, -5);

					if(!lua_isnil(L, -1))
					{
						lua_pop(L, 2);
						continue;
					}

					lua_pop(L, 1);
					lua_pushvalue(L, -2); // copy key
					lua_insert(L, -2);

					lua_settable(L, -5);
				}
				lua_pop(L, 2);

			}

      object classRep(luabind::from_stack(L, -1));
			lua_settable(L, -3);
      object _G = luabind::globals(L);
      auto const& scopedNames = m_type.get_id()->GetNames();
      if (scopedNames.size() > 1 ) 
      {
        int t = lua_gettop(L);
        eXl::String const& moduleName = scopedNames[0];
        _G.push(L);
        {
          int type = lua_type(L, -1);
          eXl_ASSERT(type == LUA_TTABLE );
        }
        for (uint32_t i = 0; i < scopedNames.size() - 1; ++i)
        {
          eXl_ASSERT(lua_type(L, -1) == LUA_TTABLE);
          lua_pushstring(L, scopedNames[i].c_str());
          int type = lua_gettable( L, -2 );
          if (type == LUA_TNIL)
          {
            lua_pop(L, 1);
            lua_pushstring(L, scopedNames[i].c_str());
            lua_newtable(L);
            lua_settable( L, -3 );
            lua_pushstring(L, scopedNames[i].c_str());
            lua_gettable(L, -2);
          }
          else
          {
            eXl_ASSERT(type == LUA_TTABLE);
          }
        }
        lua_pushstring(L, scopedNames.back().c_str());
        classRep.push(L);
        lua_settable(L, -3);
        lua_pop(L, static_cast<int>(scopedNames.size()));
        eXl_ASSERT(t == lua_gettop(L));
      }
		}

		// -- interface ---------------------------------------------------------

		class_base::class_base(/*char const* name*/)
			: scope(std::unique_ptr<registration>(
				m_registration = new class_registration(/*name*/))
			)
		{
		}

		void class_base::init(
			type_id const& type_id_, class_id id
			, type_id const& wrapper_type, class_id wrapper_id)
		{
			m_registration->m_type = type_id_;
			m_registration->m_id = id;
			m_registration->m_wrapper_type = wrapper_type;
			m_registration->m_wrapper_id = wrapper_id;
		}

		void class_base::add_base(type_id const& base, cast_function cast)
		{
			m_registration->m_bases.push_back(std::make_pair(base, cast));
		}

		void class_base::add_member(registration* member)
		{
			std::unique_ptr<registration> ptr(member);
			m_registration->m_members.operator,(scope(std::move(ptr)));
		}

		void class_base::add_default_member(registration* member)
		{
			std::unique_ptr<registration> ptr(member);
			m_registration->m_default_members.operator,(scope(std::move(ptr)));
		}

    void class_base::add_ext(registration* member)
    {
      std::unique_ptr<registration> ptr(member);
      m_registration->m_extScope.operator,(scope(std::move(ptr)));
    }

		const char* class_base::name() const
		{
			return m_registration->m_name.c_str();
		}

    type_id class_base::get_type() const
    {
      return m_registration->m_type;
    }

		void class_base::add_static_constant(const char* name, int val)
		{
			m_registration->m_static_constants[name] = val;
		}

		void class_base::add_inner_scope(scope& s)
		{
			m_registration->m_scope.operator,(s);
		}

		void class_base::add_cast(
			class_id src, class_id target, cast_function cast)
		{
			m_registration->m_casts.push_back(cast_entry(src, target, cast));
		}

		void add_custom_name(type_id const& i, std::string& s)
		{
			s += " [";
			s += i.name();
			s += "]";
		}

		std::string get_class_name(lua_State* L, type_id const& i)
		{
			std::string ret;

			assert(L);

      eXl::KString nativeType = i.get_id()->GetLuaNativeType();
      if (!nativeType.empty())
      {
        ret = nativeType;
        return ret;
      }

			class_registry* r = class_registry::get_registry(L);
			class_rep* crep = r->find_class(i);

      if (crep == nullptr)
      {
        eXl::OnTheFlyRegisterType(L, i.get_id());
        crep = r->find_class(i);
      }

			if(crep == 0)
			{
        ret = "custom";
        add_custom_name(i, ret);
			} else
			{
				/* TODO reimplement this?
				if (i == crep->holder_type())
				{
					ret += "smart_ptr<";
					ret += crep->name();
					ret += ">";
				}
				else if (i == crep->const_holder_type())
				{
					ret += "smart_ptr<const ";
					ret += crep->name();
					ret += ">";
				}
				else*/
				{
					ret += crep->name();
				}
			}
			return ret;
		}

	}
} // namespace luabind::detail

