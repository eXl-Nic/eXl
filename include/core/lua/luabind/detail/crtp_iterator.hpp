/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_CRTP_ITERATOR_HPP_INCLUDED
#define LUABIND_CRTP_ITERATOR_HPP_INCLUDED

#include <iterator>

namespace luabind {
	namespace detail {

		template< typename CRTP, typename Category, typename ValueType, typename ReferenceType = ValueType&, typename DifferenceType = ptrdiff_t >
		class crtp_iterator /*:
			public std::iterator<Category, ValueType, DifferenceType, ValueType*, ReferenceType >*/
		{
		public:
			//using base_type = std::iterator<Category, ValueType, DifferenceType, ValueType*, ReferenceType >;

      using iterator_category = Category;
      using value_type = ValueType;
      using difference_type = DifferenceType;
      using pointer = ValueType*;
      using reference = ReferenceType;

			CRTP& operator++()
			{
				upcast().increment();
				return upcast();
			}

			CRTP operator++(int)
			{
				CRTP tmp(upcast());
				upcast().increment();
				return tmp;
			}

			bool operator==(const CRTP& rhs)
			{
				return upcast().equal(rhs);
			}

			bool operator!=(const CRTP& rhs)
			{
				return !upcast().equal(rhs);
			}

			typename reference operator*()
			{
				return upcast().dereference();
			}

			typename reference operator->()
			{
				return upcast().dereference();
			}

		private:
			CRTP& upcast() { return static_cast<CRTP&>(*this); }
			const CRTP& upcast() const { return static_cast<const CRTP&>(*this); }
		};

	}
}

#endif
