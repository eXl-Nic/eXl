/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>

#include <math/polygon.hpp>
#include <math/aabb2d.hpp>
#include <map>

#include <boost/random.hpp>

#define BOOST_MULTI_INDEX_DISABLE_SERIALIZATION

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/container/flat_map.hpp>

namespace voro
{
  class wall;
  class container_poly;
}

namespace eXl
{
  enum CellKind
  {
    CK_Circle = 0,
    CK_Rectangle = 1,
    CK_Capsule = 2,
    CK_KindShift = 2,
    CK_KindMask = 3
  };

  class EXL_GEN_API Particle
  {
  public:

    inline Particle(uint32_t iId)
      : m_Id(iId)
      , m_Flags(0)
    {}

    inline unsigned int GetId() const { return m_Id; }
    inline uint32_t GetKind() const { return m_Flags & CK_KindMask; }

    inline Vec2& GetPos() { return m_Pos; }
    inline Vec2 const& GetPos() const { return m_Pos; }
    inline float& GetAngle() { return m_Angle; }
    inline float  GetAngle() const { return m_Angle; }

  protected:
    Vec2 m_Pos;
    float    m_Angle;
    uint32_t m_Flags;
    uint32_t m_Id;
  };

  class EXL_GEN_API Circle : public Particle
  {
  public:
    inline Circle(uint32_t iId)
      :Particle(iId)
    {
      m_Flags |= CK_Circle;
    }
    inline float& GetRadius() { return radius; }
    inline float GetRadius() const { return radius; }
    void GetAABB(AABB2D<float>& oBox) const;
  protected:
    float radius;
  };

  struct EXL_GEN_API Wall : public Particle
  {
  public:
    inline Wall(uint32_t iId) : Particle(iId) {}
    virtual voro::wall& GetWall() = 0;
  };

  class EXL_GEN_API Rect : public Wall
  {
  public:
    Rect(unsigned int iId, Vec2 const& iHalfDim, int iWallId);
    //inline Vec2& GetHalfDim() { return halfDim; }
    Vec2 const& GetHalfDim() const;
    voro::wall& GetWall();
    void GetAABB(AABB2D<float>& oBox) const;
  protected:
    voro::wall* m_Wall;
  };

  class EXL_GEN_API Capsule : public Wall
  {
  public:
    Capsule(unsigned int iId, float iRadius, float iLength, int iWallId);
    //inline Vec2& GetHalfDim() { return halfDim; }
    float GetRadius() const;
    float GetLength() const;
    voro::wall& GetWall();
    void GetAABB(AABB2D<float>& oBox) const;
  protected:
    voro::wall* m_Wall;
  };

  typedef voro::container_poly VoronoiDiagram;

  class EXL_GEN_API VoronoiGraph
  {
  public:

    VoronoiGraph();
    ~VoronoiGraph();

    struct Vertex
    {
      Vec2 m_Position;
    };

    struct Edge
    {
      uint32_t m_Pt1;
      uint32_t m_Pt2;
    };

    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, 
      boost::no_property,
      boost::property<boost::edge_name_t, uint32_t,
      boost::property<boost::edge_weight_t, float> > > CellGraphImpl;
    typedef UnorderedMap<CellGraphImpl::edge_descriptor, Edge> EdgeMap;
    typedef Vector<Vertex> EdgesVertexArray;

    struct CellGraph
    {
      CellGraphImpl    m_CellGraph;
      EdgeMap          m_Edges;
      EdgesVertexArray m_Vertices;
    };

    typedef Vector<Circle*> CircleArray;
    typedef Vector<Wall*>   WallArray;

    inline CircleArray::const_iterator BeginCircles() const { return m_Circles.begin(); }
    inline CircleArray::const_iterator EndCircles()   const { return m_Circles.end(); }

    inline WallArray::const_iterator BeginWalls() const { return m_Walls.begin(); }
    inline WallArray::const_iterator EndWalls()   const { return m_Walls.end(); }

    void SetAABB(AABB2Df const& iAABB)
    {
      Clear();
      m_AABB = iAABB;
    }

    //Verrue...
    VoronoiDiagram& GetDiagram() const
    {
      if (m_Diagram == nullptr)
      {
        MakeDiagram();
      }
      return *m_Diagram;
    }

    inline AABB2Df const& GetAABB() const { return m_AABB; }

    void Clear();

    uint32_t AddCircle(float iRadius, Vec2 const& iPos);

    uint32_t AddCapsuleWall(float iRadius, float iLength, Vec2 const& iPos, float iAngle);

    uint32_t AddRectangleWall(Vec2 const& iHalfDims, Vec2 const& iPos, float iAngle);

    inline Particle const* GetParticle(uint32_t iId) const
    {
      if (iId < m_IdCounter)
        return m_ParticleMap[iId];
      return nullptr;
    }

    void BuildGraph(CellGraph& oGraph, bool iSmoothBorder) const;

    void GetCells(Vector<Polygond>& oCells);

  protected:

    void ClearDiagram() const;

    void MakeDiagram() const;

    mutable VoronoiDiagram* m_Diagram;
    CircleArray       m_Circles;
    WallArray         m_Walls;
    AABB2Df           m_AABB;
    uint32_t          m_IdCounter;
    Vector<Particle*> m_ParticleMap;
    float             m_Epsilon;
  };
}
