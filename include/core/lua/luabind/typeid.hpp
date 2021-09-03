// Copyright Daniel Wallin 2008. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef LUABIND_TYPEID_081227_HPP
# define LUABIND_TYPEID_081227_HPP

//# include <typeinfo>
#include <luabind/config.hpp>
#include <core/type/typemanager_get.hpp>
# include <luabind/detail/type_traits.hpp>

namespace luabind {

	class type_id
	{
	public:
		type_id()
			: id(nullptr)
		{}

		type_id(eXl::Type const* iId)
			: id(iId)
		{}

		bool operator!=(type_id const& other) const
		{
			return id != other.id;
		}

		bool operator==(type_id const& other) const
		{
			return id == other.id;
		}

		bool operator<(type_id const& other) const
		{
			return id < other.id;
		}

		size_t hash_code() const
		{
			return /*id->hash_code()*/(ptrdiff_t)id;
		}

    LUABIND_API char const* name() const;

    eXl::Type const* get_id() const { return id; }

	private:
		//std::type_info const* id;
    eXl::Type const* id;
	};

} // namespace luabind

#endif // LUABIND_TYPEID_081227_HPP

