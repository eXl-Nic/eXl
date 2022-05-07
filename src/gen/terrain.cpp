#include <gen/terrain.hpp>

#include <boost/graph/astar_search.hpp>
#include <gen/graphutils.hpp>

#include <gen/poissonsampling.hpp>
#include <math/mathtools.hpp>

#include <armadillo>

namespace eXl
{

  class ArmaLaplaceMatrix : public LaplaceMatrix
  {
  public:
    arma::sp_mat m_Matrix;
  };

  struct found_goal{};


  template <class Vertex>
  class astar_goal_visitor : public boost::default_astar_visitor
  {
  public:
    inline astar_goal_visitor(Vertex goal) : m_goal(goal) {}
    template <class Graph>
    void examine_vertex(Vertex u, Graph& g) {
      if(u == m_goal)
        throw found_goal();
    }
  private:
    Vertex m_goal;
  };

  template<typename PosMap>
  class VoroDiagGraphDistance : public boost::astar_heuristic<VoronoiGraph::CellGraphImpl, float>
  {
  public:
    typedef boost::graph_traits<VoronoiGraph::CellGraphImpl>::vertex_descriptor Vertex;
    VoroDiagGraphDistance(PosMap const& iCenters, Vertex iGoal, TIndexMap<VoronoiGraph::CellGraphImpl> const& iIndex)
      : m_CellCenter(iCenters), m_Goal(iGoal), m_Index(iIndex) {}
    float operator()(Vertex u)
    {
      unsigned int goalIdx = boost::get(m_Index, m_Goal);
      unsigned int idx = boost::get(m_Index, u);

      Vec2 goalPos = m_CellCenter(goalIdx);
      Vec2 curPos = m_CellCenter(idx);

      return (curPos - goalPos).Length();
    }
  private:
    PosMap const& m_CellCenter;
    TIndexMap<VoronoiGraph::CellGraphImpl> const& m_Index;
    Vertex m_Goal;
  };

  template<typename PosMap>
  void FindPath(VoronoiGraph::CellGraph const& iGraph,
    PosMap const& iCellCenters,
    VoronoiGraph::CellGraphImpl::vertex_descriptor start,
    VoronoiGraph::CellGraphImpl::vertex_descriptor goal,
    Vector<VoronoiGraph::CellGraphImpl::vertex_descriptor>& oPath)
  {
    IndexMap<VoronoiGraph::CellGraphImpl> index(iGraph.m_CellGraph);
    Vector<VoronoiGraph::CellGraphImpl::vertex_descriptor> p(boost::num_vertices(iGraph.m_CellGraph), start);
    Vector<float> d(boost::num_vertices(iGraph.m_CellGraph));
    bool found = false;
    try 
    {
      // call astar named parameter interface
      boost::astar_search_tree
      (iGraph.m_CellGraph, start,
        VoroDiagGraphDistance<PosMap>(iCellCenters, goal, index),
        //iFactory.Make<boost::filtered_graph<OutGraph, boost::keep_all, TagFilter> >(filteredGr, goal),
        boost::predecessor_map(boost::make_iterator_property_map(p.begin(), index)).
        distance_map(make_iterator_property_map(d.begin(), index)).
        visitor(astar_goal_visitor<VoronoiGraph::CellGraphImpl::vertex_descriptor>(goal)));
    } 
    catch(found_goal ) 
    { // found a path to the goal
      Vector<VoronoiGraph::CellGraphImpl::vertex_descriptor> path;
      for(auto v = goal;; v = p[v]) 
      {
        path.push_back(v);

        if(p[v] == v)
          break;
      }
      oPath.swap(path);
      found = true;
    }
  }

  struct TerrainPosMap
  {
    TerrainPosMap(Vector<Terrain::CellProperties> const& iCells)
      :m_Cells(iCells){}

    Vec2 const& operator()(unsigned int i) const {return m_Cells[i].position;}

    Vector<Terrain::CellProperties> const& m_Cells;
  };

  Terrain::Terrain()
  {
  }
  
