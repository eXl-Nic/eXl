/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/dungeonlayout.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>

#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/function_property_map.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <math/geometrytraits.hpp>
#include <math/mathtools.hpp>

#include <math/segment.hpp>

#include <math/polygon_def.hpp>

#include <gen/graphutils.hpp>
namespace eXl
{
  using RoomIndexEntry = std::pair<AABB2Di, uint32_t>;
  using RoomIndex = boost::geometry::index::rtree<RoomIndexEntry, boost::geometry::index::linear<16, 4>>;

  typedef DungeonGraph::Graph Graph;
  typedef Graph::vertex_descriptor Vertex;
  typedef Graph::edge_descriptor Edge;

  //struct NodeWithDegree
  //{
  //  Vertex vtx;
  //  uint32_t degree;
  //};
  //
  //struct NodeIdentity
  //{
  //  typedef Vertex result_type;
  //  result_type operator()(NodeWithDegree const& iEntry) const
  //  {
  //    return iEntry.vtx;
  //  }
  //};
  //
  //struct NodeDegree
  //{
  //  typedef uint32_t result_type;
  //  result_type operator()(NodeWithDegree const& iEntry) const
  //  {
  //    return iEntry.degree;
  //  }
  //};
  //
  //typedef boost::multi_index::multi_index_container<NodeWithDegree,
  //  boost::multi_index::indexed_by<
  //  boost::multi_index::hashed_unique<NodeIdentity>
  //  ,boost::multi_index::ordered_non_unique<NodeDegree>
  //  >  
  //> NodeDegreeMap;

  struct PhysicalNodesFilter
  {
    PhysicalNodesFilter()
    {}

    PhysicalNodesFilter(DungeonGraph const& iGraph)
      : m_Graph(&iGraph)
    {}

    bool operator()(const DungeonGraph::GraphVtx& e) const
    {
      DungeonGraph::NodeProperties const* props = m_Graph->GetProperties(e);
      return props->IsPhysical() && props->GetSize() > 0;
    }

    bool operator()(const DungeonGraph::GraphEdge& e) const
    {
      return m_Graph->GetProperties(e)->m_PhysicalConnection;
    }

    DungeonGraph const* m_Graph;
  };

  struct MSTFilter
  {
    MSTFilter()
    {}

    MSTFilter(Set<Edge> const& iEdges)
      :m_Edges(&iEdges)
    {}

    template <typename Edge>
    bool operator()(const Edge& e) const
    {
      return m_Edges->count(e) > 0;
    }
    Set<Edge> const* m_Edges;
  };

  struct GraphFilter
  {
    GraphFilter()
    {}

    GraphFilter(Set<Vertex> const& iVtx)
      :m_Vtx(&iVtx)
    {}

    bool operator()(const Vertex& vtx) const
    {
      return m_Vtx->count(vtx) > 0;
    }
    Set<Vertex> const* m_Vtx;
  };

  namespace
  {

    int const s_DoorSize = 4;
    int const s_MaxRoomSize = 5;

    Vec2i SampleRoom(DungeonGraph const& iGraph, Vertex iVtx, Random& iRand)
    {
      DungeonGraph::NodeProperties const* props = iGraph.GetProperties(iVtx);

      Vec2i baseSize = Vec2i(iRand() % 10 + 4, iRand() % 10 + 4);

      //uint32_t const roomSize = Mathi::Min(s_MaxRoomSize, props != nullptr ? props->GetSize() : 1);
      //
      //Vec2i baseSize = One<Vec2i>() * 2 * (roomSize + 1);
      //
      //switch(iRand() % 3)
      //{
      //case 0:
      //  baseSize += One<Vec2i>() * Mathi::Max(roomSize / 4, 1);
      //  break;
      //case 1:
      //  baseSize.m_Data[0] += roomSize / 2;
      //  break;
      //case 2:
      //  baseSize.m_Data[1] += roomSize / 2;
      //  break;
      //}
      return baseSize;
    };

    void AddRoomConfSpace(Vec2i const& iBoxToLayoutSize, AABB2Di const& iBox, Vector<Segmenti>& oSegs) 
    {
      for(int dim = 0; dim<2; ++dim)
      {
        int otherDim = 1 - dim;
        Segmenti minSeg;
        minSeg.m_Ext1 = minSeg.m_Ext2 = iBox.m_Data[0];
        minSeg.m_Ext1[otherDim] -= iBoxToLayoutSize[otherDim];
        minSeg.m_Ext2 = minSeg.m_Ext1;
        minSeg.m_Ext2[dim] = iBox.m_Data[1][dim];

        minSeg.m_Ext1[dim] -= (iBoxToLayoutSize[dim] - s_DoorSize);
        minSeg.m_Ext2[dim] -= s_DoorSize;

        oSegs.push_back(minSeg);

        Segmenti maxSeg = minSeg;
        maxSeg.m_Ext1[otherDim] = maxSeg.m_Ext2[otherDim] = iBox.m_Data[1][otherDim];

        oSegs.push_back(maxSeg);
      }
    };

