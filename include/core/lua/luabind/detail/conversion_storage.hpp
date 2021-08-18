/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_CONVERSION_STORAGE_080930_HPP
# define LUABIND_CONVERSION_STORAGE_080930_HPP

# include <luabind/config.hpp>
# include <type_traits>

namespace luabind {
	namespace detail {

		using destruction_function = void(*)(void*);

		// This is used by the converters in policy.hpp, and
		// class_rep::convert_to as temporary storage when constructing
		// holders.

		struct conversion_storage
		{
			conversion_storage()
				: destructor(0)
			{}

			~conversion_storage()
			{
				if(destructor)
					destructor(&data);
			}

			// Unfortunately the converters currently doesn't have access to
			// the actual type being converted when this is instantiated, so
			// we have to guess a max size.
			std::aligned_storage<128> data;
			destruction_function destructor;
		};

	}
} // namespace luabind::detail

#endif // LUABIND_CONVERSION_STORAGE_080930_HPP