  void Terrain::MakeGrid(Vec2 const& iSize, Vec2i const& iGridSize)
  {
    m_Size = iSize;
    
    Vec2 gridStep(iSize.x / iGridSize.x, iSize.y / iGridSize.y);

    m_QueryBox = AABB2Df::FromMinAndSize(-gridStep * 0.51, gridStep * 1.02);

    m_Cells.clear();
    m_Graph.m_CellGraph.clear();
    m_Graph.m_Edges.clear();
    m_Graph.m_Vertices.clear();
    m_Neigh.clear();
    m_Index.clear();

    uint32_t totNumCells = iGridSize.x * iGridSize.y;

    m_Cells.reserve(totNumCells);
    m_Neigh.reserve(totNumCells * 4);
    for (uint32_t i = 0; i < totNumCells; ++i)
    {
      boost::add_vertex(m_Graph.m_CellGraph);
    }
    
    for (int32_t y = 0; y < iGridSize.y + 1; ++y)
    {
      for (int32_t x = 0; x < iGridSize.x + 1; ++x)
      {
        VoronoiGraph::Vertex gridVtx;
        gridVtx.m_Position = Vec2(gridStep.x * x, gridStep.y * y);
        m_Graph.m_Vertices.push_back(gridVtx);
      }
    }

    uint32_t cellOffset = 0;
    Vector<CellLoc> locs;
    locs.reserve(totNumCells);
    for (int32_t y = 0; y < iGridSize.y; ++y)
    {
      for (int32_t x = 0; x < iGridSize.x; ++x)
      {
        uint32_t gridLowLeftCorner = y * (iGridSize.x + 1) + x;

        CellProperties cell;
        cell.neighCount = 0;
        cell.position = Vec2(((float)x + 0.5) * gridStep.x, ((float)y + 0.5) * gridStep.y);
        cell.neighStart = m_Neigh.size();

        Vec2 const& cellPos = cell.position;
        locs.push_back(std::make_pair(AABB2Df::FromMinAndSize(cellPos - gridStep * 0.5, gridStep), cellOffset));

        if (x > 0)
        {
          cell.neighCount++;
          m_Neigh.push_back(cellOffset - 1);
        //  auto edgeDesc = boost::add_edge(cellOffset, cellOffset - 1, m_Graph.m_CellGraph);
        //  VoronoiGraph::Edge newEdge;
        //  newEdge.m_Pt1 = gridLowLeftCorner;
        //  newEdge.m_Pt2 = gridLowLeftCorner + iGridSize.x + 1;
        //  m_Graph.m_Edges.insert(std::make_pair(edgeDesc, newEdge));
        }

        if (x < iGridSize.x - 1)
        {
          cell.neighCount++;
          m_Neigh.push_back(cellOffset + 1);
          auto edgeDesc = boost::add_edge(cellOffset, cellOffset + 1, m_Graph.m_CellGraph).first;
          VoronoiGraph::Edge newEdge;
          newEdge.m_Pt1 = gridLowLeftCorner + 1;
          newEdge.m_Pt2 = gridLowLeftCorner + 1 + iGridSize.x + 1;
          m_Graph.m_Edges[edgeDesc] = newEdge;
        }

        if (y > 0)
        {
          cell.neighCount++;
          m_Neigh.push_back(cellOffset - iGridSize.x);
        //  auto edgeDesc = boost::add_edge(cellOffset, cellOffset - iGridSize.x, m_Graph.m_CellGraph);
        //  VoronoiGraph::Edge newEdge;
        //  newEdge.m_Pt1 = gridLowLeftCorner;
        //  newEdge.m_Pt2 = gridLowLeftCorner + 1;
        //  m_Graph.m_Edges.insert(std::make_pair(edgeDesc, newEdge));
        }

        if (y < iGridSize.y - 1)
        {
          cell.neighCount++;
          m_Neigh.push_back(cellOffset + iGridSize.x);

          auto edgeDesc = boost::add_edge(cellOffset, cellOffset + iGridSize.x, m_Graph.m_CellGraph).first;
          VoronoiGraph::Edge newEdge;
          newEdge.m_Pt1 = gridLowLeftCorner + iGridSize.x + 1;
          newEdge.m_Pt2 = gridLowLeftCorner + 1 + iGridSize.x + 1;
          m_Graph.m_Edges[edgeDesc] = newEdge;
        }
        m_Cells.push_back(cell);
        ++cellOffset;
      }
    }

    m_Index = CellIndex(locs);
  }