    Vector<Segmentf> MergeConfSpace (Vector<Segmentf> const& iSpace1, Vector<Segmentf> const& iSpace2)
    {
      Vector<Segmentf> result;
      for(auto const& seg1 : iSpace1)
      {
        bool seg1IsPoint = seg1.m_Ext1 == seg1.m_Ext2;
        for(auto const& seg2 : iSpace2)
        {
          bool seg2IsPoint = seg2.m_Ext1 == seg2.m_Ext2;
          if(seg1IsPoint)
          {
            if(seg2IsPoint)
            {
              if(seg1 == seg2)
              {
                result.push_back(seg1);
              }
            }
            else
            {
              if(seg2.IsOnSegment(seg1.m_Ext1))
              {
                result.push_back(seg1);
              }
            }
          }
          else
          {
            if(seg2IsPoint)
            {
              if(seg1.IsOnSegment(seg2.m_Ext1))
              {
                result.push_back(seg2);
              }
            }
            else
            {
              Vec2 oPoint;
              uint32_t res = seg1.Intersect(seg2, oPoint);
              if((res & Segmentf::PointOnSegments) == Segmentf::PointOnSegments)
              {
                //Intersection limited to a point.
                Segmentf resultSegment;
                resultSegment.m_Ext1 = resultSegment.m_Ext2 = oPoint;
                result.push_back(resultSegment);
              }
              else if(res & Segmentf::ConfoundSegments)
              {
                Segmentf resultSegment;
                //Segments overlap.
                if(seg1.IsOnSegment(seg2.m_Ext1))
                {
                  resultSegment.m_Ext1 = seg2.m_Ext1;
                }
                else
                {
                  resultSegment.m_Ext1 = seg2.m_Ext2;
                }

                if(seg2.IsOnSegment(seg1.m_Ext1))
                {
                  resultSegment.m_Ext2 = seg1.m_Ext1;
                }
                else
                {
                  resultSegment.m_Ext2 = seg1.m_Ext2;
                }
              }
            }
          }
        }
      }

      return result;
    };
  }

  void ComputeMaxConfSpace(Vector<Segmenti>& ioConfSpace)
  {
    unsigned int const s_NumSegPerRoom = 4;

    //Compute maximum configuration space.
    Vector<std::pair<size_t, Segmenti>> segments;
    boost::polygon::intersect_segments(segments, ioConfSpace.begin(), ioConfSpace.end());

    //Create graph of connected segments and get the biggest connected component.
    UnorderedMap<Vec2i, uint32_t> pointMap;
    Vector<Vec2i> vertices;
    Vector<Segmenti> edges;
    Vector<Set<uint32_t>> vtxMap;
    Vector<Set<uint32_t>>  edgeMap;
    Map<std::pair<uint32_t, uint32_t>, uint32_t> connections;
    uint32_t numEdges = 0;
    uint32_t numVtx = 0;

    for(auto const& seg : segments)
    {
      auto insertRes = pointMap.insert(std::make_pair(seg.second.m_Ext1, numVtx));
      if(insertRes.second)
      {
        ++numVtx;
        vertices.push_back(seg.second.m_Ext1);
        vtxMap.push_back(Set<uint32_t>());
      }
      auto vtx1 = insertRes.first->second;
      vtxMap[vtx1].insert(seg.first / s_NumSegPerRoom);

      insertRes = pointMap.insert(std::make_pair(seg.second.m_Ext2, numVtx));
      if(insertRes.second)
      {
        ++numVtx;
        vertices.push_back(seg.second.m_Ext1);
        vtxMap.push_back(Set<uint32_t>());
      }
      auto vtx2 = insertRes.first->second;
      vtxMap[vtx2].insert(seg.first / s_NumSegPerRoom);

      auto edgeDesc = std::make_pair(vtx1, vtx2);
      if(edgeDesc.first > edgeDesc.second)
      {
        std::swap(edgeDesc.first, edgeDesc.second);
      }
      auto newEdge = connections.insert(std::make_pair(edgeDesc, numEdges));
      if(newEdge.second)
      {
        ++numEdges;
        edges.push_back(seg.second);
        edgeMap.push_back(Set<uint32_t>());
      }
      edgeMap[newEdge.first->second].insert(seg.first / s_NumSegPerRoom);
    }

    ioConfSpace.clear();

    uint32_t maxScore = 0;
    for(auto const& entry : vtxMap)
    {
      auto curScore = entry.size();
      if(curScore > maxScore)
      {
        maxScore = curScore;
      }
    }
    for(auto const& edge : connections)
    {
      if(edgeMap[edge.second].size() == maxScore)
      {
        ioConfSpace.push_back(edges[edge.second]);
        vtxMap[edge.first.first].clear();
        vtxMap[edge.first.second].clear();
      }
    }

    for(auto const& vtx : pointMap)
    {
      if(vtxMap[vtx.second].size() == maxScore)
      {
        ioConfSpace.push_back({vtx.first, vtx.first});
      }
    }
  }

