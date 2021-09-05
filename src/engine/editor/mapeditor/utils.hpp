#pragma once

#include <math/aabb2d.hpp>
#include <math/geometrytraits.hpp>
#include <boost/geometry/index/rtree.hpp>

class QSpinBox;

namespace eXl
{
  template <typename Functor>
  void ConnectToValueChanged(QSpinBox* iSpinBox, Functor&& iFunctor)
  {
    QObject::connect(iSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [functor = std::move(iFunctor)](int value)
    {
      functor(value);
    });
  }

	using BoxIndexEntry = std::pair<AABB2Di, ObjectHandle>;
	using BoxIndex = boost::geometry::index::rtree<BoxIndexEntry, boost::geometry::index::rstar<16, 4>>;
	struct QueryResult
	{
		QueryResult(Vector<BoxIndexEntry>& outVec)
			: m_Result(outVec)
		{}

		Vector<BoxIndexEntry>::const_iterator begin() { return m_Result.begin(); }
		Vector<BoxIndexEntry>::const_iterator end() { return m_Result.end(); }
		bool empty() const { return m_Result.empty(); }
		size_t size() const { return m_Result.size(); }

		std::back_insert_iterator<Vector<BoxIndexEntry>> Inserter()
		{
			return std::back_inserter(m_Result);
		}

		~QueryResult()
		{
			m_Result.clear();
		}
		Vector<BoxIndexEntry>& m_Result;
	};

  Vector2i SafeGetTileSize(ResourceHandle<Tileset> const& iHandle, TileName iName);
  Vector2i SafeGetSizeFromArchetype(ResourceHandle<Archetype> const& iArchetype);
  Vector2i GetSizeFromArchetype(Archetype const* iArchetype);
}