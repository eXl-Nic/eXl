/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

#ifndef LUABIND_CLASS_INFO_HPP_INCLUDED
#define LUABIND_CLASS_INFO_HPP_INCLUDED

#include <luabind/prefix.hpp>
#include <luabind/lua_include.hpp>
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>
#include <core/type/typetraits.hpp>

namespace luabind
{
	struct LUABIND_API class_info
	{
		std::string name;
		object methods;
		object attributes;

    bool operator == (class_info const& iOther) const
    {
      return false;
    }
	};

	LUABIND_API class_info get_class_info(argument const&);

	// returns a table of bound class names
	LUABIND_API object get_class_names(lua_State* L);

	LUABIND_API void bind_class_info(lua_State*);
}

namespace eXl
{
  DEFINE_TYPE_EX(luabind::class_info, luabind__class_info, LUABIND_API)
  namespace TypeTraits
  {
    template <>
    inline Err Stream<luabind::class_info>(luabind::class_info const* iObj, Streamer& iStreamer)
    {
      return Err::Error;
    }

    template <>
    inline Err Unstream<luabind::class_info>(luabind::class_info* oObj, Unstreamer& iUnstreamer)
    {
      return Err::Error;
    }
  }
}

#endif