  void Terrain::MakeCirclePacking(Random& iRand, Vec2 const& iSize, float iCellSize)
  {
    m_Size = iSize;

    m_QueryBox = AABB2Df(-iCellSize * 1.1, -iCellSize * 1.1, iCellSize * 1.1, iCellSize * 1.1);

    m_Cells.clear();
    m_Graph.m_CellGraph.clear();
    m_Graph.m_Edges.clear();
    m_Graph.m_Vertices.clear();
    m_Neigh.clear();
    m_Index.clear();

    Polygoni poly(AABB2Di::FromMinAndSize(Zero<Vec2i>(), Vec2i(Mathf::Ceil(m_Size.x), Mathf::Ceil(m_Size.y))));
    PoissonDiskSampling sampler(poly, iRand);

    sampler.Sample(iCellSize, iCellSize);

    Vector<Vec2d> samples;
    Vector<Vec2> cellCenter;
    sampler.GetLayer(0, samples);
    //sampler.GetLayer(1, samples[1]);
    //sampler.GetLayer(2, samples[2]);

    unsigned int const totSamples = samples.size() /*+ samples[1].size() + samples[2].size()*/;

    VoronoiGraph voroGr;

    for(unsigned int j = 0; j<samples.size(); ++j)
    {
      voroGr.AddCircle(iCellSize, MathTools::ToFVec(samples[j]));
      cellCenter.push_back(MathTools::ToFVec(samples[j]));

      CellProperties props;
      props.position = cellCenter.back();
      m_Cells.push_back(props);
    }

    voroGr.BuildGraph(m_Graph, true);


    {
      Vector<CellLoc> locs(cellCenter.size());
      for(unsigned int i = 0; i<cellCenter.size(); ++i)
      {
        Vec2 const& cellPos = cellCenter[i];
        locs.push_back(std::make_pair(AABB2Df::FromMinAndSize(cellPos - One<Vec2>() * iCellSize, One<Vec2>() * 2 * iCellSize), i));
      }
      m_Index = CellIndex(locs);
    }

    unsigned int const numCells = m_Cells.size();
    Vector<unsigned int> sortedPoints;

    for (unsigned int i = 0; i < numCells; ++i)
    {
      m_Cells[i].neighCount = 0;
      sortedPoints.clear();
      Vec2 center = m_Cells[i].position;

      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
      unsigned int numNeigh = std::distance(edgesBegin, edgesEnd);
      if(edgesBegin != edgesEnd)
      {
        unsigned int firstPoint = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        VoronoiGraph::Edge firstEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
        ++edgesBegin;
        while(firstPoint == numCells)
        {
          --numNeigh;
          firstPoint = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
          firstEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
          ++edgesBegin;
        }

        Segmentf::SortByAngle sortMeth(m_Cells[firstPoint].position);
        std::map<Vec2, unsigned int, Segmentf::SortByAngle > sortedPointsMap(sortMeth);

        for (; edgesBegin != edgesEnd; ++edgesBegin)
        {
          unsigned int target = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
          if(target == numCells)
          {
            --numNeigh;
            continue;
          }
          sortedPointsMap.insert(std::make_pair(m_Cells[target].position, target));
        }
        sortedPoints.push_back(firstPoint);
        for(auto pair : sortedPointsMap)
        {
          sortedPoints.push_back(pair.second);
        }

        m_Cells[i].neighStart = m_Neigh.size();

        for(uint32_t curNeighIdx = 0; curNeighIdx < numNeigh; ++curNeighIdx)
        {
          unsigned int curNeigh = sortedPoints[curNeighIdx];
          
          m_Neigh.push_back(curNeigh);
        }

        m_Cells[i].neighCount = numNeigh;
      }
    }
  }

  bool Terrain::GetClosestCell(Vec2 const& iPos, uint32_t& oCellIdx)
  {
    AABB2Df queryBox = m_QueryBox;
    queryBox.m_Data[0] += iPos;
    queryBox.m_Data[1] += iPos;

    Vector<CellLoc> results;
    m_Index.query(boost::geometry::index::intersects(queryBox), std::back_inserter(results));

    if (results.empty())
    {
      return false;
    }

    float minDist = FLT_MAX;
    for (auto const& cellLoc : results)
    {
      CellProperties const& cell = m_Cells[cellLoc.second];
      float const distSq = distance2(cell.position , iPos);
      if (minDist > distSq)
      {
        oCellIdx = cellLoc.second;
        minDist = distSq;
      }
    }

    return true;
  }

  void Terrain::GetLineCells(Segmentf const& iSeg, float iRadius, Vector<unsigned int>& oCells)
  {
    oCells.clear();

    AABB2Df queryBox;
    queryBox.m_Data[0] = iSeg.m_Ext1;
    queryBox.m_Data[1] = iSeg.m_Ext1;
    queryBox.Absorb(iSeg.m_Ext2);
    queryBox.m_Data[0] -= One<Vec2>() * iRadius;
    queryBox.m_Data[1] += One<Vec2>() * iRadius;

    Vector<CellLoc> results;
    m_Index.query(boost::geometry::index::intersects(queryBox), std::back_inserter(results));

    for (auto const& cellLoc : results)
    {
      CellProperties const& cell = m_Cells[cellLoc.second];

      Vec2 dir;
      float dist = iSeg.NearestPointSeg(cell.position, dir);
      if (dist < iRadius)
      {
        oCells.push_back(cellLoc.second);
      }
    }
  }

