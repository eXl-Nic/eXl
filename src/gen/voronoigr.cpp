/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/voronoigr.hpp>
#include <voro++.hh>
#include <math/segment.hpp>

namespace eXl
{
  template <class Real>
  class Primitive_Distance
  {
  public:
    /** Vertical segment at (0,0)*/
    static Real NearestPointVSeg(Real iHalfLength, glm::vec<2, Real> const& iPos, glm::vec<2, Real>& oVect)
    {
      if (Math<Real>::FAbs(iPos.y) < iHalfLength)
      {
        oVect = glm::vec<2, Real>(-iPos.x, 0.0);
        return Math<Real>::FAbs(iPos.x);
      }
      else
      {
        oVect = glm::vec<2, Real>(-iPos.x, -iPos.y - (iPos.y > 0.0 ? -1.0 : 1.0) * iHalfLength);
        return NormalizeAndGetLength(oVect);
      }
    }

    /**Rect is at (0,0)*/
    static Real NearestPointRect(glm::vec<2, Real> const& iHalfDim, glm::vec<2, Real> const& iPos, glm::vec<2, Real>& oVect, unsigned int& oSelDist)
    {
      glm::vec<2, Real> dists[4];

      dists[0].x = -iHalfDim.x - iPos.x;
      dists[1].x = +iHalfDim.x - iPos.x;

      if (iPos.y > iHalfDim.y)
      {
        dists[0].y = dists[1].y = iHalfDim.y - iPos.y;
      }
      else if (iPos.y < -iHalfDim.y)
      {
        dists[0].y = dists[1].y = -iHalfDim.y - iPos.y;
      }

      dists[2].y = -iHalfDim.y - iPos.y;
      dists[3].y = +iHalfDim.y - iPos.y;

      if (iPos.x > iHalfDim.x)
      {
        dists[2].x = dists[3].x = iHalfDim.x - iPos.x;
      }
      else if (iPos.x < -iHalfDim.x)
      {
        dists[2].x = dists[3].x = -iHalfDim.x - iPos.x;
      }

      oSelDist = 0;
      Real curMin = length(dists[0]);
      for (unsigned int i = 1; i < 4; ++i)
      {
        Real curDist = length(dists[i]);
        if (curDist < curMin)
        {
          curMin = curDist;
          oSelDist = i;
        }
      }

      oVect = dists[oSelDist];
      return NormalizeAndGetLength(oVect);
    }
  };

  class wall_Rect : public voro::wall
  {
  public:
    wall_Rect(Vec2 const& iHalfDim, int iId) : wall()
      , m_HalfDim(iHalfDim)
      , w_id(iId)
    {

    }

    virtual bool point_inside(double x, double y, double z)
    {
      Vec2 traj(x - m_Pos.x, y - m_Pos.y);
      Vec2 trigAngle(Mathf::Cos(m_Angle), Mathf::Sin(m_Angle));
      Vec2 trajInRect = Vec2(traj.x * trigAngle.x + traj.y * trigAngle.y,
        traj.y * trigAngle.x - traj.x * trigAngle.y);
      AABB2Df box = AABB2Df::FromCenterAndSize(Zero<Vec2>(), m_HalfDim * 2.f);
      return !box.Contains(trajInRect);
    }

    template <class vcell>
    bool cut_cell_base(vcell& c, double x, double y, double z)
    {
      Vec2 traj(x - m_Pos.x, y - m_Pos.y);
      Vec2 trigAngle(Mathf::Cos(m_Angle), Mathf::Sin(m_Angle));
      Vec2 trajInRect = Vec2(traj.x * trigAngle.x + traj.y * trigAngle.y,
        traj.y * trigAngle.x - traj.x * trigAngle.y);

      AABB2Df box(Zero<Vec2>(), m_HalfDim * 2);
      if (box.Contains(trajInRect))
        return false;

      float const shrinkCoeff = 0.0 * (m_HalfDim.x > m_HalfDim.y ? m_HalfDim.y : m_HalfDim.x);

      Vec2 const halfDim(m_HalfDim.x, m_HalfDim.y);
      Vec2 normalVect;
      unsigned int selDist;
      float curMin = Primitive_Distance<float>::NearestPointRect(halfDim, trajInRect, normalVect, selDist);

      normalVect = normalize(normalVect);
      normalVect = Vec2(normalVect.x * trigAngle.x - normalVect.y * trigAngle.y,
        normalVect.y * trigAngle.x + normalVect.x * trigAngle.y);
      //float dq = traj.x * traj.x + traj.y * traj.y;

      float dq = 2 * (curMin - shrinkCoeff);
      return c.nplane(normalVect.x, normalVect.y, 0.0, dq, w_id);

    }

    void SetPos(Vec2 const& iPos, float iAngle)
    {
      m_Pos = iPos;
      m_Angle = iAngle;
    }

    virtual bool cut_cell(voro::voronoicell& c, double x, double y, double z)
    {
      return cut_cell_base(c, x, y, z);
    }

    virtual bool cut_cell(voro::voronoicell_neighbor& c, double x, double y, double z)
    {
      return cut_cell_base(c, x, y, z);
    }

    inline Vec2 const& GetHalfDim() const { return m_HalfDim; }

  protected:
    Vec2 m_HalfDim;
    Vec2 m_Pos;
    float m_Angle;
    const int w_id;
  };

  class wall_Capsule : public voro::wall
  {
  public:
    wall_Capsule(float iRadius, float iLength, int iId) : wall()
      , m_Radius(iRadius)
      , m_Length(iLength)
      , w_id(iId)
    {

    }

    virtual bool point_inside(double x, double y, double z)
    {
      Vec2 traj(x - m_Pos.x, y - m_Pos.y);
      Vec2 trigAngle(Mathf::Cos(m_Angle), Mathf::Sin(m_Angle));
      Vec2 trajInCaps = Vec2(traj.x * trigAngle.x + traj.y * trigAngle.y,
        traj.y * trigAngle.x - traj.x * trigAngle.y);

      return Primitive_Distance<float>::NearestPointVSeg(m_Length / 2.0, trajInCaps, traj) > m_Radius;
    }

    template <class vcell>
    bool cut_cell_base(vcell& c, double x, double y, double z)
    {
      Vec2 traj(x - m_Pos.x, y - m_Pos.y);
      Vec2 trigAngle(Mathf::Cos(m_Angle), Mathf::Sin(m_Angle));
      Vec2 trajInCaps = Vec2(traj.x * trigAngle.x + traj.y * trigAngle.y,
        traj.y * trigAngle.x - traj.x * trigAngle.y);

      if (!(Primitive_Distance<float>::NearestPointVSeg(m_Length / 2.0, trajInCaps, traj) > m_Radius))
        return false;

      float const shrinkCoeff = 0.0; //0.1 * (m_HalfDim.x > m_HalfDim.y ? m_HalfDim.y : m_HalfDim.x);

      Vec2 trajToPoint;
      float dist = Primitive_Distance<float>::NearestPointVSeg(m_Length / 2.0, trajInCaps, trajToPoint);

      Vec2 normalVect = trajToPoint;
      normalVect = normalize(normalVect);
      normalVect = Vec2(normalVect.x * trigAngle.x - normalVect.y * trigAngle.y,
        normalVect.y * trigAngle.x + normalVect.x * trigAngle.y);

      float dq = 2 * (dist - m_Radius);
      return c.nplane(normalVect.x, normalVect.y, 0.0, dq, w_id);
    }

    void SetPos(Vec2 const& iPos, float iAngle)
    {
      m_Pos = iPos;
      m_Angle = iAngle;
    }