  Vector<Segmenti> CullConfigurationSpace(Vec2i const& iRoomSize, Vector<Segmenti> const& iConfSpace, RoomIndex const& iRoomIndex)
  {
    Vector<Segmenti> finalConfSpace;
    Vector<RoomIndexEntry> results;
    for(auto const& seg : iConfSpace)
    {
      results.clear();
      if(seg.m_Ext1 == seg.m_Ext2)
      {
        AABB2Di boxToTest = AABB2Di::FromMinAndSize(seg.m_Ext1, iRoomSize);
        iRoomIndex.query(boost::geometry::index::overlaps(boxToTest), std::back_inserter(results));
        for(auto& result : results)
        {
          if(boxToTest.Intersect(result.first))
            continue;
        }
        finalConfSpace.push_back(seg);
      }
      else
      {
        int dim = seg.m_Ext1[0] != seg.m_Ext2[0] ? 0 : 1;
        int otherDim = 1 - dim;

        int minSeg = seg.m_Ext1[dim] < seg.m_Ext2[dim] ? 0 : 1;
        int maxSeg = 1 - minSeg;

        Vec2i const* segPts = reinterpret_cast<Vec2i const*>(&seg);
        AABB2Di boxToTest;
        boxToTest.m_Data[0] = segPts[minSeg];
        boxToTest.m_Data[1] = segPts[maxSeg] + iRoomSize;

        Segmenti finalSeg;
        finalSeg.m_Ext1 = segPts[minSeg];
        finalSeg.m_Ext2 = segPts[maxSeg];

        Vector<AABB2Di> interBoxes;

        iRoomIndex.query(boost::geometry::index::overlaps(boxToTest), std::back_inserter(results));
        for(auto& result : results)
        {
          if(boxToTest.Intersect(result.first))
          {
            AABB2Di intersection;
            intersection.SetCommonBox(result.first, boxToTest);

            interBoxes.push_back(intersection);
          }
        }

        std::sort(interBoxes.begin(), interBoxes.end(), [dim](AABB2Di const& iBox1, AABB2Di const& iBox2)
        {
          return iBox1.m_Data[0][dim] < iBox2.m_Data[0][dim];
        });

        for(auto& box : interBoxes)
        {
          if(box.m_Data[0][dim] <= finalSeg.m_Ext1[dim])
          {
            finalSeg.m_Ext1[dim] = Mathi::Max(box.m_Data[1][dim], finalSeg.m_Ext1[dim]);
          }
          else 
          {
            if(box.m_Data[0][dim] > finalSeg.m_Ext1[dim] + iRoomSize[dim])
            {
              Segmenti seg;
              seg.m_Ext1 = seg.m_Ext2 = finalSeg.m_Ext1;
              seg.m_Ext2[dim] = box.m_Data[0][dim] - iRoomSize[dim];

              finalConfSpace.push_back(seg);
            }

            finalSeg.m_Ext1[dim] = box.m_Data[1][dim];
          }
        }

        if(finalSeg.m_Ext2[dim] - finalSeg.m_Ext1[dim] > 0)
        {
          finalConfSpace.push_back(finalSeg);
        }
      }
    }

    return finalConfSpace;
  }

  struct RoomViolation
  {
    Map<uint32_t, float> m_Overlaps;
    Map<uint32_t, float> m_MissingDoor;
  };

  //Insert the room and compute its energy regarding collision against other rooms.
  template <typename FilteredGraph>
  void InsertRoomAndUpdateViolations(Vertex iVtx, uint32_t const iRoomIdx, AABB2Di const& iRoom, 
    RoomIndex& iRoomIndex, Map<uint32_t, RoomViolation>& oMap, 
    FilteredGraph const& iGraph, Map<Vertex, uint32_t> const& iVtxToRoom, Layout const& iCurLayout)
  {
    Vector<RoomIndexEntry> results;
    Set<uint32_t> touchingRooms;

    iRoomIndex.query(
      boost::geometry::index::intersects(iRoom),
      std::back_inserter(results));

    float totArea = 0;
    for(auto const& entry : results)
    {
      AABB2Di intersectionBox;
      intersectionBox.SetCommonBox(iRoom, entry.first);

      auto size = intersectionBox.GetSize();

      if(size.x > 0 || size.y > 0)
      {
        if(size.x * size.y == 0)
        {
          if(size.x + size.y >= s_DoorSize)
          {
            touchingRooms.insert(entry.second);
          }
        }
        else
        {
          float curArea = size.x * size.y;
          if(curArea > 0)
          {
            totArea += curArea;
            if(size.x >= s_DoorSize || size.y >= s_DoorSize)
            {
              touchingRooms.insert(entry.second);
            }

            auto insertRes = oMap.insert(std::make_pair(entry.second, RoomViolation()));
            insertRes.first->second.m_Overlaps[iRoomIdx] = curArea;

            insertRes = oMap.insert(std::make_pair(iRoomIdx, RoomViolation()));
            insertRes.first->second.m_Overlaps[entry.second] = curArea;
          }
        }
      }
    }

    iRoomIndex.insert(std::make_pair(iRoom, iRoomIdx));

    Vec2 center1 = MathTools::ToFVec(iRoom.m_Data[0]) + MathTools::ToFVec(iRoom.GetSize()) * 0.5;

    for(auto edge : OutEdgesIter(iGraph, iVtx))
    {
      Vertex target = GetTarget(iVtx, edge);
      uint32_t neighIdx = iVtxToRoom.find(target)->second;

      auto const& neightRoom = iCurLayout[neighIdx].m_Box;

      if(touchingRooms.count(neighIdx) == 0)
      {
        Vec2 center2 = MathTools::ToFVec(neightRoom.m_Data[0]) + MathTools::ToFVec(neightRoom.GetSize()) * 0.5;
        float sqDist = distance2(center2, center1);

        auto insertRes = oMap.insert(std::make_pair(neighIdx, RoomViolation()));
        insertRes.first->second.m_MissingDoor[iRoomIdx] = sqDist;

        insertRes = oMap.insert(std::make_pair(iRoomIdx, RoomViolation()));
        insertRes.first->second.m_MissingDoor[neighIdx] = sqDist;
      }
    }
  }