  void Terrain::GetDiskCells(Vec2 const& iCenter, float iRadius, Vector<unsigned int>& oCells)
  {
    oCells.clear();

    AABB2Df queryBox = AABB2Df::FromMinAndSize(iCenter - iRadius * One<Vec2>(), One<Vec2>() * 2 * iRadius);

    Vector<CellLoc> results;
    m_Index.query(boost::geometry::index::intersects(queryBox), std::back_inserter(results));

    for(auto cell : results)
    {
      if(distance(m_Cells[cell.second].position, iCenter) < iRadius)
      {
        oCells.push_back(cell.second);
      }
    }
  }

  void Terrain::BuildLaplaceMatrix(ConductivityGetter& iConductivity, LaplaceMatrix*& oMat)
  {
    unsigned int const numCells = m_Cells.size();

    if (oMat != nullptr)
    {
      delete oMat;
    }

    ArmaLaplaceMatrix* matrix = eXl_NEW(ArmaLaplaceMatrix);

    oMat = matrix;

    arma::sp_mat& mat = matrix->m_Matrix;

    mat.set_size(numCells, numCells);

    //Vector<unsigned int> sortedPoints;
    //Vector<Vec2> cellPoints;

    for (unsigned int i = 0; i < numCells; ++i)
    {
      //cellPoints.clear();
      //sortedPoints.clear();
      float sum = 0.0;

      Vec2 center = m_Cells[i].position;

      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
      
      for (; edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int target = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if (target == numCells)
        {
          continue;
        }
        VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
        
        float edgeLen = distance(m_Graph.m_Vertices[curEdge.m_Pt2].m_Position, m_Graph.m_Vertices[curEdge.m_Pt1].m_Position);
        float cellDist = distance(center, m_Cells[target].position);

        float curConnectionValue = iConductivity(i, target, edgeLen, cellDist);
        sum -= curConnectionValue;

        mat.at(i, target) = 0.5 * curConnectionValue;
      }

      mat.at(i, i) = 0.5 * sum;
    }
  }

  void Terrain::BuildSmoothingMatrix(LaplaceMatrix*& oMat)
  {
    unsigned int const numCells = m_Cells.size();

    if (oMat != nullptr)
    {
      delete oMat;
    }

    ArmaLaplaceMatrix* matrix = eXl_NEW(ArmaLaplaceMatrix);

    oMat = matrix;

    arma::sp_mat& mat = matrix->m_Matrix;

    mat.set_size(numCells, numCells);

    Vector<unsigned int> sortedPoints;
    Vector<Vec2> cellPoints;

    for (unsigned int i = 0; i < numCells; ++i)
    {
      cellPoints.clear();
      sortedPoints.clear();
      Vec2 center = m_Cells[i].position;
      
      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
      unsigned int numNeigh = std::distance(edgesBegin, edgesEnd);
      if(edgesBegin != edgesEnd)
      {
        unsigned int firstPoint = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        VoronoiGraph::Edge firstEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
        ++edgesBegin;
        while(firstPoint == numCells)
        {
          --numNeigh;
          firstPoint = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
          firstEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
          ++edgesBegin;
        }
          
        Segmentf::SortByAngle sortMeth(m_Cells[firstPoint].position);
        std::map<Vec2, unsigned int, Segmentf::SortByAngle > sortedPointsMap(sortMeth);

        cellPoints.push_back(m_Graph.m_Vertices[firstEdge.m_Pt1].m_Position);
        cellPoints.push_back(m_Graph.m_Vertices[firstEdge.m_Pt2].m_Position);

        for (; edgesBegin != edgesEnd; ++edgesBegin)
        {
          unsigned int target = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
          if(target == numCells)
          {
            --numNeigh;
            continue;
          }
          VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
          cellPoints.push_back(m_Graph.m_Vertices[curEdge.m_Pt1].m_Position);
          cellPoints.push_back(m_Graph.m_Vertices[curEdge.m_Pt2].m_Position);
          sortedPointsMap.insert(std::make_pair(m_Cells[target].position, target));
        }
        sortedPoints.push_back(firstPoint);
        for(auto pair : sortedPointsMap)
        {
          sortedPoints.push_back(pair.second);
        }

        Polygonf cellPoly;
        Polygonf::ConvexHull(cellPoints, cellPoly);

        float area = cellPoly.Area();
        float sum = 0.0;

        for(uint32_t curNeighIdx = 0; curNeighIdx < numNeigh; ++curNeighIdx)
        {
          float alphaI = 0.0;
          float betaI = 0.0;

          unsigned int curNeigh = sortedPoints[curNeighIdx];
          unsigned int prevNeigh = sortedPoints[curNeighIdx == 0 ? numNeigh - 1 : curNeighIdx - 1];
          unsigned int nextNeigh = sortedPoints[curNeighIdx == numNeigh - 1 ? 0 : curNeighIdx + 1];

          Vec2 next1 = m_Cells[curNeigh].position - m_Cells[nextNeigh].position;
          Vec2 next2 = center - m_Cells[nextNeigh].position;

          float crossA = Segmentf::Cross(next1, next2);
          if(Mathf::Abs(crossA) > Mathf::Epsilon())
          {
            alphaI = Mathf::Abs(dot(next1, next2) / crossA);
          }

          Vec2 prev1 = center - m_Cells[prevNeigh].position;
          Vec2 prev2 = m_Cells[curNeigh].position - m_Cells[prevNeigh].position;

          float crossB = Segmentf::Cross(prev1, prev2);
          if(Mathf::Abs(crossB) > Mathf::Epsilon())
          {
            betaI = Mathf::Abs(dot(prev1, prev2) / crossB);
          }
          float curConnectionValue = (alphaI + betaI / area);
          sum -= curConnectionValue;

          mat.at(i, curNeigh) = 0.5 * curConnectionValue;
        }
         
        mat.at(i, i) = 0.5 * sum;
      }
    }
  }

