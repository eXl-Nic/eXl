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

    inline Vector2f& GetPos() { return m_Pos; }
    inline Vector2f const& GetPos() const { return m_Pos; }
    inline float& GetAngle() { return m_Angle; }
    inline float  GetAngle() const { return m_Angle; }

  protected:
    Vector2f m_Pos;
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
    Rect(unsigned int iId, Vector2f const& iHalfDim, int iWallId);
    //inline Vector2f& GetHalfDim() { return halfDim; }
    Vector2f const& GetHalfDim() const;
    voro::wall& GetWall();
    void GetAABB(AABB2D<float>& oBox) const;
  protected:
    voro::wall* m_Wall;
  };

  class EXL_GEN_API Capsule : public Wall
  {
  public:
    Capsule(unsigned int iId, float iRadius, float iLength, int iWallId);
    //inline Vector2f& GetHalfDim() { return halfDim; }
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
      Vector2f m_Position;
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

    uint32_t AddCircle(float iRadius, Vector2f const& iPos);

    uint32_t AddCapsuleWall(float iRadius, float iLength, Vector2f const& iPos, float iAngle);

    uint32_t AddRectangleWall(Vector2f const& iHalfDims, Vector2f const& iPos, float iAngle);

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
