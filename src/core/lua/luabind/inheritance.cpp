/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LUABIND_BUILDING
#define LUABIND_BUILDING
#endif

#include <limits>
#include <map>
#include <vector>
#include <queue>
#include <vector>
#include <luabind/typeid.hpp>
#include <luabind/detail/inheritance.hpp>

namespace luabind {
	namespace detail {

		class_id const class_id_map::local_id_base = std::numeric_limits<class_id>::max() / 2;

		namespace
		{

			struct edge
			{
				edge(class_id target, cast_function cast)
					: target(target), cast(cast)
				{}

				class_id target;
				cast_function cast;
			};

			bool operator<(edge const& x, edge const& y)
			{
				return x.target < y.target;
			}

			struct vertex
			{
				vertex(class_id id)
					: id(id)
				{}

				class_id id;
				std::vector<edge> edges;
			};

			using cache_entry = std::pair<std::ptrdiff_t, int>;

			class cache
			{
			public:
				static constexpr std::ptrdiff_t unknown = std::numeric_limits<std::ptrdiff_t>::max();
				static constexpr std::ptrdiff_t invalid = unknown - 1;

				cache_entry get(class_id src, class_id target, class_id dynamic_id, std::ptrdiff_t object_offset) const;

				void put(class_id src, class_id target, class_id dynamic_id, std::ptrdiff_t object_offset, std::ptrdiff_t offset, int distance);
				void invalidate();

			private:
				using key_type = std::tuple<class_id, class_id, class_id, std::ptrdiff_t>;
				using map_type = std::map<key_type, cache_entry>;
				map_type m_cache;
			};

			constexpr std::ptrdiff_t cache::unknown;
			constexpr std::ptrdiff_t cache::invalid;

			cache_entry cache::get(class_id src, class_id target, class_id dynamic_id, std::ptrdiff_t object_offset) const
			{
				map_type::const_iterator i = m_cache.find(
					key_type(src, target, dynamic_id, object_offset));
				return i != m_cache.end() ? i->second : cache_entry(unknown, -1);
			}

			void cache::put(class_id src, class_id target, class_id dynamic_id, std::ptrdiff_t object_offset, std::ptrdiff_t offset, int distance)
			{
				m_cache.insert(std::make_pair(key_type(src, target, dynamic_id, object_offset), cache_entry(offset, distance)));
			}

			void cache::invalidate()
			{
				m_cache.clear();
			}

		} // namespace anonymous

		class cast_graph::impl
		{
		public:
			std::pair<void*, int> cast(
				void* p, class_id src, class_id target
				, class_id dynamic_id, void const* dynamic_ptr) const;
			void insert(class_id src, class_id target, cast_function cast);

		private:
			std::vector<vertex> m_vertices;
			mutable cache m_cache;
		};

		namespace
		{

			struct queue_entry
			{
				queue_entry(void* p, class_id vertex_id, int distance)
					: p(p)
					, vertex_id(vertex_id)
					, distance(distance)
				{}

				void* p;
				class_id vertex_id;
				int distance;
			};

		} // namespace anonymous

		std::pair<void*, int> cast_graph::impl::cast(void* const p, class_id src, class_id target, class_id dynamic_id, void const* dynamic_ptr) const
		{
			if(src == target) return std::make_pair(p, 0);
			if(src >= m_vertices.size() || target >= m_vertices.size()) return std::pair<void*, int>((void*)0, -1);

			std::ptrdiff_t const object_offset = (char const*)dynamic_ptr - (char const*)p;
			cache_entry cached = m_cache.get(src, target, dynamic_id, object_offset);

			if(cached.first != cache::unknown)
			{
				if(cached.first == cache::invalid)
					return std::pair<void*, int>((void*)0, -1);
				return std::make_pair((char*)p + cached.first, cached.second);
			}

			std::queue<queue_entry> q;
			q.push(queue_entry(p, src, 0));

			// Original source used boost::dynamic_bitset but didn't make use
			// of its advanced capability of set operations, that's why I think
			// it's safe to use a std::vector<bool> here.

			std::vector<bool> visited(m_vertices.size(), false);

			while(!q.empty())
			{
				queue_entry const qe = q.front();
				q.pop();

				visited[qe.vertex_id] = true;
				vertex const& v = m_vertices[qe.vertex_id];

				if(v.id == target)
				{
					m_cache.put(
						src, target, dynamic_id, object_offset
						, (char*)qe.p - (char*)p, qe.distance
					);

					return std::make_pair(qe.p, qe.distance);
				}

				for(auto const& e : v.edges) {
					if(visited[e.target])
						continue;
					if(void* casted = e.cast(qe.p))
						q.push(queue_entry(casted, e.target, qe.distance + 1));
				}
			}

			m_cache.put(src, target, dynamic_id, object_offset, cache::invalid, -1);

			return std::pair<void*, int>((void*)0, -1);
		}

		void cast_graph::impl::insert(class_id src, class_id target, cast_function cast)
		{
			class_id const max_id = std::max(src, target);

			if(max_id >= m_vertices.size())
			{
				m_vertices.reserve(max_id + 1);
				for(class_id i = m_vertices.size(); i < max_id + 1; ++i)
					m_vertices.push_back(vertex(i));
			}

			std::vector<edge>& edges = m_vertices[src].edges;

			std::vector<edge>::iterator i = std::lower_bound(
				edges.begin(), edges.end(), edge(target, 0)
			);

			if(i == edges.end() || i->target != target)
			{
				edges.insert(i, edge(target, cast));
				m_cache.invalidate();
			}
		}

		std::pair<void*, int> cast_graph::cast(void* p, class_id src, class_id target, class_id dynamic_id, void const* dynamic_ptr) const
		{
			return m_impl->cast(p, src, target, dynamic_id, dynamic_ptr);
		}

		void cast_graph::insert(class_id src, class_id target, cast_function cast)
		{
			m_impl->insert(src, target, cast);
		}

		cast_graph::cast_graph()
			: m_impl(new impl)
		{}

		cast_graph::~cast_graph()
		{}

    using class_map_type = std::map<type_id, class_id>;
    class_map_type& get_classes()
    {
      static class_map_type registered;
      return registered;
    }
    class_id& get_class_id()
    {
      static class_id id = 0;
      return id;
    }

    LUABIND_API void reset_class_id()
    {
      get_class_id() = 0;
      get_classes().clear();
    }

		LUABIND_API class_id allocate_class_id(type_id const& cls)
		{
			std::pair<class_map_type::iterator, bool> inserted = get_classes().insert(std::make_pair(cls, get_class_id()));
			if(inserted.second) ++get_class_id();

			return inserted.first->second;
		}

	} // namespace detail
} // namespace luabind