  void Terrain::Diffusion(LaplaceMatrix const* iMatrix, float iHeatCoefficient, Vector<float>& ioValues)
  {
    ArmaLaplaceMatrix const* matrixContainer = static_cast<ArmaLaplaceMatrix const*>(iMatrix);

    unsigned int const numCells = m_Cells.size();

    eXl_ASSERT(numCells == ioValues.size());

    arma::colvec inputTemp(numCells);
    for (unsigned int i = 0; i < numCells; ++i)
    {
      inputTemp.at(i) = ioValues[i];
    }

    arma::colvec tempRes(numCells, arma::fill::zeros);

    arma::sp_mat resEqn(numCells, numCells);
    arma::sp_mat idMat(numCells, numCells);
    for (unsigned int i = 0; i < numCells; ++i)
      idMat.at(i, i) = 1.0;

    resEqn = idMat - matrixContainer->m_Matrix * iHeatCoefficient;

    arma::spsolve(tempRes, resEqn, inputTemp);

    for (unsigned int i = 0; i < numCells; ++i)
    {
      ioValues[i] = tempRes.at(i);
    }
  }

  size_t Terrain::GetNumMeshVertices()
  {
    size_t numVtx = 2 * m_Graph.m_Edges.size();
    for (auto const& cell : m_Cells)
    {
      numVtx += 1 + cell.neighCount;
    }

    return numVtx;
  }

  size_t Terrain::GetNumMeshIndices()
  {
    return GetNumSmoothMeshIndices() /*+ 6 * m_Graph.m_Edges.size()*/;
  }

  size_t Terrain::GetNumSmoothMeshVertices()
  {
    return m_Cells.size() + m_Graph.m_Vertices.size();
  }

  size_t Terrain::GetNumSmoothMeshIndices()
  {
    size_t numIndices = 0;
    for (auto const& cell : m_Cells)
    {
      numIndices += 3 * cell.neighCount;
    }

    return numIndices;
  }