    virtual bool cut_cell(voro::voronoicell& c, double x, double y, double z)
    {
      return cut_cell_base(c, x, y, z);
    }

    virtual bool cut_cell(voro::voronoicell_neighbor& c, double x, double y, double z)
    {
      return cut_cell_base(c, x, y, z);
    }

    inline float GetRadius() const { return m_Radius; }
    inline float GetLength() const { return m_Length; }

  protected:
    float m_Radius;
    float m_Length;
    Vec2 m_Pos;
    float m_Angle;
    const int w_id;
  };

  void Circle::GetAABB(AABB2Df& oBox) const
  {
    oBox = AABB2Df::FromCenterAndSize(m_Pos, Vec2(radius, radius) * 2.f);
  }

  Rect::Rect(unsigned int iId, Vec2 const& iHalfDim, int iWallId)
    :Wall(iId)
    , m_Wall(new wall_Rect(iHalfDim, iWallId))
  {
    m_Flags |= CK_Rectangle;
  }
  //inline Vec2& GetHalfDim() { return halfDim; }
  Vec2 const& Rect::GetHalfDim() const { return static_cast<wall_Rect const*>(m_Wall)->GetHalfDim(); }
  voro::wall& Rect::GetWall()
  {
    static_cast<wall_Rect*>(m_Wall)->SetPos(GetPos(), GetAngle());
    return *m_Wall;
  }

  void Rect::GetAABB(AABB2Df& oBox) const
  {
    oBox = AABB2Df::FromCenterAndSize(m_Pos, Vec2(static_cast<wall_Rect*>(m_Wall)->GetHalfDim()) * 2.f);
    oBox.Rotate(GetAngle());
  }

  Capsule::Capsule(unsigned int iId, float iRadius, float iLength, int iWallId)
    :Wall(iId)
    , m_Wall(new wall_Capsule(iRadius, iLength, iWallId))
  {
    m_Flags |= CK_Capsule;
  }

  float Capsule::GetRadius() const { return static_cast<wall_Capsule const*>(m_Wall)->GetRadius(); }
  float Capsule::GetLength() const { return static_cast<wall_Capsule const*>(m_Wall)->GetRadius(); }

  voro::wall& Capsule::GetWall()
  {
    static_cast<wall_Capsule*>(m_Wall)->SetPos(GetPos(), GetAngle());
    return *m_Wall;
  }

  void Capsule::GetAABB(AABB2Df& oBox) const
  {
    oBox = AABB2Df::FromCenterAndSize(m_Pos, Vec2(GetRadius(), GetLength() + GetRadius() * 2.0));
    oBox.Rotate(GetAngle());
  }

  VoronoiGraph::VoronoiGraph()
    : m_Diagram(nullptr)
    , m_IdCounter(0)
    , m_Epsilon(100.0)
  {
  }

  VoronoiGraph::~VoronoiGraph()
  {
    Clear();
  }

  static const uint32_t blockXSize = 32;
  static const uint32_t blockYSize = 32;
  static const uint32_t gridSize = blockXSize * blockYSize;

  void VoronoiGraph::MakeDiagram() const
  {
    if (m_Diagram == nullptr)
    {
      m_Diagram = new VoronoiDiagram(m_AABB.m_Data[0].x, m_AABB.m_Data[1].x, m_AABB.m_Data[0].y, m_AABB.m_Data[1].y, 0, 1, blockXSize, blockYSize, 1, false, false, false, m_Circles.size());
      for (uint32_t i = 0; i < m_Walls.size(); ++i)
      {
        m_Diagram->add_wall(m_Walls[i]->GetWall());
      }
    }
    m_Diagram->clear();
    m_Diagram->update_box_size(m_AABB.m_Data[0].x, m_AABB.m_Data[1].x, m_AABB.m_Data[0].y, m_AABB.m_Data[1].y, 0, 1);
    //m_Diagram->ax = m_AABB.m_Data[0].x;
    //m_Diagram->ay = m_AABB.m_Data[0].y;
    //m_Diagram->bx = m_AABB.m_Data[1].x;
    //m_Diagram->by = m_AABB.m_Data[1].y;
    for (uint32_t i = 0; i<m_Circles.size(); ++i)
    {
      m_Diagram->put(i, m_Circles[i]->GetPos().x, m_Circles[i]->GetPos().y, 0.5, m_Circles[i]->GetRadius());
    }
  }

  void VoronoiGraph::Clear()
  {
    ClearDiagram();

    for (uint32_t i = 0; i < m_Circles.size(); ++i)
    {
      delete m_Circles[i];
    }

    m_Circles.clear();

    for (uint32_t i = 0; i < m_Walls.size(); ++i)
    {
      delete m_Walls[i];
    }

    m_Walls.clear();
    m_ParticleMap.clear();
    m_AABB = AABB2Df();
    m_IdCounter = 0;
  }

  void VoronoiGraph::ClearDiagram() const
  {
    if (m_Diagram != nullptr)
    {
      delete m_Diagram;
    }
    m_Diagram = nullptr;
  }

  uint32_t VoronoiGraph::AddCircle(float iRadius, Vec2 const& iPos)
  {
    ClearDiagram();
    AABB2Df newAABB;
    Circle* newCircle = new Circle(m_IdCounter);
    newCircle->GetPos() = iPos;
    newCircle->GetRadius() = iRadius;
    newCircle->GetAABB(newAABB);
    if (m_AABB.Empty())
    {
  
      m_AABB = newAABB;
    }
    else
    {
      m_AABB.Absorb(newAABB);
    }

    ++m_IdCounter;

    m_Circles.push_back(newCircle);
    m_ParticleMap.push_back(newCircle);

    if (0.001 * iRadius < m_Epsilon)
      m_Epsilon = 0.001 * iRadius;

    return newCircle->GetId();
  }


  uint32_t VoronoiGraph::AddCapsuleWall(float iRadius, float iLength, Vec2 const& iPos, float iAngle)
  {
    ClearDiagram();
    AABB2Df newAABB;
    Capsule* newCapsule = new Capsule(m_IdCounter, iRadius, iLength, -int(m_Walls.size()) - 7);
    newCapsule->GetPos() = iPos;
    newCapsule->GetAngle() = iAngle;
    newCapsule->GetAABB(newAABB);
    if (m_AABB.Empty())
    {
      m_AABB = newAABB;
    }
    else
    {
      m_AABB.Absorb(newAABB);
    }
  
    ++m_IdCounter;
    m_Walls.push_back(newCapsule);
    m_ParticleMap.push_back(newCapsule);

    return newCapsule->GetId();
  }


  uint32_t VoronoiGraph::AddRectangleWall(Vec2 const& iHalfDims, Vec2 const& iPos, float iAngle)
  {
    ClearDiagram();
    AABB2Df newAABB;
    Rect* newRect = new Rect(m_IdCounter, iHalfDims, -int(m_Walls.size()) - 7);
    newRect->GetPos() = iPos;
    newRect->GetAngle() = iAngle;
    newRect->GetAABB(newAABB);
    if (m_AABB.Empty())
    {
    
      m_AABB = newAABB;
    }
    else
    {
      m_AABB.Absorb(newAABB);
    }
  
    ++m_IdCounter;
    m_Walls.push_back(newRect);
    m_ParticleMap.push_back(newRect);

    return newRect->GetId();
  }


  struct CIEdge;

  struct GridPoint
  {
    GridPoint()
    {
      //cellRef = 0;
    }
    Vec2 m_Pos;
    uint32_t m_Identifier;
    //mutable int cellRef;
    //mutable std::list<CellEdgeMap::iterator > outEdges;
  };

