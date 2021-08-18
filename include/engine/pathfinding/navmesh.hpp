/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/coredef.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <math/geometrytraits.hpp>
#include <math/aabb2d.hpp>
#include <math/aabb2dpolygon.hpp>
#include <math/segment.hpp>
#include <boost/optional.hpp>

namespace eXl
{
  class EXL_ENGINE_API NavMesh
  {
  public:
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, 
      boost::no_property,
      boost::property<boost::edge_weight_t, float> > Graph;

    using BoxIndexEntry = std::pair<AABB2Df, uint32_t>;
    using BoxIndex = boost::geometry::index::rtree<BoxIndexEntry, boost::geometry::index::rstar<16, 4>>;

    struct EXL_ENGINE_API Face
    {
      AABB2Df m_Box;
      Vector<Graph::vertex_descriptor> m_Edges;
      Vector<Segmentf> m_Walls;

      Vector2f GetTreadmillDir(Vector2f const& iPos) const;

      // Left, Bottom, Right, Top
      bool m_HasSide[4];
    };

    struct Edge
    {
      boost::optional<uint32_t> CommonFace(Edge const& iOther) const;

      uint32_t face1;
      uint32_t face2;
      Segmentf segment;
    };

    //typedef UnorderedMap<Graph::vertex_descriptor, Edge> EdgeMap;
    typedef Vector<Edge> EdgeMap;

    struct Component
    {
      BoxIndex m_FacesIdx;
      Vector<Face> m_Faces;

      Graph m_Graph;

      EdgeMap m_FaceEdges;
    };

    static NavMesh MakeFromAABB2DPoly(Vector<AABB2DPolygoni> const& iPolys);

    static NavMesh MakeFromBoxes(Vector<AABB2Di> const& iBoxes);

    Vector<Component> const& GetComponents() const { return m_Components; }

    Graph const& GetGraph(uint32_t iComponent) const 
    { return m_Components[iComponent].m_Graph; }
    Vector<Face> const& GetFaces(uint32_t iComponent) const 
    { return m_Components[iComponent].m_Faces; }
    EdgeMap const& GetEdges(uint32_t iComponent) const 
    { return m_Components[iComponent].m_FaceEdges; }

    struct FoundFace
    {
      bool operator == (FoundFace const& iOther)
      {
        return m_Component == iOther.m_Component && m_Face == iOther.m_Face;
      }
      uint32_t m_Component;
      uint32_t m_Face;
    };

    boost::optional<FoundFace> FindFace(Vector2f const& iPos) const;

    struct Path
    {
      uint32_t m_Component;
      // Stack, first destination is at the back.
      Vector<Graph::vertex_descriptor> m_Edges;
      Vector<Vector2f> m_EdgeDirs;
    };

    boost::optional<Path> FindPath(Vector2f const& iStart, Vector2f const& iEnd) const;

  protected:

    Vector<Component> m_Components;
  };
}