  void Terrain::BuildMesh(Vec3 const& iScale, const Vector<float>& iHeight, OutputBuffer oPositions, OutputBuffer oNormals, OutputBuffer oTexCoords, OutputBuffer oIdx)
  {
    uint32_t maxIndices = GetNumMeshIndices();
    uint32_t counterCheck = 0;

    Vec3* positionsPtr = reinterpret_cast<Vec3*>(oPositions.data);
    float* texCoordPtr = reinterpret_cast<float*>(oTexCoords.data);
    Vec3* normalsPtr = reinterpret_cast<Vec3*>(oNormals.data);
    uint32_t* indicesPtr = reinterpret_cast<uint32_t*>(oIdx.data);
    uint32_t addVtxCounter = 0;

    UnorderedMap<uint32_t, uint32_t> vtxAlloc;

    for (unsigned int i = 0; i < m_Cells.size(); ++i)
    {
      uint32_t cellCenterIdx = addVtxCounter;
      vtxAlloc.clear();
      Vec2 ptCenter = m_Cells[i].position;

      float height = iHeight[i];

      Vec3 pos(iScale.x * ptCenter.x, iScale.y * ptCenter.y, iScale.z * height);

      *positionsPtr = pos;
      texCoordPtr[0] = ptCenter.x / m_Size.x;
      texCoordPtr[1] = ptCenter.y / m_Size.y;
      *normalsPtr = UnitZ<Vec3>();

      oPositions.NextItem(positionsPtr);
      oTexCoords.NextItem(texCoordPtr);
      oNormals.NextItem(normalsPtr);
      ++addVtxCounter;

      VoronoiGraph::CellGraphImpl::vertex_descriptor curCell = i;
      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      unsigned int numPts = 0;
      for (boost::tie(edgesBegin, edgesEnd) = boost::out_edges(curCell, m_Graph.m_CellGraph); edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int target = curCell == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if (target >= m_Cells.size())
        {
          continue;
        }

        VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;

        auto insertRes = vtxAlloc.insert(std::make_pair(curEdge.m_Pt1, addVtxCounter));
        if (insertRes.second)
        {
          *positionsPtr = Vec3(iScale.x * m_Graph.m_Vertices[curEdge.m_Pt1].m_Position.x, 
            iScale.y * m_Graph.m_Vertices[curEdge.m_Pt1].m_Position.y, 
            iScale.z * height);
          texCoordPtr[0] = m_Graph.m_Vertices[curEdge.m_Pt1].m_Position.x / m_Size.x;
          texCoordPtr[1] = m_Graph.m_Vertices[curEdge.m_Pt1].m_Position.y / m_Size.y;
          *normalsPtr = UnitZ<Vec3>();

          oPositions.NextItem(positionsPtr);
          oTexCoords.NextItem(texCoordPtr);
          oNormals.NextItem(normalsPtr);
          ++addVtxCounter;
        }
        insertRes = vtxAlloc.insert(std::make_pair(curEdge.m_Pt2, addVtxCounter));
        if (insertRes.second)
        {
          *positionsPtr = Vec3(iScale.x * m_Graph.m_Vertices[curEdge.m_Pt2].m_Position.x,
            iScale.y * m_Graph.m_Vertices[curEdge.m_Pt2].m_Position.y, 
            iScale.z * height);
          texCoordPtr[0] = m_Graph.m_Vertices[curEdge.m_Pt2].m_Position.x / m_Size.x;
          texCoordPtr[1] = m_Graph.m_Vertices[curEdge.m_Pt2].m_Position.y / m_Size.y;
          *normalsPtr = UnitZ<Vec3>();

          oPositions.NextItem(positionsPtr);
          oTexCoords.NextItem(texCoordPtr);
          oNormals.NextItem(normalsPtr);
          ++addVtxCounter;
        }
      }

      for (boost::tie(edgesBegin, edgesEnd) = boost::out_edges(curCell, m_Graph.m_CellGraph); edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int target = curCell == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if (target >= m_Cells.size())
        {
          continue;
        }

        VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;
        
        uint32_t pt1Idx = vtxAlloc[curEdge.m_Pt1];
        uint32_t pt2Idx = vtxAlloc[curEdge.m_Pt2];

        Vec3 const& otherPt1 = *oPositions.Item<Vec3>(pt1Idx);
        Vec3 const& otherPt2 = *oPositions.Item<Vec3>(pt2Idx);

        Vec3 locNormal =  cross((pos - otherPt1), (pos - otherPt2));

        if (dot(locNormal, UnitZ<Vec3>()) < -Mathf::Epsilon())
        {
          std::swap(pt1Idx, pt2Idx);
        }

        eXl_ASSERT(counterCheck < maxIndices);

        *indicesPtr = cellCenterIdx;
        oIdx.NextItem(indicesPtr);
        *indicesPtr = pt1Idx;
        oIdx.NextItem(indicesPtr);
        *indicesPtr = pt2Idx;
        oIdx.NextItem(indicesPtr);

        counterCheck += 3;
      }
    }

    for (unsigned int i = 0; i < m_Graph.m_Edges.size(); ++i)
    {
      //unsigned int numRef = 0;
      //float height = 0.0;
      //for (auto ref : edgePtRef[i])
      //{
      //  height += oPositions.Item<float>(ref)[2];
      //  ++numRef;
      //}
      //
      //if (numRef > 0)
      //  height /= numRef;
      //
      //
      //positionsPtr[0] = iScale.x * m_Graph.m_Vertices[i].m_Position.x;
      //positionsPtr[1] = iScale.y * m_Graph.m_Vertices[i].m_Position.y;
      //positionsPtr[2] = height;
      //texCoordPtr[0] = m_Graph.m_Vertices[i].m_Position.x / m_Size.x;
      //texCoordPtr[1] = m_Graph.m_Vertices[i].m_Position.y / m_Size.y;
      //*normalsPtr = Vec3::ZERO;
      //
      //oPositions.NextItem(positionsPtr);
      //oTexCoords.NextItem(texCoordPtr);
      //oNormals.NextItem(normalsPtr);
    }
  }