  typedef Vector<std::list<GridPoint> > PointGrid;

  struct CIPointRef
  {
    inline bool operator==(CIPointRef const& iOther)const
    {
      return !this->operator!=(iOther);
    }
    inline bool operator!=(CIPointRef const& iOther)const
    {
      return gridLoc != iOther.gridLoc || listIter != iOther.listIter;
    }
    uint32_t gridLoc;
    std::list<GridPoint>::const_iterator listIter;
  };
  struct CIEdge
  {
    CIEdge()
    {
      borderEdge = false;
    }
    int CommonPoint(CIEdge const& iOther) const
    {
      if (point[0] == iOther.point[0])
        return 0;
      if (point[1] == iOther.point[0])
        return 1;
      if (point[0] == iOther.point[1])
        return 2;
      if (point[1] == iOther.point[1])
        return 3;
      return -1;
    }
    //IPointSet::iterator point[0];
    //IPointSet::iterator point[1];
    CIPointRef point[2];
    bool borderEdge;
  };
  typedef std::map<int, CIEdge> CellEdgeMap;
  struct CellInfo
  {
    CellInfo()
    {
      m_Initialized = false;
    }
  
    bool operator<(CellInfo const& iOther) const
    {
      return m_Id < iOther.m_Id;
    }
    //Not movable...
    //voro::voronoicell_neighbor m_VoroCell;
    bool m_Initialized;
    int m_Id;
    uint32_t m_DiagPid;
    //Vec2 m_RelPos;
    double origPos[3];
    float m_Radius;
    CellEdgeMap m_Edges;
    //Vector<int> m_Neighbours;
  };

  bool ComparePoints(std::list<GridPoint> const& gridCase, Vec2 const& iPoint, float iTol, CIPointRef& oRef)
  {
  
    for (std::list<GridPoint>::const_iterator iter = gridCase.begin(), iterEnd = gridCase.end();
      iter != iterEnd; ++iter)
    {
      if (length(iPoint - iter->m_Pos) < iTol)
      {
        oRef.listIter = iter;
        return true;
      }
    }
    return false;
  }

  CIPointRef InsertPoint(PointGrid& ioGrid, Vec2 const& iPoint, Vec2 const& iIncr, Vec2i const& iSize, float iTol, uint32_t& oNumPts)
  {
    Vec2i gridCoord(iPoint.x / iIncr.x, iPoint.y / iIncr.y);
    CIPointRef oRef;
    oRef.gridLoc = gridCoord.y * iSize.x + gridCoord.x;
    if (ComparePoints(ioGrid[oRef.gridLoc], iPoint, iTol, oRef))
    {
      return oRef;
    }
    if (gridCoord.x > 0 && ComparePoints(ioGrid[oRef.gridLoc - 1], iPoint, iTol, oRef))
    {
      oRef.gridLoc -= 1;
      return oRef;
    }
    if (gridCoord.x + 1 < iSize.x && ComparePoints(ioGrid[oRef.gridLoc + 1], iPoint, iTol, oRef))
    {
      oRef.gridLoc += 1;
      return oRef;
    }
    if (gridCoord.y > 0 && ComparePoints(ioGrid[oRef.gridLoc - iSize.x], iPoint, iTol, oRef))
    {
      oRef.gridLoc -= iSize.x;
      return oRef;
    }
    if (gridCoord.y + 1 < iSize.y && ComparePoints(ioGrid[oRef.gridLoc + iSize.x], iPoint, iTol, oRef))
    {
      oRef.gridLoc += iSize.x;
      return oRef;
    }
    GridPoint newPoint;
    newPoint.m_Pos = iPoint;
    newPoint.m_Identifier = oNumPts;
    ioGrid[oRef.gridLoc].push_front(newPoint);
    oRef.listIter = ioGrid[oRef.gridLoc].begin();
    ++oNumPts;
    return oRef;
  }

  struct EdgeUniqMap;
  typedef std::multimap<uint32_t, CIEdge> BorderEdges;

  struct EdgeUniqMap : std::map < std::pair<uint32_t, uint32_t>, CIEdge > {
  };

  struct ParticleKey
  {
    ParticleKey(Vec2d const& iPos)
    {
      pos[0] = iPos.x;
      pos[1] = iPos.y;
    }
    inline bool operator <(ParticleKey const& iOther)const
    {
      if (key[0] == iOther.key[0])
        return key[1] < iOther.key[1];
      else
        return key[0] < iOther.key[0];
    }
    union
    {
      double pos[2];
      int64_t key[2];
    };
  };

  //typedef std::map<ParticleKey, Vec2d> WallMap;
  typedef std::list<std::pair<uint32_t, CIEdge> > BorderEdgeList;

  typedef std::map<int, Vec2d> WallMap;
  struct BorderCut
  {
    CellEdgeMap::iterator iterCell;
    CellEdgeMap::iterator iterCellBorder;
    int commonPoint;
    BorderEdgeList::iterator iterBorder;
    //WallMap::iterator positionInMap;
    //Vec2d direction;
    //Vec2d positionOnSeg;
    //Vec2d otherPosition;
  };

  typedef std::map<std::pair<uint32_t, uint32_t>, BorderCut > EdgeTwinMap;

  bool GetEdgeFromFaces(std::vector<double> const& iVertices, std::vector<int> const& iFaceO, std::vector<int>const& iFaceV, uint32_t iIdx, uint32_t iOffset, Vec2d& oPt1, Vec2d& oPt2)
  {
    int indic = 0;
    for (int j = 0; j < iFaceO[iIdx]; ++j)
    {
      if (iVertices[3 * iFaceV[iOffset + j] + 2] < 0.5)
      {
        indic |= 1;
      }
      if (iVertices[3 * iFaceV[iOffset + j] + 2] > 0.5)
      {
        indic |= 2;
      }
    }
    assert(indic == 3);

    if (indic == 3)
    {
      for (int j = 0; j < iFaceO[iIdx] - 1; ++j)
      {
        if ((iVertices[3 * iFaceV[iOffset + j] + 2] < 0.5 && iVertices[3 * iFaceV[iOffset + j + 1] + 2] < 0.5)
          || (iVertices[3 * iFaceV[iOffset + j] + 2] > 0.5 && iVertices[3 * iFaceV[iOffset + j + 1] + 2] > 0.5))
        {
          oPt1.x = iVertices[3 * iFaceV[iOffset + j + 0] + 0];
          oPt1.y = iVertices[3 * iFaceV[iOffset + j + 0] + 1];
          oPt2.x = iVertices[3 * iFaceV[iOffset + j + 1] + 0];
          oPt2.y = iVertices[3 * iFaceV[iOffset + j + 1] + 1];
          break;
        }
      }
      return true;
    }
    return false;
  }