  void RemoveRoomAndUpdateViolations(uint32_t const iRoomIdx, AABB2Di const& iRoom, RoomIndex& iRoomIndex, Map<uint32_t, RoomViolation>& oMap)
  {
    iRoomIndex.remove(RoomIndexEntry(iRoom, iRoomIdx));

    auto violations = oMap.find(iRoomIdx);
    if(violations != oMap.end())
    {
      for(auto overlap : violations->second.m_Overlaps)
      {
        auto otherViolation = oMap.find(overlap.first);
        if(otherViolation != oMap.end())
        {
          otherViolation->second.m_Overlaps.erase(iRoomIdx);
        }
      }
      for(auto missingDoor : violations->second.m_MissingDoor)
      {
        auto otherViolation = oMap.find(missingDoor.first);
        if(otherViolation != oMap.end())
        {
          otherViolation->second.m_MissingDoor.erase(iRoomIdx);
        }
      }
      oMap.erase(violations);
    }
  }

  AABB2Di SampleConfigurationSpace(Vec2i const& iRoomSize, Random& iRand, Vector<Segmenti> const& iConfSpace, RoomIndex const& iRoomIndex)
  {
    Vector<Segmenti> culledConfSpace = CullConfigurationSpace(iRoomSize, iConfSpace, iRoomIndex);

    Vector<Segmenti> const& confSpace = (culledConfSpace.empty() ? iConfSpace : culledConfSpace);

    Multimap<float, AABB2Di> sampledPos;
    while(sampledPos.size() < 20)
    {
      // Should try to get a uniform sampling.
      auto const& seg = confSpace[iRand() % confSpace.size()];
      Vec2i roomOrigin;
      if(seg.m_Ext1 != seg.m_Ext2)
      {
        Vec2i segDir = seg.m_Ext2 - seg.m_Ext1;
        int32_t segLen = Mathi::Abs(Mathi::Max(segDir.x, segDir.y));
        segDir /= segLen;
        segLen = iRand() % segLen;

        roomOrigin = seg.m_Ext1 + segDir * segLen;
      }
      else
      {
        roomOrigin = seg.m_Ext1;
      }

      AABB2Di newBox;
      newBox.m_Data[0] = roomOrigin;
      newBox.m_Data[1] = roomOrigin + iRoomSize;

      Vector<RoomIndexEntry> results;

      iRoomIndex.query(
        boost::geometry::index::intersects(newBox),
        std::back_inserter(results));

      float totArea = 0;
      for(auto const& entry : results)
      {
        AABB2Di intersectionBox;
        intersectionBox.SetCommonBox(newBox, entry.first);

        auto size = intersectionBox.GetSize();

        totArea += size.x * size.y;
      }

      sampledPos.insert(std::make_pair(totArea, newBox));

      if(totArea == 0)
        break;
    }

    return sampledPos.begin()->second;
  }

  using PhysicalGraph = boost::filtered_graph<Graph, PhysicalNodesFilter, PhysicalNodesFilter>;

