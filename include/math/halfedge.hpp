/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <math/mathexp.hpp>
#include <core/containers.hpp>
#include <math/vector2.hpp>
#include <math/segment.hpp>
#include <boost/container/flat_map.hpp>

namespace eXl
{
  struct PolyVertex
  {
    PolyVertex(Vector2i const& iPos, Vector2f const& iPosf)
      : position(iPos)
      , positionf(iPosf)
    {
    }

    Vector2f positionf;
    Vector2i position;
    int32_t firstEdge = -1;
  };

  struct PolyHalfEdge
  {
    uint32_t srcVtx;
    uint32_t dstVtx;
    uint32_t sibling;
    int32_t userId = -1;
    int32_t nextEdge = -1;
    Vector2f normDir;
    float length;
  };

  inline std::size_t hash_value(Vector2i const& iPt)
  {

    if (sizeof(Vector2i) == sizeof(std::size_t))
    {
      return reinterpret_cast<size_t const&>(iPt);
    }
    else
    {
      size_t seed = iPt.X();
      boost::hash_combine(seed, iPt.Y());

      return seed;
    }
  }

  struct EXL_MATH_API PolyMesh
  {
    Vector<PolyVertex> vertices;
    Vector<PolyHalfEdge> edges;

    // boost::container::flat_map<Vector2i, uint32_t, std::less<Vector2i>, eXl::Allocator<std::pair<Vector2i, uint32_t>>> pointMap;
    // Need a way to keep capacity.
    UnorderedMap<Vector2i, uint32_t> pointMap;

    // leftmost/bottom most point
    Optional<std::pair<Vector2i, uint32_t>> smallerPoint;

    void UpdateSmallerPoint(std::pair<Vector2i, uint32_t> const& iNewPoint);

    PolyHalfEdge const* GetNextIncomingEdge(uint32_t curVtx, PolyHalfEdge const* curEdge) const;

    bool FindInsertionPoint(uint32_t srcVtx, uint32_t dstVtx, uint32_t firstEdge, Vector2f const& iOutgoingDir, int32_t& prevInsertPt, int32_t& nextInsertPt) const;

    Optional<uint32_t> InsertEdge(Segmenti const& iSeg, Segmentf const& iFltSeg, int32_t edgeId = -1);

    void Clear();
  };
}