  void Terrain::BuildSmoothMesh(Vec3 const& iScale, const Vector<float>& iHeight, OutputBuffer oPositions, OutputBuffer oNormals, OutputBuffer oTexCoords, OutputBuffer oIdx)
  {
    unsigned int const numCells = m_Cells.size();

    Vector<Set<unsigned int> > edgePtRef(m_Graph.m_Vertices.size());

    float* positionsPtr = reinterpret_cast<float*>(oPositions.data);
    float* texCoordPtr = reinterpret_cast<float*>(oTexCoords.data);
    uint32_t* indicesPtr = reinterpret_cast<uint32_t*>(oIdx.data);
    Vec3* normalsPtr = reinterpret_cast<Vec3*>(oNormals.data);

    for (unsigned int i = 0; i < numCells; ++i)
    {
      Vec2 ptCenter = m_Cells[i].position;
      
      float height = iHeight[i];

      positionsPtr[0] = iScale.x * ptCenter.x;
      positionsPtr[1] = iScale.y * ptCenter.y;
      positionsPtr[2] = iScale.z * height;
      texCoordPtr[0] = ptCenter.x / m_Size.x;
      texCoordPtr[1] = ptCenter.y / m_Size.y;

      VoronoiGraph::CellGraphImpl::vertex_descriptor curCell = i;
      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      unsigned int numPts = 0;
      for (boost::tie(edgesBegin, edgesEnd) = boost::out_edges(curCell, m_Graph.m_CellGraph); edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int target = curCell == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if (target >= m_Cells.size())
        {
          continue;
        }

        VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;

        uint32_t pt1Idx = numCells + curEdge.m_Pt1;
        uint32_t pt2Idx = numCells + curEdge.m_Pt2;

        *indicesPtr = i;
        oIdx.NextItem(indicesPtr);
        *indicesPtr = pt1Idx;
        oIdx.NextItem(indicesPtr);
        *indicesPtr = pt2Idx;
        oIdx.NextItem(indicesPtr);

        edgePtRef[curEdge.m_Pt1].insert(i);
        edgePtRef[curEdge.m_Pt2].insert(i);
      }  

      *normalsPtr = Zero<Vec3>();

      oPositions.NextItem(positionsPtr);
      oTexCoords.NextItem(texCoordPtr);
      oNormals.NextItem(normalsPtr);
    }

    for(unsigned int i = 0; i<edgePtRef.size(); ++i)
    {
      unsigned int numRef = 0;
      float height = 0.0;
      for(auto ref : edgePtRef[i])
      {
        height += oPositions.Item<float>(ref)[2];
        ++numRef;
      }

      if(numRef > 0)
        height /= numRef;


      positionsPtr[0] = iScale.x * m_Graph.m_Vertices[i].m_Position.x;
      positionsPtr[1] = iScale.y * m_Graph.m_Vertices[i].m_Position.y;
      positionsPtr[2] = height;
      texCoordPtr[0] = m_Graph.m_Vertices[i].m_Position.x / m_Size.x;
      texCoordPtr[1] = m_Graph.m_Vertices[i].m_Position.y / m_Size.y;
      *normalsPtr = Zero<Vec3>();

      oPositions.NextItem(positionsPtr);
      oTexCoords.NextItem(texCoordPtr);
      oNormals.NextItem(normalsPtr);
    }

    indicesPtr = reinterpret_cast<uint32_t*>(oIdx.data);
    for (unsigned int i = 0; i < numCells; ++i)
    {
      Vec3 const& pos = *oPositions.Item<Vec3>(i);

      VoronoiGraph::CellGraphImpl::vertex_descriptor curCell = i;
      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      unsigned int numPts = 0;
      Vec3 normal;
      Set<unsigned int> pts;
      for (boost::tie(edgesBegin, edgesEnd) = boost::out_edges(curCell, m_Graph.m_CellGraph); edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int target = curCell == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if (target >= m_Cells.size())
        {
          continue;
        }

        VoronoiGraph::Edge const& curEdge = m_Graph.m_Edges.find(*edgesBegin)->second;

        Vec3 const& otherPt1 = *oPositions.Item<Vec3>(curEdge.m_Pt1 + numCells);
        Vec3 const& otherPt2 = *oPositions.Item<Vec3>(curEdge.m_Pt2 + numCells);

        Vec3 locNormal = cross((pos - otherPt1), (pos - otherPt2));

        if (dot(locNormal, UnitZ<Vec3>()) < -Mathf::Epsilon())
        {
          locNormal *= -1.0;
          std::swap(indicesPtr[1], indicesPtr[2]);
        }

        *oNormals.Item<Vec3>(i) += locNormal;
        *oNormals.Item<Vec3>(curEdge.m_Pt1 + numCells) += locNormal;
        *oNormals.Item<Vec3>(curEdge.m_Pt2 + numCells) += locNormal;

        oIdx.NextItem(indicesPtr);
        oIdx.NextItem(indicesPtr);
        oIdx.NextItem(indicesPtr);
      }
    }

    normalsPtr = reinterpret_cast<Vec3*>(oNormals.data);
    for (unsigned int i = 0; i < numCells + m_Graph.m_Vertices.size(); ++i)
    {
      *normalsPtr = normalize(*normalsPtr);
      oNormals.NextItem(normalsPtr);
    }
  }