  Vector<Vector<Vertex>> GetChainList(/*DungeonGraph const& iGraph, */PhysicalGraph const& graph)
  {
    auto const& idxMap = boost::get(boost::vertex_index, graph);

    //Test if the graph can be layout.
    if (!boost::boyer_myrvold_planarity_test(graph))
    {
      return Vector<Vector<Vertex>>();
    }
    Vector<Vector<Vertex>> cycles;
    Vector<Vector<Vertex>> chains;

    Set<Edge> origEdges;
    for (auto edges = boost::edges(graph); edges.first != edges.second; ++edges.first)
    {
      origEdges.insert(*edges.first);
    }

    auto dummyWhFunc = [](Edge) {return 1.0; };

    Set<Edge> mstEdges;
    boost::kruskal_minimum_spanning_tree(graph, std::inserter(mstEdges, mstEdges.begin()),
      boost::weight_map(boost::make_function_property_map<Edge>(dummyWhFunc)).vertex_index_map(idxMap));

    //--> Could replace it with a planar embedding and process faces.
    Set<Edge> cycleEdges;
    std::set_difference(origEdges.begin(), origEdges.end(), mstEdges.begin(), mstEdges.end(), std::inserter(cycleEdges, cycleEdges.begin()));

    //NodeDegreeMap nodeDegreeMap;
    Map<Vertex, uint32_t> nodeDegreeMap;
    for (auto vtx : VerticesIter(graph))
    {
      nodeDegreeMap.insert({ vtx, uint32_t(boost::degree(vtx, graph)) });
    }

    MSTFilter filter(mstEdges);

    boost::filtered_graph<PhysicalGraph, MSTFilter, boost::keep_all> filteredGr(graph, filter, boost::keep_all());

    for (auto edge : cycleEdges)
    {
      Vector<Vertex> newCycle;
      Vertex ext1 = edge.m_source;
      Vertex ext2 = edge.m_target;

      Vector<Vertex> p(boost::num_vertices(filteredGr), ext1);
      Vector<float> d(boost::num_vertices(filteredGr));

      boost::dijkstra_shortest_paths(filteredGr, ext1,
        boost::make_iterator_property_map(p.begin(), idxMap),
        boost::make_iterator_property_map(d.begin(), idxMap),
        boost::make_function_property_map<Edge>(dummyWhFunc),
        idxMap,
        std::less<float>(), boost::closed_plus<float>(), Mathf::MaxReal(), 0.0, boost::dijkstra_visitor<boost::null_visitor>());

      float dist1 = d[idxMap[ext1]];
      float dist2 = d[idxMap[ext2]];

      Vertex farthestVtx = dist1 > dist2 ? ext1 : ext2;
      Vertex nearestVtx = dist1 > dist2 ? ext2 : ext1;

      do
      {
        newCycle.push_back(farthestVtx);
        farthestVtx = p[idxMap[farthestVtx]];
      } while (farthestVtx != nearestVtx);

      newCycle.push_back(farthestVtx);

      cycles.emplace_back(std::move(newCycle));
    }

    std::sort(cycles.begin(), cycles.end(),
      [](Vector<Vertex> const& iCycle1, Vector<Vertex> const& iCycle2)
    {
      return iCycle1.size() < iCycle2.size();
    });

    for (int i = 0; i < (int)cycles.size(); ++i)
    {
      bool cycleNested = false;
      auto& curCycle = cycles[i];
      auto cycleCopy = curCycle;
      for (auto& vtx : curCycle)
      {
        for (auto outEdge : OutEdgesIter(graph, vtx))
        {
          auto entry = nodeDegreeMap.find(outEdge.m_target);
          if (entry != nodeDegreeMap.end())
          {
            entry->second--;
          }
          //auto entry = nodeDegreeMap.get<0>().find(outEdge.m_target);
          //if (entry != nodeDegreeMap.get<0>().end())
          //{
          //  NodeWithDegree node = *entry;
          //  node.degree--;
          //  nodeDegreeMap.replace(entry, node);
          //}
        }
        if (nodeDegreeMap.count(vtx) == 0)
        {
          cycleNested = true;
          vtx = Graph::null_vertex();
        }
        else
        {
          nodeDegreeMap.erase(vtx);
        }
      }

      //Consecutive sequences of non-null vertices are chains to add.
      if (cycleNested)
      {
        Vector<Vertex> newChain;
        for (int i = -1; i < (int)curCycle.size(); ++i)
        {
          int loopedIdx = i >= 0 ? i : curCycle.size() - 1;
          Vertex vtx = curCycle[loopedIdx];

          if (vtx != Graph::null_vertex())
          {
            newChain.push_back(vtx);
          }
          else
          {
            if (!newChain.empty())
            {
              // Add vertex for connection.
              newChain.push_back(cycleCopy[loopedIdx]);
              chains.emplace_back(std::move(newChain));
            }
          }
        }

        if (!newChain.empty())
        {
          chains.emplace_back(std::move(newChain));
        }

        cycles[i] = std::move(cycles.back());
        cycles.pop_back();
        --i;
      }
    }

    Vertex pathSearchStartVtx = Graph::null_vertex();
    for (auto vtx : VerticesIter(filteredGr))
    {
      auto edges = boost::out_edges(vtx, filteredGr);
      if (std::distance(edges.first, edges.second) == 1)
      {
        pathSearchStartVtx = vtx;
        break;
      }
    }

    if (pathSearchStartVtx != Graph::null_vertex())
    {
      Vector<Vertex> p(boost::num_vertices(filteredGr), pathSearchStartVtx);
      Vector<float> d(boost::num_vertices(filteredGr));

      boost::dijkstra_shortest_paths(filteredGr, pathSearchStartVtx,
        boost::make_iterator_property_map(p.begin(), idxMap),
        boost::make_iterator_property_map(d.begin(), idxMap),
        boost::make_function_property_map<Edge>(dummyWhFunc), idxMap,
        std::less<float>(), boost::closed_plus<float>(), Mathf::MaxReal(), 0.0, boost::dijkstra_visitor<boost::null_visitor>());

      while (!nodeDegreeMap.empty())
      {
        //auto chainExtremityEntry = nodeDegreeMap.get<1>().begin();
        auto chainExtremityEntry = std::min_element(nodeDegreeMap.begin(), nodeDegreeMap.end(),
          [](std::pair<Vertex, uint32_t> const& iElem1, std::pair<Vertex, uint32_t> const& iElem2)
        {
          return iElem1.second < iElem2.second;
        });

        //if (chainExtremityEntry->vtx == pathSearchStartVtx)
        if (chainExtremityEntry->first == pathSearchStartVtx)
        {
          nodeDegreeMap.erase(chainExtremityEntry);
          continue;
        }

        Vector<Vertex> newChain;
        Vertex chainExtremity = chainExtremityEntry->first;
        if (chainExtremityEntry->second == 0)
        {
          nodeDegreeMap.erase(chainExtremityEntry);
          //Was connected to cycles
          auto edges = boost::out_edges(chainExtremity, filteredGr);
          if (std::distance(edges.first, edges.second) >= 1)
          {
            newChain.push_back(edges.first->m_target);
          }
        }
        else
        {
          //auto iter = nodeDegreeMap.get<0>().find(chainExtremity);
          //NodeWithDegree entry = *iter;
          
          eXl_ASSERT(chainExtremityEntry->second == 1);

          while (true)
          {
            newChain.push_back(chainExtremity);
            //degree--;
            //if (degree == 0)
            {
              nodeDegreeMap.erase(chainExtremity);
              chainExtremityEntry = nodeDegreeMap.end();
            }

            {
              auto predecessor = p[idxMap[chainExtremity]];
              if (predecessor == chainExtremity)
              {
                break;
              }

              chainExtremity = predecessor;
            }

            chainExtremityEntry = nodeDegreeMap.find(chainExtremity);
            if (chainExtremityEntry == nodeDegreeMap.end())
            {
              // Was removed when processing cycles, stop here.
              break;
            }
            // decrease degree of next node.
            //entry = *iter;
            uint32_t& degree = chainExtremityEntry->second;
            degree--;
            //nodeDegreeMap.replace(iter, entry);
            if (degree == 0)
            {
              nodeDegreeMap.erase(chainExtremity);
            }
            else
            {
              //nodeDegreeMap.replace(iter, entry);
              if (degree != 1)
              {
                //Next vertex has more neighbours.
                break;
              }
            }
          }
        }
        newChain.push_back(chainExtremity);

        chains.emplace_back(std::move(newChain));
      }
    }

    if (cycles.empty() && chains.empty())
    {
      return Vector<Vector<Vertex>>();
    }

    //First process smaller cycle.
    //Next order all chain through a BFS exploration.

    Vector<Vector<Vertex>> orderedChains;
    orderedChains.reserve(cycles.size() + chains.size());
    if (!cycles.empty())
    {
      orderedChains.emplace_back(std::move(cycles.front()));
      cycles.front() = std::move(cycles.back());
      cycles.pop_back();

      for (auto& cycle : cycles)
      {
        chains.emplace_back(std::move(cycle));
      }
      cycles.clear();
    }
    else
    {
      orderedChains.push_back(std::move(chains.back()));
      chains.pop_back();
    }

    Map<Vertex, Set<uint32_t>> vtxChains;

    for (uint32_t i = 0; i < chains.size(); ++i)
    {
      auto const& chain = chains[i];
      for (auto vtx : chain)
      {
        auto& set = vtxChains.insert(std::make_pair(vtx, Set<uint32_t>())).first->second;
        set.insert(i);
      }
    }

    uint32_t curChainBegin = 0;
    uint32_t curChainEnd = 1;
    uint32_t nextChainEnd = 1;

    uint32_t remainingChains = chains.size();
    while (remainingChains > 0)
    {
      for (; curChainBegin < curChainEnd; ++curChainBegin)
      {
        auto& curChain = orderedChains[curChainBegin];
        for (auto vtx : curChain)
        {
          for (auto edge : OutEdgesIter(graph, vtx))
          {
            auto entry = vtxChains.find(edge.m_target);
            if (entry != vtxChains.end())
            {
              for (auto connectedChain : entry->second)
              {
                if (!chains[connectedChain].empty())
                {
                  orderedChains.emplace_back(chains[connectedChain]);
                  chains[connectedChain].clear();
                  ++nextChainEnd;
                  --remainingChains;
                }
              }
            }
          }
        }
      }
      eXl_ASSERT_REPAIR_RET(curChainEnd != nextChainEnd, Vector<Vector<Vertex>>());
      curChainEnd = nextChainEnd;
    }

    return orderedChains;
  }

