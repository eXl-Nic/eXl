/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// call_traits: defines typedefs for function usage
// (see libs/utility/call_traits.htm)

/* Release notes:
23rd July 2000:
Fixed array specialization. (JM)
Added Borland specific fixes for reference types
(issue raised by Steve Cleary).
*/

#ifndef LUABIND_CALL_TRAITS_HPP_INCLUDED
#define LUABIND_CALL_TRAITS_HPP_INCLUDED

namespace luabind {
	namespace detail {

		template <typename T, bool small_>
		struct ct_imp2
		{
			using param_type = const T&;
		};

		template <typename T>
		struct ct_imp2<T, true>
		{
			using param_type = const T;
		};

		template <typename T, bool isp, bool b1, bool b2>
		struct ct_imp
		{
			using param_type = const T&;
		};

		template <typename T, bool isp, bool b2>
		struct ct_imp<T, isp, true, b2>
		{
			using param_type = typename ct_imp2<T, sizeof(T) <= sizeof(void*)>::param_type;
		};

		template <typename T, bool isp, bool b1>
		struct ct_imp<T, isp, b1, true>
		{
			using param_type = typename ct_imp2<T, sizeof(T) <= sizeof(void*)>::param_type;
		};

		template <typename T, bool b1, bool b2>
		struct ct_imp<T, true, b1, b2>
		{
			using param_type = const T;
		};

		template <typename T>
		struct call_traits
		{
		public:
			using value_type      = T;
			using reference       = T&;
			using const_reference = const T&;

			using param_type = typename ct_imp<
				T,
				std::is_pointer<T>::value,
				std::is_integral<T>::value || std::is_floating_point<T>::value,
				std::is_enum<T>::value
			>::param_type;
		};

		template <typename T>
		struct call_traits<T&>
		{
			using value_type      = T&;
			using reference       = T&;
			using const_reference = const T&;
			using param_type      = T&;
		};

		template <typename T, std::size_t N>
		struct call_traits<T[N]>
		{
		private:
			using array_type = T[N];
		public:
			using value_type      = const T*;
			using reference       = array_type&;
			using const_reference = const array_type&;
			using param_type      = const T* const;
		};

		template <typename T, std::size_t N>
		struct call_traits<const T[N]>
		{
		private:
			using array_type = const T[N];
		public:
			using value_type      = const T*;
			using reference       = array_type&;
			using const_reference = const array_type&;
			using param_type      = const T* const;
		};
	}
}

#endif