  uint32_t FindCellEges(std::vector<double> const& iVertices, std::vector<int> const& iFaceO, std::vector<int>const& iFaceV, std::vector<int> const& iNeighs,
    uint32_t iOrigIdx, Vec2d iRefPt, Vec2d& oPt1, Vec2d& oPt2, unsigned char& ioHandledWallsMask)
  {
    uint32_t res = 0;
    uint32_t numNh = iNeighs.size();
    int triedPt[5] = {(int)iOrigIdx, -1, -1, -1, -1 };
    //2 tries to follow corners.
    for (uint32_t i = 0; i < 4; ++i)
    {
      uint32_t offsetI = 1;
      for (res = 0; res < numNh; ++res)
      {
        if (res != triedPt[0]
          && res != triedPt[1]
          && res != triedPt[2]
          && res != triedPt[3]
          && iNeighs[res] != -5
          && iNeighs[res] != -6)
        {
          Vec2d curEdgePt1;
          Vec2d curEdgePt2;

          GetEdgeFromFaces(iVertices, iFaceO, iFaceV, res, offsetI, curEdgePt1, curEdgePt2);
          if (curEdgePt1 == iRefPt)
          {
            if (iNeighs[res] >= 0 || iNeighs[res] < -6)
            {
              oPt1 = iRefPt;
              oPt2 = curEdgePt2;
              break;
            }
            else
            {
              uint32_t curNeighbour = -1 - iNeighs[res];
              ioHandledWallsMask |= 1 << curNeighbour;
              iRefPt = curEdgePt2;
              triedPt[i + 1] = res;
            }
          }
          else if (curEdgePt2 == iRefPt)
          {
            if (iNeighs[res] >= 0 || iNeighs[res] < -6)
            {
              oPt1 = iRefPt;
              oPt2 = curEdgePt1;
              break;
            }
            else
            {
              uint32_t curNeighbour = -1 - iNeighs[res];
              ioHandledWallsMask |= 1 << curNeighbour;
              iRefPt = curEdgePt1;
              triedPt[i + 1] = res;
            }
          }
        }
        offsetI += iFaceO[res] + 1;
      }
    }
    return res;
  }

  CellEdgeMap::iterator FindCellConnectedWith(CellEdgeMap& iEdges, CellEdgeMap::iterator filterIter, CIPointRef const& iPoint)
  {
    for (CellEdgeMap::iterator iter = iEdges.begin(), iterEnd = iEdges.end(); iter != iterEnd; ++iter)
    {
      if (filterIter != iter)
      {
        if (iter->second.point[0] == iPoint
          || iter->second.point[1] == iPoint)
        {
          return iter;
        }
      }
    }
    return iEdges.end();
  }

  void InitializeBorderPlanes(WallMap& additionalWalls, VoronoiDiagram& iDiagram, Vector<CellInfo>& cells, Vector<Circle*> const& iCircles, Vec2 iMin, Vector<uint32_t>& mapping, bool iSmoothBorder)
  {
    voro::voronoicell_neighbor cell;
    {
      EdgeTwinMap twinMap;

      voro::c_loop_all looper(iDiagram);

      if (looper.start())
      {
        uint32_t loopStep = 0;
        do
        {
          if (iDiagram.compute_cell(/*cells[loopStep].m_VoroCell*/cell, looper))
          {
            //voro::voronoicell_neighbor& cell = cells[loopStep].m_VoroCell;
            uint32_t pid = looper.pid();

            CellInfo& newCell = cells[loopStep];
            Circle& part = *iCircles[pid];
            looper.pos(newCell.origPos[0], newCell.origPos[1], newCell.origPos[2]);
            Vec2d partPos(newCell.origPos[0], newCell.origPos[1]);
            //newCell.m_RelPos = partPos - iMin;
            newCell.m_Id = part.GetId();
            mapping[newCell.m_Id] = loopStep;
            newCell.m_DiagPid = pid;
            newCell.m_Initialized = true;
            newCell.m_Radius = part.GetRadius();

            std::vector<double> vertices;
            cell.vertices(newCell.origPos[0], newCell.origPos[1], newCell.origPos[2], vertices);
            std::vector<int> faceO;
            std::vector<int> faceV;
            cell.face_orders(faceO);
            cell.face_vertices(faceV);

            std::vector<int> neighs;
            cell.neighbors(neighs);

            uint32_t numNh = neighs.size();

            uint32_t offsetI = 1;
            unsigned char handledWallMask = 0;
            for (uint32_t i = 0; i < numNh; ++i)
            {
              if (iSmoothBorder && neighs[i] < 0 && neighs[i] > -5)
              {
                Vec2d pt1;
                Vec2d pt2;
                uint32_t curNeighbour = -1 - neighs[i];

                if ((1 << curNeighbour) & handledWallMask)
                  continue;

                GetEdgeFromFaces(vertices, faceO, faceV, i, offsetI, pt1, pt2);

                Vec2d edge1Pt1;
                Vec2d edge1Pt2;
                Vec2d edge2Pt1;
                Vec2d edge2Pt2;

                uint32_t adjEdge1 = FindCellEges(vertices, faceO, faceV, neighs, i, pt1, edge1Pt1, edge1Pt2, handledWallMask);
                uint32_t adjEdge2 = FindCellEges(vertices, faceO, faceV, neighs, i, pt2, edge2Pt1, edge2Pt2, handledWallMask);

                handledWallMask |= 1 << curNeighbour;

                Vec2d outDir;
                outDir[curNeighbour / 2] = -1.0 + (curNeighbour % 2) * 2.0;
                Vec2d oPoint;
                Vec2d dir;
                uint32_t res = Segmentd::Intersect(edge1Pt1, edge1Pt2, edge2Pt1, edge2Pt2, oPoint);
                if (res & Segmentd::PointFound)
                {
                  dir = oPoint - partPos;
                }
                else
                {
                  // // segments.
                  dir = edge1Pt2 - edge1Pt1;
                }
                dir = normalize(dir);
                if (dot(dir, outDir) < 0.0)
                  dir = dir * -1.0;
                Vec2d normalVect(-dir.y, dir.x);

                float currentDist = part.GetRadius();
                oPoint = partPos + dir * static_cast<double>(currentDist);

                Vec2d posOnSeg1;
                Vec2d posOnSeg2;

                res = Segmentd::NearestPointOnSeg1(edge1Pt1, edge1Pt2, oPoint, oPoint + normalVect, posOnSeg1);
                res = Segmentd::NearestPointOnSeg1(edge2Pt1, edge2Pt2, oPoint, oPoint + normalVect, posOnSeg2);

                std::pair<WallMap::iterator, bool> insertRes = additionalWalls.insert(std::make_pair(pid, oPoint));

              }
              offsetI += faceO[i] + 1;
            }
          }
          ++loopStep;
        } while (looper.inc());
      }
    }
  }

  template <uint32_t iCellPoint, uint32_t iBorderPoint>
  void InsertAndAdjust(CellInfo& iCell, CellEdgeMap::iterator borderEdgeIter, CellEdgeMap::iterator edgeIter, CIEdge iBorderEdge, EdgeTwinMap& oTwinMap, BorderEdgeList& oBorders)
  {
    std::pair<uint32_t, uint32_t> key;
    if (iCell.m_Id < edgeIter->first)
      key = std::make_pair(iCell.m_Id, edgeIter->first);
    else
      key = std::make_pair(edgeIter->first, iCell.m_Id);

    EdgeTwinMap::iterator iter = oTwinMap.find(key);
    if (iter == oTwinMap.end())
    {
      //assert(edgeIter->second.CommonPoint(borderEdgeIter->second) >= 0);
      if (edgeIter->second.CommonPoint(borderEdgeIter->second) >= 0)
      {
        CIEdge oBorderEdge;
        //+ proche de cellEdgePt1
        oBorderEdge.point[0] = iBorderEdge.point[iBorderPoint];
        oBorderEdge.point[1] = edgeIter->second.point[iCellPoint];
        edgeIter->second.point[iCellPoint] = iBorderEdge.point[iBorderPoint];

        BorderEdgeList::iterator newIter = oBorders.insert(oBorders.end(), std::make_pair(iCell.m_Id, oBorderEdge));
        BorderCut border = { edgeIter, borderEdgeIter != iCell.m_Edges.end() ? borderEdgeIter : edgeIter, iCellPoint, newIter };

        oTwinMap.insert(std::make_pair(key, border));
      }
    }
    else
    {
      int commonPoint = iter->second.commonPoint;
      if (commonPoint > 0)
      {
        assert(iter->second.iterBorder != oBorders.end());
        CIPointRef extPoint = iter->second.iterBorder->second.point[1];

        iter->second.iterCell->second.point[iter->second.commonPoint & 1] = extPoint;
      }
      if (iter->second.iterCell != iter->second.iterCellBorder)
      {
        commonPoint = iter->second.iterCell->second.CommonPoint(iter->second.iterCellBorder->second);

        if (commonPoint >= 0)
        {
          iter->second.iterCell->second.point[commonPoint & 1] = edgeIter->second.point[iCellPoint];
          iter->second.iterCellBorder->second.point[(commonPoint >> 1) & 1] = edgeIter->second.point[iCellPoint];
          if (iter->second.iterBorder != oBorders.end())
            oBorders.erase(iter->second.iterBorder);
          oTwinMap.erase(iter);
        }
      }
    }
  }

