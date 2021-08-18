/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