  LayoutCollection LayoutGraph(DungeonGraph const& iGraph, Random& iRand)
  {
    auto const& fullGraph = iGraph.GetGraph();
    
    PhysicalNodesFilter phFilter;
    PhysicalGraph graph(fullGraph, phFilter, phFilter);

    Vector<Vector<Vertex>> orderedChains = GetChainList(graph);

    if (orderedChains.empty())
    {
      return LayoutCollection();
    }

    uint32_t curChainToAdd = 0;

    Vector<LayoutCollection> layoutStack;

    layoutStack.push_back(LayoutCollection());
    layoutStack.back().push_back(Layout());

    while(!layoutStack.empty() && layoutStack.size() < (orderedChains.size() + 1))
    {
      uint32_t chainToLayoutIdx = layoutStack.size() - 1;

      LayoutCollection const& baseLayoutCol = layoutStack.back(); 
        
      LayoutCollection nextCollection;

      uint32_t const s_NumAttempts = 50;
      uint32_t const s_CollectionSize = 15;

      for(uint32_t attempt = 0; attempt < s_NumAttempts && nextCollection.size() < s_CollectionSize; ++attempt)
      {
        Layout const& baseLayout = baseLayoutCol[attempt % baseLayoutCol.size()];
        Layout curLayout = baseLayout;
        Set<Vertex> vtxToConsider;
        // Copy chain to layout because vtx might not be sorted in connection order.
        Vector<Vertex> chainToLayout = orderedChains[chainToLayoutIdx];

        Map<Vertex, uint32_t> vtxToRoom;

        Vector<RoomIndexEntry> roomEntries;
        for(unsigned int i = 0; i<curLayout.size(); ++i)
        {
          roomEntries.push_back(std::make_pair(curLayout[i].m_Box, i));
          vtxToConsider.insert(curLayout[i].m_Node);
          vtxToRoom.insert(std::make_pair(curLayout[i].m_Node, i));
        }
        
        RoomIndex roomIndex(roomEntries.begin(), roomEntries.end());
        // Initial layout is intersection free.

        //Only consider current layout + chain vtx.
        boost::filtered_graph<decltype(graph), boost::keep_all, GraphFilter> filteredGr(graph, boost::keep_all(), GraphFilter(vtxToConsider));

        float curEnergy = 0.0;
        Map<uint32_t, RoomViolation> violatingRoom;

        uint32_t expectedSize = curLayout.size() + chainToLayout.size();

        for(int i = 0; i<chainToLayout.size(); ++i)
        {
          auto chainVtx = chainToLayout[i];

          if(vtxToConsider.count(chainVtx))
          {
            // Chain extermities are repeated
            chainToLayout[i] = chainToLayout.back();
            chainToLayout.pop_back();
            --expectedSize;
            i = -1;
            continue;
          }

          vtxToConsider.insert(chainVtx);

          Vector<Segmenti> confSpace;
          Vector<Segmenti> maxConfSpace;
          vtxToConsider.insert(chainVtx);

          Vec2i roomSize = SampleRoom(iGraph, chainVtx, iRand);

          if(curLayout.empty())
          {
            Room newRoom;
            newRoom.m_Node = chainVtx;
            newRoom.m_Box = AABB2Di::FromMinAndSize(Zero<Vec2i>(), roomSize);
            vtxToRoom[chainVtx] = curLayout.size();
            roomIndex.insert(RoomIndexEntry(newRoom.m_Box, curLayout.size()));
            curLayout.push_back(newRoom);

            chainToLayout[i] = chainToLayout.back();
            chainToLayout.pop_back();
            i = -1;

            continue;
          }
          else
          {
            uint32_t numNeigh = 0;
            for(auto edge : OutEdgesIter(filteredGr, chainVtx))
            {
              auto target = GetTarget(chainVtx, edge);

              auto const& neightRoom = curLayout[vtxToRoom[target]].m_Box;
              
              AddRoomConfSpace(roomSize, neightRoom, confSpace);
              ++numNeigh;
            }

            if(numNeigh == 0)
            {
              vtxToConsider.erase(chainVtx);
              continue;
            }
            else
            {
              // Found a way to connect the graph.
              chainToLayout[i] = chainToLayout.back();
              chainToLayout.pop_back();
              i = -1;
            }

            if(numNeigh > 1)
            {
              ComputeMaxConfSpace(confSpace);
            }
          }

          if(!confSpace.empty())
          {
            Room newRoom;
            newRoom.m_Node = chainVtx;
            newRoom.m_Box = SampleConfigurationSpace(roomSize, iRand, confSpace, roomIndex);

            uint32_t newRoomIdx = curLayout.size();

            vtxToRoom[chainVtx] = newRoomIdx;
            curLayout.push_back(newRoom);
            vtxToConsider.insert(chainVtx);

            InsertRoomAndUpdateViolations(chainVtx, newRoomIdx, newRoom.m_Box, roomIndex, violatingRoom, filteredGr, vtxToRoom, curLayout);
          }
          else
          {
            // This layout cannot be extended.
            break;
          }

          if(!violatingRoom.empty())
          {
            RoomIndex wkIndex = roomIndex;
            Layout wkLayout = curLayout;

            // Block area is between (3*3) and (6*6), so let's just take 15 as an average for now.
            float const s_AreaFactor = 15 * 100;
            uint32_t s_MaxTrials = 500;

            auto ComputeTotalEnergy = [&violatingRoom, s_AreaFactor] ()
            {
              float sumCol = 0;
              float sumDist = 0;
              for(auto const& room : violatingRoom)
              {
                for(auto const& overlap : room.second.m_Overlaps)
                {
                  sumCol += overlap.second;
                }
                for(auto const& missingDoor : room.second.m_MissingDoor)
                {
                  sumDist += missingDoor.second;
                }
              }

              return exp(sumCol / s_AreaFactor) * exp(sumDist / s_AreaFactor) - 1;
            };

            float prevEnergy = ComputeTotalEnergy();
            float const initTemp = 0.6;
            float const finalTemp = 0.2;
            float const tempRatio = pow(0.2 / 0.6, 1.0 / float(s_MaxTrials));
            float const kB = 1.38064852e10-23;
            float temp = initTemp;

            Vector<uint32_t> candidates;
            candidates.reserve(curLayout.size());

            for(uint32_t trial = 0; trial < s_MaxTrials && !violatingRoom.empty(); ++trial)
            {
              candidates.clear();
              for(auto room : violatingRoom)
              {
                candidates.push_back(room.first);
              }

              uint32_t roomIdx = candidates[iRand() % candidates.size()];
              Room& curRoom = wkLayout[roomIdx];
              AABB2Di prevRoom = curRoom.m_Box;
              RemoveRoomAndUpdateViolations(roomIdx, curRoom.m_Box, wkIndex, violatingRoom);

              Vec2i roomSize = curRoom.m_Box.GetSize();

              bool changeRoom = (iRand() % 10) > 7;
              if(changeRoom)
              {
                roomSize = SampleRoom(iGraph, curRoom.m_Node, iRand);
              }

              uint32_t numNeigh = 0;
              for(auto edge : OutEdgesIter(filteredGr, curRoom.m_Node))
              {
                auto target = GetTarget(curRoom.m_Node, edge);

                auto const& neightRoom = curLayout[vtxToRoom[target]].m_Box;

                AddRoomConfSpace(roomSize, neightRoom, confSpace);
                ++numNeigh;
              }

              if(numNeigh > 1)
              {
                ComputeMaxConfSpace(confSpace);
              }

              if(confSpace.empty())
              {
                InsertRoomAndUpdateViolations(curRoom.m_Node, roomIdx, curRoom.m_Box, wkIndex, violatingRoom, filteredGr, vtxToRoom, wkLayout);
                continue;
              }
              else
              {
                curRoom.m_Box = SampleConfigurationSpace(roomSize, iRand, confSpace, wkIndex);
                InsertRoomAndUpdateViolations(curRoom.m_Node, roomIdx, curRoom.m_Box, wkIndex, violatingRoom, filteredGr, vtxToRoom, wkLayout);
              }

              float curEnergy = ComputeTotalEnergy();
              float deltaEnergy = curEnergy - prevEnergy;

              if(deltaEnergy <= Mathf::ZeroTolerance()
              || exp(deltaEnergy / (temp * kB)) > iRand())
              {
                //Accept move.
                roomIndex = wkIndex;
                curLayout = wkLayout;
                prevEnergy = curEnergy;
              }
              else
              {
                RemoveRoomAndUpdateViolations(roomIdx, curRoom.m_Box, wkIndex, violatingRoom);
                curRoom.m_Box = prevRoom;
                InsertRoomAndUpdateViolations(curRoom.m_Node, roomIdx, curRoom.m_Box, wkIndex, violatingRoom, filteredGr, vtxToRoom, wkLayout);
              }
              temp *= tempRatio;
            }
          }
        }

        if(violatingRoom.empty() && curLayout.size() == expectedSize)
        {
          nextCollection.push_back(curLayout);
        }
        else
        {
          //printf("Blah");
        }
      }

      if(nextCollection.empty())
      {
        if(!layoutStack.empty() /*&& layoutStack.back().empty()*/)
        {
          // Could not build on top on previous conf, backtrack.
          layoutStack.pop_back();
        }
      }
      else
      {
        layoutStack.emplace_back(std::move(nextCollection));
      }
    }

    if(!layoutStack.empty())
    {
      return layoutStack.back();
    }
    else 
    {
      return LayoutCollection();
    }
  }
}