  void SplitCellEdgeBorder(CellInfo& iCell, CellEdgeMap::iterator borderEdgeIter, CellEdgeMap::iterator edgeIter, CIEdge iBorderEdge, /*CIEdge& oBorderEdge,*/ EdgeTwinMap& oTwinMap, BorderEdgeList& oBorders, float iEpsilon)
  {
    CIEdge& ioCellEdge = edgeIter->second;
    Vec2 cellEdgePt1 = ioCellEdge.point[0].listIter->m_Pos;
    Vec2 cellEdgePt2 = ioCellEdge.point[1].listIter->m_Pos;

    Vec2 borderEdgePt1 = iBorderEdge.point[0].listIter->m_Pos;
    Vec2 borderEdgePt2 = iBorderEdge.point[1].listIter->m_Pos;

    Vec2 dir = cellEdgePt2 - cellEdgePt1;
    float cellEdgeLength = NormalizeAndGetLength(dir);

    float dist1 = dot((borderEdgePt1 - cellEdgePt1), dir);
    float dist2 = dot((borderEdgePt2 - cellEdgePt1), dir);
    if (dist1 > -iEpsilon && dist1 - iEpsilon < cellEdgeLength)
    {
      if (dist2 > -iEpsilon && dist2 - iEpsilon < cellEdgeLength)
      {
        CIEdge oBorderEdge = iBorderEdge;
        if (ioCellEdge.point[0] == iBorderEdge.point[0]){ioCellEdge.point[0] = iBorderEdge.point[1];}
        else if (ioCellEdge.point[0] == iBorderEdge.point[1]){ ioCellEdge.point[0] = iBorderEdge.point[0]; }
        else if (ioCellEdge.point[1] == iBorderEdge.point[0]){ ioCellEdge.point[1] = iBorderEdge.point[1]; }
        else if (ioCellEdge.point[1] == iBorderEdge.point[1]){ ioCellEdge.point[1] = iBorderEdge.point[0]; }
        else
        {
          iCell.m_Edges.erase(edgeIter);
          //ioCellEdge.point[0] = ioCellEdge.point[1];
        }
        oBorders.push_back(std::make_pair(iCell.m_Id, oBorderEdge));
        return;
        //return true;
      }
      else
      {
        float dist3 = Mathf::FAbs(dot((borderEdgePt2 - cellEdgePt1), dir));
        float dist4 = Mathf::FAbs(dot((borderEdgePt2 - cellEdgePt2), dir));
        if (dist3 < dist4)
        {
          InsertAndAdjust<0, 0>(iCell, borderEdgeIter, edgeIter, iBorderEdge, oTwinMap, oBorders);
          return;
        }
        else
        {
          InsertAndAdjust<1, 0>(iCell, borderEdgeIter, edgeIter, iBorderEdge, oTwinMap, oBorders);
          return;
        }
      }
    }
    else
    {
    
      if (dist2 > -iEpsilon && dist2 - iEpsilon < cellEdgeLength)
      {
        float dist3 = Mathf::FAbs(dot((borderEdgePt1 - cellEdgePt1), dir));
        float dist4 = Mathf::FAbs(dot((borderEdgePt1 - cellEdgePt2), dir));
        if (dist3 < dist4)
        {
          //+ proche de cellEdgePt1
          InsertAndAdjust<0, 1>(iCell, borderEdgeIter, edgeIter, iBorderEdge, oTwinMap, oBorders);
          return;
        }
        else
        {
          InsertAndAdjust<1, 1>(iCell, borderEdgeIter, edgeIter, iBorderEdge, oTwinMap, oBorders);
          return;
        }
      }
    }
    std::pair<uint32_t, uint32_t> key;
    if (iCell.m_Id < edgeIter->first)
      key = std::make_pair(iCell.m_Id, edgeIter->first);
    else
      key = std::make_pair(edgeIter->first, iCell.m_Id);
    EdgeTwinMap::iterator iter = oTwinMap.find(key);
    if (iter == oTwinMap.end())
    {
      //BorderEdgeList::iterator newIter = oBorders.insert(oBorders.end(), std::make_pair(iCellId, oBorderEdge));
      BorderCut border = { edgeIter, borderEdgeIter != iCell.m_Edges.end() ? borderEdgeIter : edgeIter, -1, oBorders.end() };

      oTwinMap.insert(std::make_pair(key, border));
    }
    else
    {
      assert(iter->second.iterBorder != oBorders.end());
      CIPointRef extPoint = iter->second.iterBorder->second.point[1];
      assert(iter->second.commonPoint >= 0);
      iter->second.iterCell->second.point[iter->second.commonPoint & 1] = extPoint;
      //iter->second.iterCellBorder->second.point[(commonPoint >> 1) & 1] = edgeIter->second.point[iCellPoint];
      if (iter->second.iterCell != iter->second.iterCellBorder)
      {
        int commonPoint = edgeIter->second.CommonPoint(borderEdgeIter->second);//iter->second.commonPoint;
        assert(commonPoint >= 0);
        edgeIter->second.point[commonPoint & 1] = extPoint;
        borderEdgeIter->second.point[(commonPoint >> 1) & 1] = extPoint;
        oBorders.erase(iter->second.iterBorder);
        oTwinMap.erase(iter);
      }
    }

    //return false;
  }

  void VoronoiGraph::GetCells(Vector<Polygond>& oCells)
  {
    oCells.clear();

    if (m_Circles.empty()
      || m_AABB.Empty())
    {
      return;
    }

    Vec2 levelSize = m_AABB.GetSize();
    levelSize = levelSize /** 1.25*/;

    Vec2 gridOrig = m_AABB.GetCenter() - levelSize * 0.5f;

    uint32_t const baseSize = 128;
    Vec2i gridSize;
    if (levelSize.x > levelSize.y)
    {
      gridSize.x = baseSize + 1;
      gridSize.y = ((levelSize.y / levelSize.x) * baseSize) + 1;
    }
    else
    {
      gridSize.y = baseSize + 1;
      gridSize.x = ((levelSize.x / levelSize.y) * baseSize) + 1;
    }
    Vec2 gridInc(levelSize.x / (gridSize.x - 1), levelSize.y / (gridSize.y - 1));

    PointGrid grid(gridSize.x * gridSize.y);

    MakeDiagram();

    oCells.resize(m_Circles.size());

    voro::c_loop_all looper(*m_Diagram);

    if (looper.start())
    {
      voro::voronoicell_neighbor cell;
      voro::voronoicell_neighbor cell_comp;
      uint32_t loopStep = 0;
      do
      {
        if (m_Diagram->compute_cell(cell, looper))
        {
          Vec3d pos;
          looper.pos(pos.x, pos.y, pos.z);
          std::vector<double> vertices;
          cell.vertices(pos.x, pos.y, pos.z, vertices);
          Vector<Vec2d> pts;
          for(uint32_t i = 0; i<vertices.size() / 3; ++i)
          {
            pts.push_back(Vec2d(vertices[3*i + 0], vertices[3*i + 1]));
          }
          Polygond::ConvexHull(pts, oCells[looper.pid()]);
        }
      } while (looper.inc());
    }
  }

