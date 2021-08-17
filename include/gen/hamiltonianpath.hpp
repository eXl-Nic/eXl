#pragma once

#include <gen/gen_exp.hpp>
#include <math/polygon.hpp>
#include <math/aabb2dpolygon.hpp>

namespace eXl
{
  class Random;

  class EXL_GEN_API HamiltonianPath
  {
  public:

    struct Point
    {
      Point(Vector2i const& iPos) : pos(iPos), neigh1(-1), neigh2(-1) {}
      Vector2i pos;
      int neigh1;
      int neigh2;
    };

    struct Segment
    {
      int pt1;
      int pt2;
    };

    inline HamiltonianPath(HamiltonianPath&& iOther);
    inline HamiltonianPath(HamiltonianPath const& iOther);

    HamiltonianPath(Polygoni const& iPoly, unsigned int iGridSize);

    //HamiltonianPath(LevelGrammar_Old::Graph const& iGraph, unsigned int iGridSize);

    void MakePoly(AABB2DPolygoni& oCycle, bool iFilled) const;

    void BackbiteMove(Random& iRand);

    inline unsigned int GetNumPoints() const {return m_NumPoints;}

	  void Cull(Random& iRand, float iReduc, Vector2f iPathRange, Vector<HamiltonianPath>* oPathes = NULL);

    inline Point const& GetPoint(unsigned int iNum) const {return m_Points[iNum];}

    inline unsigned int GetStart() const {return m_Start;}
    inline unsigned int GetEnd() const {return m_End;}

  protected:

    HamiltonianPath();

    bool ConnectedPoint(int numPt) const;

    AABB2Di        m_GridBox;
    Vector2i       m_Offset;
    unsigned int   m_GridSize;
    unsigned int   m_NumPoints;
    Vector<int>    m_Grid;
    Vector<Point>  m_Points;
    int   m_Start;
    int   m_End;

  };

  inline HamiltonianPath::HamiltonianPath(HamiltonianPath&& iOther)
    :m_GridBox(iOther.m_GridBox)
    ,m_Offset(iOther.m_Offset)
    ,m_GridSize(iOther.m_GridSize)
    ,m_NumPoints(iOther.m_NumPoints)
    ,m_Start(iOther.m_Start)
    ,m_End(iOther.m_End)
  {
    m_Grid.swap(iOther.m_Grid);
    m_Points.swap(iOther.m_Points);
  }

  inline HamiltonianPath::HamiltonianPath(HamiltonianPath const& iOther)
    :m_GridBox(iOther.m_GridBox)
    ,m_Offset(iOther.m_Offset)
    ,m_GridSize(iOther.m_GridSize)
    ,m_NumPoints(iOther.m_NumPoints)
    ,m_Start(iOther.m_Start)
    ,m_End(iOther.m_End)
    ,m_Grid(iOther.m_Grid)
    ,m_Points(iOther.m_Points)
  {
  }

}