  void Terrain::ComputeFlowMap(const Vector<float>& iHeight, Vector<unsigned int>& oSummits, Vector<int>& oNext)
  {
    unsigned int const numCells = m_Cells.size();

    oSummits.clear();
    oNext.resize(numCells, -1);

    
    Vector<float> filledHeight(numCells, Mathf::MaxReal());

    for (unsigned int i = 0; i < numCells; ++i)
    {
      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
      for(; edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int curNeigh = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;

        if(curNeigh == numCells)
        {
          filledHeight[i] = iHeight[i];
          break;
        }
      }
    }

    bool updated;
    do
    {
      updated = false;

      for (unsigned int i = 0; i < numCells; ++i)
      {
        if(iHeight[i] == filledHeight[i])
          continue;

        float curOrigH = iHeight[i];

        VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
        boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
        for(; edgesBegin != edgesEnd; ++edgesBegin)
        {
          unsigned int curNeigh = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;

          eXl_ASSERT(curNeigh < numCells);

          float neighH = filledHeight[curNeigh];
          if(curOrigH >= neighH + Mathf::ZeroTolerance())
          {
            //C'est bon on a un voisin plus bas
            filledHeight[i] = curOrigH;
            updated = true;
          }
          else
          {
            float newH = neighH + Mathf::ZeroTolerance();
            if(filledHeight[i] > newH && (newH > curOrigH))
            {
              filledHeight[i] = newH;
              updated = true;
            }
          }
        }
      }
    }while(updated);

    for (unsigned int i = 0; i < numCells; ++i)
    {
      int lowerN = -1;
      float curLowerH = Mathf::MaxReal();
      float curH = filledHeight[i];
      bool summit = true;

      VoronoiGraph::CellGraphImpl::out_edge_iterator edgesBegin, edgesEnd;
      boost::tie(edgesBegin, edgesEnd) = boost::out_edges(i, m_Graph.m_CellGraph);
      for(; edgesBegin != edgesEnd; ++edgesBegin)
      {
        unsigned int curNeigh = i == edgesBegin->m_target ? edgesBegin->m_source : edgesBegin->m_target;
        if(curNeigh == numCells)
          continue;

        float neighH = filledHeight[curNeigh];
        if(curH > neighH)
        {
          if(neighH < curLowerH)
          {
            lowerN = curNeigh;
            curLowerH = neighH;
          }
        }
        else
          summit = false;
      }

      if(summit)
        oSummits.push_back(i);

      oNext[i] = lowerN;
    }
  }

  //void Terrain::ComputeNormals(Vec3 const& iScale, const Vector<float>& iHeight, Vector<Vec3>& oNormals)
  //{
  //  unsigned int const numCells = m_Cells.size();
  //  oNormals.resize(numCells);
  //
  //  for (unsigned int i = 0; i < numCells; ++i)
  //  {
  //    Vec3 pos(m_Cells[i].position.x * iScale.x, iHeight[i] * iScale.y, m_Cells[i].position.y * iScale.z);
  //
  //    unsigned int neighStart = m_Cells[i].neighStart;
  //    unsigned int neighEnd = m_Cells[i].neighCount + neighStart;
  //
  //    Vec3 normal;
  //    if(neighEnd != neighStart)
  //    {
  //      unsigned int prevNeigh = m_Neigh[neighEnd - 1];
  //      Vec3 prevPt(m_Cells[prevNeigh].position.x * iScale.x, iHeight[prevNeigh] * iScale.y, m_Cells[prevNeigh].position.y * iScale.z);
  //
  //      for (unsigned int neighIdx = neighStart; neighIdx < neighEnd; ++neighIdx)
  //      {
  //        unsigned int curNeigh = m_Neigh[neighIdx];
  //        Vec3 curPt(m_Cells[curNeigh].position.x * iScale.x, iHeight[curNeigh] * iScale.y, m_Cells[curNeigh].position.y * iScale.z);
  //        Vec3 locNormal = (pos - prevPt).Cross(pos - curPt);
  //
  //        if(locNormal.Dot(UnitY<Vec3>()) < -Mathf::Epsilon())
  //          locNormal *= -1.0;
  //
  //        normal += locNormal;
  //        prevPt = curPt;
  //      }
  //    }
  //    normal.Normalize();
  //    oNormals[i] = normal;
  //  }
  //}

}