/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "allocator.hpp"

#include <set>
#include <map>
#include <list>
#include <vector>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/container/small_vector.hpp>

namespace boost
{
  namespace container
  {
#undef EXL_ALLOCATOR_INCLUDED
#define EXL_NAMESPACE_ALLOC_INJECTION
#include "allocator.hpp"
#undef EXL_NAMESPACE_ALLOC_INJECTION
  }
}

namespace eXl
{
  template <typename T1, typename T2>
  using Pair = std::pair<T1, T2>;

  template <class T>
  using List = std::list<T, Allocator<T> >;

  template <class T>
  using Vector = std::vector<T, Allocator<T> >;

  template <class T, class Cmp = std::less<T> >
  using Set = std::set<T, Cmp, Allocator<T> >;

  template <class T, class Cmp = std::less<T>>
  using Multiset = std::multiset<T, Cmp, Allocator<T> >;

  template <class Key, class Value, class Cmp = std::less<Key> >
  using Map = std::map<Key, Value, Cmp, Allocator<std::pair<const Key, Value> > >;

  template <class Key, class Value, class Cmp = std::less<Key> >
  using Multimap = std::multimap<Key, Value, Cmp, Allocator<std::pair<const Key, Value> > >;

  template <class Key, class Value, class Hash = boost::hash<Key>, class Pred = std::equal_to<Key>>
  using UnorderedMap = boost::unordered_map<Key, Value, Hash, Pred, Allocator<std::pair<const Key, Value> > >;

  template <class Key, class Value, class Hash = boost::hash<Key>, class Pred = std::equal_to<Key>>
  using UnorderedMultimap = boost::unordered_multimap<Key, Value, Hash, Pred, Allocator<std::pair<const Key, Value> > >;

  template <typename Value, typename Hash = boost::hash<Value>>
  using UnorderedSet = boost::unordered::unordered_set<Value, Hash, std::equal_to<Value>, Allocator<Value>>;

  template <typename Value, size_t N>
  using SmallVector = boost::container::small_vector<Value, N, boost::container::eXlAllocator<Value>>;
  
}