  void VoronoiGraph::BuildGraph(CellGraph& oGraph, bool iSmoothBorder) const
  {
    oGraph.m_CellGraph.clear();
    oGraph.m_Edges.clear();
    oGraph.m_Vertices.clear();

    if (m_Circles.empty()
      || m_AABB.Empty())
    {
      return;
    }

    Vec2 levelSize = m_AABB.GetSize();
    levelSize = levelSize /** 1.25*/;

    Vec2 gridOrig = m_AABB.GetCenter() - levelSize * 0.5f;

    uint32_t const baseSize = 128;
    Vec2i gridSize;
    if (levelSize.x > levelSize.y)
    {
      gridSize.x = baseSize + 1;
      gridSize.y = ((levelSize.y / levelSize.x) * baseSize) + 1;
    }
    else
    {
      gridSize.y = baseSize + 1;
      gridSize.x = ((levelSize.x / levelSize.y) * baseSize) + 1;
    }
    Vec2 gridInc(levelSize.x / (gridSize.x - 1), levelSize.y / (gridSize.y - 1));

    PointGrid grid(gridSize.x * gridSize.y);

    //ClearDiagram();
    MakeDiagram();

    //IPointSet intPoint;

    uint32_t pointAlloc = 0;
    WallMap additionalWalls;
    //Vector<int>      wallId;
    Vector<CellInfo> cells;
    cells.resize(m_Circles.size() + 1);
    Vector<uint32_t> mapping;
    mapping.resize(m_IdCounter);
    CellInfo& outCell = cells[m_Circles.size()];
    outCell.m_Id = -((int)m_Walls.size()) - 7;

    InitializeBorderPlanes(additionalWalls, *m_Diagram, cells, m_Circles, m_AABB.m_Data[0], mapping, iSmoothBorder);
  
    float invSqrt2 = 1.0 / Mathf::Sqrt(2.0);
    Vec2 octahedron[8] =
    {
      Vec2(1.0, 0.0),
      Vec2(-1.0, 0.0),
      Vec2(0.0, 1.0),
      Vec2(0.0, -1.0),
      Vec2(invSqrt2, invSqrt2),
      Vec2(-invSqrt2, invSqrt2),
      Vec2(invSqrt2, -invSqrt2),
      Vec2(-invSqrt2, -invSqrt2)
    };

    std::multimap<int, std::pair<int, CIEdge> > borderEdgeMap;
    std::vector<double> vertices;
    std::vector<int> faceO;
    std::vector<int> faceV;
    std::vector<int> neighs;
    std::list<std::pair<uint32_t, CIEdge> > borderEdges;

    bool needLoop = true;
    while (needLoop)
    {
      borderEdges.clear();
      borderEdgeMap.clear();
      for (uint32_t numCell = 0; numCell < cells.size(); numCell++)
      {
        cells[numCell].m_Edges.clear();
      }
      for (uint32_t i = 0; i < grid.size(); ++i)
      {
        grid[i].clear();
      }

      voro::c_loop_all looper(*m_Diagram);

      if (looper.start())
      {
        voro::voronoicell_neighbor cell;
        voro::voronoicell_neighbor cell_comp;
        uint32_t loopStep = 0;
        do
        {
          {
            if (m_Diagram->compute_cell(cell, looper))
              //voro::voronoicell_neighbor& cell = cells[numCell].m_VoroCell;
            {
              //CellInfo& newCell = cells[numCell];
              CellInfo& newCell = cells[loopStep];
              int pid = looper.pid();
              if (!newCell.m_Initialized)
                continue;

              //float dist = newCell.m_Radius * 8;
              //for (uint32_t fNum = 0; fNum < 8; ++fNum)
              //{
              //  Vec2 cellPos(newCell.origPos[0], newCell.origPos[1]);
              //  Vec2 fCenter = cellPos + octahedron[fNum] * dist;
              //  cell.nplane(octahedron[fNum].x, octahedron[fNum].y, 0.0, dist * 2, outCell.m_Id);
              //}

              WallMap::iterator iter = additionalWalls.find(looper.pid());
              if (iter != additionalWalls.end())
              {
                cell_comp = cell;
              
                Vec2d const& point = iter->second;
                Vec2d normalVect = point - Vec2d(newCell.origPos[0], newCell.origPos[1]);
              
                double dq = 2 * NormalizeAndGetLength(normalVect);
                cell.nplane(normalVect.x, normalVect.y, 0.0, dq, outCell.m_Id);
                cell_comp.translate(-normalVect.x * dq, -normalVect.y * dq, 0.0);
                cell_comp.nplane(-normalVect.x, -normalVect.y, 0.0, 0.0, outCell.m_Id);
                cell_comp.translate(normalVect.x * dq, normalVect.y * dq, 0.0);

                vertices.clear();
                faceO.clear();
                faceV.clear();
                neighs.clear();

                cell_comp.vertices(newCell.origPos[0], newCell.origPos[1] , newCell.origPos[2], vertices);
                cell_comp.face_orders(faceO);
                cell_comp.face_vertices(faceV);
                cell_comp.neighbors(neighs);
                uint32_t offsetI = 1;
                //bool foundBorder1 = false;
                //Vec2d ptBorder1;
                //Vec2d ptBorder2;
                //
                //for (uint32_t i = 0; i < neighs.size(); ++i)
                //{
                //  if (neighs[i] < 0 && neighs[i] > -5)
                //  {
                //    Vec2d pt1;
                //    Vec2d pt2;
                //    GetEdgeFromFaces(vertices, faceO, faceV, i, offsetI, pt1, pt2);
                //    if (foundBorder1)
                //    {
                //      //Follow corners
                //      if      (ptBorder1 == pt1)ptBorder1 = pt2;
                //      else if (ptBorder1 == pt2)ptBorder1 = pt1;
                //      else if (ptBorder2 == pt1)ptBorder2 = pt2;
                //      else if (ptBorder2 == pt2)ptBorder2 = pt1;
                //      break;
                //    }
                //    else
                //    {
                //      ptBorder1 = pt1;
                //      ptBorder2 = pt2;
                //      foundBorder1 = true;
                //      break;
                //    }
                //  }
                //  offsetI += faceO[i] + 1;
                //}
                //if (foundBorder1)
                {
                
                  offsetI = 1;
                  for (uint32_t i = 0; i < neighs.size(); ++i)
                  {
                    if (neighs[i] >= 0)
                    {
                      Vec2d pt1;
                      Vec2d pt2;
                      GetEdgeFromFaces(vertices, faceO, faceV, i, offsetI, pt1, pt2);
                      //if (!foundBorder1 ||
                      //  (pt1 != ptBorder1 && pt1 != ptBorder2
                      //  && pt2 != ptBorder1 && pt2 != ptBorder2))
                      {
                      
                        int curNeigh = neighs[i];
                        //if (curNeigh >= 0 || (curNeigh < -6 /*&& curNeigh != outCell.m_Id*/))
                        {
                          Vec2 relPt1 = Vec2(pt1.x - gridOrig.x, pt1.y - gridOrig.y);
                          Vec2 relPt2 = Vec2(pt2.x - gridOrig.x, pt2.y - gridOrig.y);
                          CIPointRef pt1Ref = InsertPoint(grid, relPt1, gridInc, gridSize, m_Epsilon, pointAlloc);
                          CIPointRef pt2Ref = InsertPoint(grid, relPt2, gridInc, gridSize, m_Epsilon, pointAlloc);
                          if (pt1Ref != pt2Ref)
                          {
                            CIEdge newEdge;
                            newEdge.point[0] = pt1Ref;
                            newEdge.point[1] = pt2Ref;
                            Particle const& neighPart = *m_Circles[pid];
                            borderEdgeMap.insert(std::make_pair(curNeigh, std::make_pair(neighPart.GetId(), newEdge)));
                          }
                        }
                      }
                    }
                    offsetI += faceO[i] + 1;
                  }
                }
              }
              vertices.clear();
              faceO.clear();
              faceV.clear();
              neighs.clear();
            
              cell.vertices(newCell.origPos[0], newCell.origPos[1], newCell.origPos[2], vertices);
              cell.face_orders(faceO);
              cell.face_vertices(faceV);
              cell.neighbors(neighs);


              assert(neighs.size() == faceO.size());
              uint32_t offsetI = 1;
              unsigned char handledWallMask = 0;
              for (uint32_t i = 0; i < neighs.size(); ++i)
              {
                if (neighs[i] > -5 || neighs[i] < -6)
                {
                  Vec2d pt1;
                  Vec2d pt2;
                  GetEdgeFromFaces(vertices, faceO, faceV, i, offsetI, pt1, pt2);

                  int curNeigh = neighs[i];
                  //if (curNeigh >= 0 || (curNeigh < -6 /*&& curNeigh != outCell.m_Id*/))
                  {
                    Vec2 relPt1 = Vec2(pt1.x - gridOrig.x, pt1.y - gridOrig.y);
                    Vec2 relPt2 = Vec2(pt2.x - gridOrig.x, pt2.y - gridOrig.y);
                    CIPointRef pt1Ref = InsertPoint(grid, relPt1, gridInc, gridSize, m_Epsilon, pointAlloc);
                    CIPointRef pt2Ref = InsertPoint(grid, relPt2, gridInc, gridSize, m_Epsilon, pointAlloc);
                    if (pt1Ref != pt2Ref)
                    {
                      CIEdge newEdge;
                      newEdge.point[0] = pt1Ref;
                      newEdge.point[1] = pt2Ref;

                      //pt1Ref.listIter->cellRef++;
                      //pt2Ref.listIter->cellRef++;
                      int partId;
                      if (curNeigh >= 0 || (curNeigh < -6 && curNeigh != outCell.m_Id))
                      {
                        Particle const& neighPart = curNeigh >= 0 ? static_cast<Particle const&>(*m_Circles[curNeigh]) : static_cast<Particle const&>(*m_Walls[-(curNeigh + 7)]);
                        if (curNeigh < 0)
                        {
                          //pt1Ref.listIter->cellRef++;
                          //pt2Ref.listIter->cellRef++;
                        }
                        partId = neighPart.GetId();
                      }
                      else
                      {
                        newEdge.borderEdge = true;
                        partId = curNeigh;
                      }
                      std::pair<CellEdgeMap::iterator, bool> res = newCell.m_Edges.insert(std::make_pair(partId, newEdge));
                      assert(res.second);
                      //pt1Ref.listIter->outEdges.push_back(res.first);
                      //pt1Ref.listIter->outEdges.push_back(res.first);
                    }
                  }
                }
                offsetI += faceO[i] + 1;
              }
            }
          }
          ++loopStep;
        } while (looper.inc());
      }

      needLoop = false;

      if(iSmoothBorder)
      {
        for (uint32_t i = 0; i < cells.size(); ++i)
        {
          CellInfo& curCell = cells[i];
          uint32_t numBorders = 0;
          //CellEdgeMap::iterator curCellBorder[2] = { curCell.m_Edges.end(), curCell.m_Edges.end() };
          CIEdge curCellBorder[2];
          std::multimap<int, std::pair<int, CIEdge> >::iterator iter, iterEnd;
          for (boost::tie(iter,iterEnd) = borderEdgeMap.equal_range(curCell.m_DiagPid); iter != iterEnd; ++iter)
          {
            CellEdgeMap::iterator iterEdge = curCell.m_Edges.find(iter->second.first);
            if (iterEdge != curCell.m_Edges.end())
            {
              if (numBorders < 2)
              {
                //curCellBorder[numBorders] = iterEdge;
                curCellBorder[numBorders] = iter->second.second;
              }
              ++numBorders;
            }
          }
          if (numBorders == 2
          && additionalWalls.find(curCell.m_DiagPid) == additionalWalls.end())
          {
            Vec2 commonPt;
            Vec2 extPts[2];
            bool foundCommon = false;
            if (curCellBorder[0].point[0] == curCellBorder[1].point[0])
            { 
              foundCommon = true; 
              commonPt =  curCellBorder[0].point[0].listIter->m_Pos;
              extPts[0] = curCellBorder[0].point[1].listIter->m_Pos;
              extPts[1] = curCellBorder[1].point[1].listIter->m_Pos;
            }
            if (curCellBorder[0].point[0] == curCellBorder[1].point[1])
            {
              foundCommon = true;
              commonPt =  curCellBorder[0].point[0].listIter->m_Pos;
              extPts[0] = curCellBorder[0].point[1].listIter->m_Pos;
              extPts[1] = curCellBorder[1].point[0].listIter->m_Pos;
            }
            if (curCellBorder[0].point[1] == curCellBorder[1].point[0])
            {
              foundCommon = true;
              commonPt =  curCellBorder[0].point[1].listIter->m_Pos;
              extPts[0] = curCellBorder[0].point[0].listIter->m_Pos;
              extPts[1] = curCellBorder[1].point[1].listIter->m_Pos;
            }
            if (curCellBorder[0].point[1] == curCellBorder[1].point[1])
            {
              foundCommon = true;
              commonPt =  curCellBorder[0].point[1].listIter->m_Pos;
              extPts[0] = curCellBorder[0].point[0].listIter->m_Pos;
              extPts[1] = curCellBorder[1].point[0].listIter->m_Pos;
            }
            if (foundCommon)
            {
              if (additionalWalls.find(curCell.m_DiagPid) == additionalWalls.end())
              {
                commonPt += m_AABB.m_Data[0];
                extPts[0] += m_AABB.m_Data[0];
                extPts[1] += m_AABB.m_Data[0];

                Vec2d cellPos(curCell.origPos[0], curCell.origPos[1]);
                needLoop = true;
                Vec2d dir = Vec2d(commonPt.x - cellPos.x, commonPt.y - cellPos.y);
                dir = normalize(dir);
                Vec2d normalVect(-dir.y, dir.x);

                float currentDist = curCell.m_Radius;
                Vec2d oPoint = cellPos + dir * static_cast<double>(currentDist);

                Vec2d edge1Pt1(commonPt.x, commonPt.y);
                Vec2d edge1Pt2(extPts[0].x, extPts[0].y);
                Vec2d edge2Pt1(commonPt.x, commonPt.y);
                Vec2d edge2Pt2(extPts[1].x, extPts[1].y);

                Vec2d posOnSeg1;
                Vec2d posOnSeg2;

                Segmentd::NearestPointOnSeg1(edge1Pt1, edge1Pt2, oPoint, oPoint + normalVect, posOnSeg1);
                Segmentd::NearestPointOnSeg1(edge2Pt1, edge2Pt2, oPoint, oPoint + normalVect, posOnSeg2);
                float newDist;
                if ((newDist = dot((posOnSeg1 - cellPos), dir)) > currentDist)
                  currentDist = newDist;
                if ((newDist = dot((posOnSeg2 - cellPos), dir)) > currentDist)
                  currentDist = newDist;
                oPoint = cellPos + dir * static_cast<double>(currentDist);

                additionalWalls.insert(std::make_pair(curCell.m_DiagPid, oPoint));
              }
            }
          }
        }
      }
    }

    EdgeTwinMap twinMap;

    for (uint32_t i = 0; i < cells.size(); ++i)
    {
      CellInfo& curCell = cells[i];
      std::multimap<int, std::pair<int, CIEdge> >::iterator iter, iterEnd;
      for (boost::tie(iter, iterEnd) = borderEdgeMap.equal_range(curCell.m_DiagPid); iter != iterEnd; ++iter)
      {
        CellEdgeMap::iterator iterEdge = curCell.m_Edges.find(iter->second.first);
        if (iterEdge != curCell.m_Edges.end())
        {
          CIEdge cellEdge = iterEdge->second;
          CIEdge borderEdge = iter->second.second;
        
          CellEdgeMap::iterator cellBorderIter = FindCellConnectedWith(curCell.m_Edges, iterEdge, iterEdge->second.point[0]);
          if (cellBorderIter == curCell.m_Edges.end() || cellBorderIter->first > 0)
          {
            cellBorderIter = FindCellConnectedWith(curCell.m_Edges, iterEdge, iterEdge->second.point[1]);
          }
          if (cellBorderIter != curCell.m_Edges.end() && cellBorderIter->first > 0)
          {
            cellBorderIter = curCell.m_Edges.end();
          }
        
          SplitCellEdgeBorder(curCell, curCell.m_Edges.begin(), iterEdge, iter->second.second, twinMap, borderEdges, m_Epsilon);
        }
      }
    }

    //back to positive
    outCell.m_Id = m_IdCounter;

    EdgeUniqMap uniqMap;
    for (uint32_t i = 0; i < cells.size(); ++i)
    {
      CellInfo& curCell = cells[i];
      for (CellEdgeMap::iterator iter = curCell.m_Edges.begin(), iterEnd = curCell.m_Edges.end(); iter != iterEnd; ++iter)
      {
        int neighCell = iter->first;
        if (neighCell < 0)
        {
          borderEdges.push_back(std::make_pair(curCell.m_Id, iter->second));
        }
        else
        {
          std::pair<EdgeUniqMap::iterator, bool> res;
          if (curCell.m_Id < neighCell)
          {
            res = uniqMap.insert(std::make_pair(std::make_pair(curCell.m_Id, neighCell), iter->second));
          }
          else
          {
            res = uniqMap.insert(std::make_pair(std::make_pair(neighCell, curCell.m_Id), iter->second));
          }
          if (!res.second)
          {
          
            Vec2 points[4] =
            { res.first->second.point[0].listIter->m_Pos,
              res.first->second.point[1].listIter->m_Pos,
              iter->second.point[0].listIter->m_Pos,
              iter->second.point[1].listIter->m_Pos };
          
            float lengths[5] = {
              length(points[0] - points[1]),
              length(points[0] - points[2]),
              length(points[0] - points[3]),
              length(points[1] - points[2]),
              length(points[1] - points[3]) };
          
            uint32_t curMax = 0;
            for (uint32_t k = 1; k < 5; ++k)
            {
              if (lengths[k] > lengths[0])
              {
                curMax = k;
                lengths[0] = lengths[k];
              }
            }
            switch (curMax)
            {
            case 1:
              res.first->second.point[1] = iter->second.point[0];
              break;
            case 2:
              res.first->second.point[1] = iter->second.point[1];
              break;
            case 3:
              res.first->second.point[0] = iter->second.point[0];
              break;
            case 4:
              res.first->second.point[0] = iter->second.point[1];
              break;
            }
          }
        }
      }
    }

    oGraph.m_Vertices.resize(pointAlloc);
    for (uint32_t i = 0; i < grid.size(); ++i)
    {
      for (std::list<GridPoint>::const_iterator iter = grid[i].begin(), iterEnd = grid[i].end(); iter != iterEnd; ++iter)
      {
        oGraph.m_Vertices[iter->m_Identifier].m_Position = iter->m_Pos + gridOrig;
      }
    }

    //boost::property_map<CellGraphImpl, boost::vertex_name_t>::type& propMapV = boost::get(boost::vertex_name, oGraph.m_CellGraph);
    boost::property_map<CellGraphImpl, boost::edge_name_t>::type propMapE = boost::get(boost::edge_name, oGraph.m_CellGraph);
    boost::property_map<CellGraphImpl, boost::edge_weight_t>::type propMapW = boost::get(boost::edge_weight, oGraph.m_CellGraph);
  
    Vector<CellGraphImpl::vertex_descriptor> vList;

    for (uint32_t i = 0; i < m_IdCounter/*cells.size()*/; ++i)
    {
      CellGraphImpl::vertex_descriptor desc = boost::add_vertex(oGraph.m_CellGraph);
      vList.push_back(desc);
    }
    CellGraphImpl::vertex_descriptor desc = boost::add_vertex(oGraph.m_CellGraph);
    vList.push_back(desc);
  
    uint32_t edgeNum = 0;
    oGraph.m_Edges.reserve(uniqMap.size() + borderEdges.size());
    Vector<CellGraphImpl::edge_descriptor> eList;
    eList.reserve(oGraph.m_Edges.size());
    for (EdgeUniqMap::iterator iter = uniqMap.begin(),iterEnd = uniqMap.end(); iter != iterEnd; ++iter)
    {
      CellGraphImpl::edge_descriptor edgeDesc;
      bool inserted;
      boost::tie(edgeDesc, inserted) = boost::add_edge(vList[iter->first.first], vList[iter->first.second], oGraph.m_CellGraph);
  
      Edge newEdge;
      newEdge.m_Pt1 = iter->second.point[0].listIter->m_Identifier;
      newEdge.m_Pt2 = iter->second.point[1].listIter->m_Identifier;
      propMapE[edgeDesc] = eList.size();

      Particle const* part1 = GetParticle(iter->first.first);
      Particle const* part2 = GetParticle(iter->first.second);
      propMapW[edgeDesc] = length(part2->GetPos() - part1->GetPos());

      oGraph.m_Edges[edgeDesc] = newEdge;
      eList.push_back(edgeDesc);
    }

    for (BorderEdgeList::iterator iter = borderEdges.begin(), iterEnd = borderEdges.end(); iter != iterEnd; ++iter)
    {
      CellGraphImpl::edge_descriptor edgeDesc;
      bool inserted;
      boost::tie(edgeDesc, inserted) = boost::add_edge(vList[iter->first], vList[outCell.m_Id], oGraph.m_CellGraph);
  
      Edge newEdge;
      newEdge.m_Pt1 = iter->second.point[0].listIter->m_Identifier;
      newEdge.m_Pt2 = iter->second.point[1].listIter->m_Identifier;
      propMapE[edgeDesc] = eList.size();
      propMapW[edgeDesc] = Mathf::MaxReal();
      oGraph.m_Edges[edgeDesc] = newEdge;
      eList.push_back(edgeDesc);
    }
  }
}