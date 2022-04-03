/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/pathfinding/navmesh.hpp>
#include <boost/graph/connected_components.hpp>
#include <engine/common/graphutils.hpp>
#include <math/mathtools.hpp>
#include <boost/graph/astar_search.hpp>
#include <engine/pathfinding/penumbratools.hpp>

namespace eXl
{
  Vec2 NavMesh::Face::GetTreadmillDir(Vec2 const& iPos) const
  {
    Vec2 faceCenter = m_Box.GetCenter();
    Vec2 halfSize = m_Box.GetSize() * 0.5;
    Vec2 otherCorner(halfSize.x, -halfSize.y);
    Vec2 relPos = iPos - faceCenter;

    static const Vec2 treadMillDirs[] = 
    {
      -UnitY<Vec2>(),
       UnitX<Vec2>(),
       UnitY<Vec2>(),
      -UnitX<Vec2>()
    };

    uint32_t quadrant;
    if (Segmentf::IsLeft(-halfSize, halfSize, relPos) > 0)
    {
      if (Segmentf::IsLeft(-otherCorner, otherCorner, relPos) > 0)
      {
        quadrant = 3;
      }
      else
      {
        quadrant = 0;
      }
    }
    else
    {
        if (Segmentf::IsLeft(-otherCorner, otherCorner, relPos) > 0)
        {
          quadrant = 2;
        }
        else
        {
          quadrant = 1;
        }
    }

    if (m_HasSide[quadrant])
    {
      return treadMillDirs[quadrant];
    }

    int32_t axis = quadrant % 2;
    int32_t otherAxis = (1 - axis);
    int32_t sign = relPos[otherAxis] > 0 ? 1 : 0;
    quadrant = otherAxis + sign * 2;
    if (m_HasSide[quadrant])
    {
      return treadMillDirs[quadrant];
    }

    return MathTools::GetPerp(relPos);
  };

  template <typename Real>
  Optional<Segment<Real>> Touch(AABB2D<Real> const& iBox1, AABB2D<Real> const& iBox2)
  {
    Segment<Real> res;

    if(iBox2.m_Data[1].y - iBox1.m_Data[0].y >= Math<Real>::ZeroTolerance() && iBox1.m_Data[1].y - iBox2.m_Data[0].y >= Math<Real>::ZeroTolerance())
    {
      res.m_Ext1.y = Math<Real>::Max(iBox1.m_Data[0].y, iBox2.m_Data[0].y);
      res.m_Ext2.y = Math<Real>::Min(iBox1.m_Data[1].y, iBox2.m_Data[1].y);

      Real val = iBox1.m_Data[0].x - iBox2.m_Data[1].x;
      if(val <= Math<Real>::ZeroTolerance() && val >= -Math<Real>::ZeroTolerance())
      {
        res.m_Ext1.x = res.m_Ext2.x = iBox1.m_Data[0].x;
        return res;
      }

      val = iBox1.m_Data[1].x - iBox2.m_Data[0].x;
      if(val <= Math<Real>::ZeroTolerance() && val >= -Math<Real>::ZeroTolerance())
      {
        res.m_Ext1.x = res.m_Ext2.x = iBox1.m_Data[1].x;
        return res;
      }
    }

    if(iBox2.m_Data[1].x - iBox1.m_Data[0].x >= Math<Real>::ZeroTolerance() && iBox1.m_Data[1].x - iBox2.m_Data[0].x >= Math<Real>::ZeroTolerance())
    {
      res.m_Ext1.x = Math<Real>::Max(iBox1.m_Data[0].x, iBox2.m_Data[0].x);
      res.m_Ext2.x = Math<Real>::Min(iBox1.m_Data[1].x, iBox2.m_Data[1].x);

      Real val = iBox1.m_Data[0].y - iBox2.m_Data[1].y;
      if(val <= Math<Real>::ZeroTolerance() && val >= -Math<Real>::ZeroTolerance())
      {
        res.m_Ext1.y = res.m_Ext2.y = iBox1.m_Data[0].y; 
        return res;
      }

      val = iBox1.m_Data[1].y - iBox2.m_Data[0].y;
      if(val <= Math<Real>::ZeroTolerance() && val >= -Math<Real>::ZeroTolerance())
      {
        res.m_Ext1.y = res.m_Ext2.y = iBox1.m_Data[1].y; 
        return res;
      }
    }

    return {};
  }

  NavMesh NavMesh::MakeFromAABB2DPoly(Vector<AABB2DPolygoni> const& iPolys)
  {
    Vector<AABB2Di> boxes;

    for(auto& poly : iPolys)
    {
      Vector<AABB2Di> polyBoxes;
      poly.GetBoxes(polyBoxes);
      boxes.insert(boxes.end(), polyBoxes.begin(), polyBoxes.end());
    }

    return MakeFromBoxes(boxes);
  }

  NavMesh NavMesh::MakeFromBoxes(Vector<AABB2Di> const& iBoxes)
  {
    BoxIndex index;
    Vector<Face> tempFaces;
    EdgeMap tempEdges;
    Vector<Vector<std::pair<uint32_t, uint32_t>>> edgeConnectivity;

    for(auto const& box : iBoxes)
    {
      auto boxF = AABB2Df(MathTools::ToFVec(box.m_Data[0]), MathTools::ToFVec(box.GetSize()));
      auto value = std::make_pair(boxF, (unsigned int)index.size());
      index.insert(value);
      tempFaces.push_back(Face());
      tempFaces.back().m_Box = boxF;
    }

    Graph tempGraph;
    edgeConnectivity.resize(tempFaces.size());
    //for(unsigned int i = 0; i<index.size(); ++i)
    //{
    //  boost::add_vertex(tempGraph);
    //}

    for(auto& boxEntry : index)
    {
      Vector<BoxIndexEntry> results;

      AABB2Df queryBox = boxEntry.first;
      queryBox.m_Data[0] -= One<Vec2>();
      queryBox.m_Data[1] += One<Vec2>();

      index.query(
        /*boost::geometry::index::satisfies([&box](BoxIndexEntry const& iEntry)
          {
            iEntry.first != box.first && iEntry.first.Touch(box.first) != 0;
          }*/
        boost::geometry::index::intersects(queryBox),
        std::back_inserter(results));

      for(auto& result : results)
      {
        if(result != boxEntry)
        {
          auto res = Touch(result.first, boxEntry.first);
          if(res)
          {
            if(boxEntry.second > result.second)
            {
              Segmentf seg;
              seg.m_Ext1 = MathTools::ToFVec(res->m_Ext1);
              seg.m_Ext2 = MathTools::ToFVec(res->m_Ext2);
              auto newVtx = boost::add_vertex(tempGraph);

              Edge newEdge = 
              {
                result.second,
                boxEntry.second,
                seg
              };
              eXl_ASSERT(tempEdges.size() == newVtx);
              tempEdges.push_back(newEdge);
              edgeConnectivity[boxEntry.second].push_back(std::make_pair(result.second, (uint32_t)newVtx));
            }
          }
        }
      }
    }

    for(unsigned int faceIdx = 0; faceIdx<tempFaces.size(); ++faceIdx)
    {
      auto& edges = edgeConnectivity[faceIdx];
      for(auto const& conn : edges)
      {
        tempFaces[faceIdx].m_Edges.push_back(conn.second);
        tempFaces[conn.first].m_Edges.push_back(conn.second);
      }
    }

    for(auto& face : tempFaces)
    {
      auto const& edges = face.m_Edges;
      for(unsigned int i = 0; i<edges.size(); ++i)
      {
        for(unsigned int j = i; j<edges.size(); ++j)
        {
          boost::add_edge(edges[i], edges[j], tempGraph);
        }
      }
    }

    if(boost::num_edges(tempGraph) == 0)
    {
      //Limited to a single face.
      NavMesh result;
      result.m_Components.push_back(Component());

      Component& resMesh = result.m_Components.back();

      resMesh.m_Graph = tempGraph;
      resMesh.m_Faces.push_back(Face());

      Face& face = resMesh.m_Faces.back();
      face.m_Box = tempFaces[0].m_Box;

      Vec2 faceDim = face.m_Box.GetSize();

      Vec2 corners[] = 
      {
        face.m_Box.m_Data[0],
        face.m_Box.m_Data[0] + faceDim.x * UnitX<Vec2>(),
        face.m_Box.m_Data[0] + faceDim.y * UnitY<Vec2>(),
        face.m_Box.m_Data[0] + faceDim.x * UnitX<Vec2>() + faceDim.y * UnitY<Vec2>()
      };

      face.m_Walls.push_back(Segmentf({corners[0], corners[1]}));
      face.m_Walls.push_back(Segmentf({corners[0], corners[2]}));
      face.m_Walls.push_back(Segmentf({corners[3], corners[1]}));
      face.m_Walls.push_back(Segmentf({corners[3], corners[2]}));
      for (auto& val : face.m_HasSide) { val = true; }

      
      resMesh.m_FacesIdx.insert(std::make_pair(tempFaces[0].m_Box, 0));

      return result;
    }

    IndexMap<Graph> componentMap;
    unsigned int numComponents = boost::connected_components(tempGraph, componentMap);

    Vector<uint32_t> numVtx(numComponents, 0);
    Vector<uint32_t> numFaces(numComponents, 0);
    Vector<uint32_t> newId(boost::num_vertices(tempGraph), 0);
    Vector<uint32_t> newFaceId(tempFaces.size(), UINT32_MAX);
    Vector<Component> resComps(numComponents);

    
    for(uint32_t vtxId = 0; vtxId < tempEdges.size(); ++vtxId)
    {
      uint32_t compNum = componentMap.m_Map[vtxId];

      newId[vtxId] = numVtx[compNum]++;

      auto& curRes = resComps[compNum];

      unsigned int newVtxId = boost::add_vertex(curRes.m_Graph);
      auto const& edgeDesc = tempEdges[vtxId];

      if(newFaceId[edgeDesc.face1] == UINT32_MAX)
      {
        newFaceId[edgeDesc.face1] = numFaces[compNum]++;
        curRes.m_Faces.push_back(Face());
        curRes.m_Faces.back().m_Box = tempFaces[edgeDesc.face1].m_Box;
        curRes.m_FacesIdx.insert(std::make_pair(curRes.m_Faces.back().m_Box, newFaceId[edgeDesc.face1]));
      }

      curRes.m_Faces[newFaceId[edgeDesc.face1]].m_Edges.push_back(newVtxId);

      if(newFaceId[edgeDesc.face2] == UINT32_MAX)
      {
        newFaceId[edgeDesc.face2] = numFaces[compNum]++;
        curRes.m_Faces.push_back(Face());
        curRes.m_Faces.back().m_Box = tempFaces[edgeDesc.face2].m_Box;
        curRes.m_FacesIdx.insert(std::make_pair(curRes.m_Faces.back().m_Box, newFaceId[edgeDesc.face2]));
      }

      curRes.m_Faces[newFaceId[edgeDesc.face2]].m_Edges.push_back(newVtxId);

      Edge newEdgeDesc = 
      {
        newFaceId[edgeDesc.face1],
        newFaceId[edgeDesc.face2],
        edgeDesc.segment
      };

      eXl_ASSERT(newVtxId == curRes.m_FaceEdges.size());
      curRes.m_FaceEdges.push_back(newEdgeDesc);
    }

    for(auto vertices = boost::vertices(tempGraph); vertices.first != vertices.second; ++vertices.first)
    {
      unsigned int curVtx = *vertices.first;
      unsigned int compNum = componentMap.m_Map[curVtx];
      unsigned int newCurVtx = newId[curVtx];

      auto& curRes = resComps[compNum];

      for(auto edges = boost::out_edges(curVtx, tempGraph); edges.first != edges.second; ++edges.first)
      {
        auto otherVtx = edges.first->m_source == curVtx ? edges.first->m_target : edges.first->m_source;
        unsigned int newOtherVtx = newId[otherVtx];
        auto edgeId = boost::add_edge(newCurVtx, newOtherVtx, curRes.m_Graph);

        auto const& seg1 = curRes.m_FaceEdges[curVtx];
        auto const& seg2 = curRes.m_FaceEdges[otherVtx];

        Vec2 vec = (seg1.segment.m_Ext1 + seg1.segment.m_Ext2) * 0.5 - (seg2.segment.m_Ext1 + seg2.segment.m_Ext2) * 0.5;
        boost::property_map<Graph, boost::edge_weight_t>::type propMapW = boost::get(boost::edge_weight, curRes.m_Graph);

        boost::put(propMapW, edgeId.first, length(vec));
      }
    }

    Penumbra openings;

    for(auto& component : resComps)
    {
      for(auto& face : component.m_Faces)
      {
        Vec2 faceCenter = face.m_Box.GetCenter();
        Vec2 faceDim = face.m_Box.GetSize();

        if(face.m_Edges.empty())
        {
          Vec2 corners[] = 
          {
            face.m_Box.m_Data[0],
            face.m_Box.m_Data[0] + faceDim.x * UnitX<Vec2>(),
            face.m_Box.m_Data[0] + faceDim.y * UnitY<Vec2>(),
            face.m_Box.m_Data[0] + faceDim.x * UnitX<Vec2>() + faceDim.y * UnitY<Vec2>()
          };

          face.m_Walls.push_back(Segmentf({corners[2], corners[0]}));
          face.m_Walls.push_back(Segmentf({corners[0], corners[1]}));
          face.m_Walls.push_back(Segmentf({corners[1], corners[3]}));
          face.m_Walls.push_back(Segmentf({corners[3], corners[2]}));
          for (auto& val : face.m_HasSide) { val = true; }
          continue;
        }

        Vec2 ccwDirs[4] =
        {
           UnitY<Vec2>(),
          -UnitX<Vec2>(),
          -UnitY<Vec2>(),
           UnitX<Vec2>()
        };

        for (auto& val : face.m_HasSide) { val = false; }
        for(uint32_t dirIdx = 0; dirIdx < 4; ++dirIdx)
        {
          float sign = dirIdx >= 2 ? -1.0: 1.0;
          int32_t axis = dirIdx % 2;
          
          Vec2 dir;
          dir[axis] = sign;

          openings.Start(faceCenter, 0.0, dir, length2(faceDim));

          openings.AddSegment(Segmentf({faceCenter - dir, faceCenter + faceDim[axis] * dir - faceDim[1 - axis] * MathTools::Perp(dir)}), 0.0);
          openings.AddSegment(Segmentf({faceCenter - dir, faceCenter + faceDim[axis] * dir + faceDim[1 - axis] * MathTools::Perp(dir)}), 0.0);

          for(auto segIdx : face.m_Edges)
          {
            Segmentf const& seg = component.m_FaceEdges[segIdx].segment;
            Vec2 segDir = seg.m_Ext2 - seg.m_Ext1;
            int32_t segAxis = segDir.x == 0 ? 0 : 1;
            float segSign = Mathf::Sign(seg.m_Ext1[axis] - faceCenter[axis]);

            if(segAxis == axis && segSign == sign)
            {
              openings.AddSegment(component.m_FaceEdges[segIdx].segment, 0.0);
            }
          }

          Vector<Segmentf> ranges = openings.GetAllOpenings();
          if (!ranges.empty())
          {
            face.m_HasSide[dirIdx] = true;
          }
          for(auto const& range : ranges)
          {
            Segmentf localRanges;
            localRanges.m_Ext1 = MathTools::GetLocal(dir, range.m_Ext1);
            localRanges.m_Ext2 = MathTools::GetLocal(dir, range.m_Ext2);

            face.m_Walls.push_back(Segmentf());
            Segmentf& newWall = face.m_Walls.back();
            newWall.m_Ext1.x = faceDim[axis] * 0.5;
            newWall.m_Ext1.y = newWall.m_Ext1.x * localRanges.m_Ext1.y / localRanges.m_Ext1.x;
            newWall.m_Ext1 = faceCenter + newWall.m_Ext1.x * dir + newWall.m_Ext1.y * MathTools::GetPerp(dir);

            newWall.m_Ext2.x = faceDim[axis] * 0.5;
            newWall.m_Ext2.y = newWall.m_Ext2.x * localRanges.m_Ext2.y / localRanges.m_Ext2.x;
            newWall.m_Ext2 = faceCenter + newWall.m_Ext2.x * dir + newWall.m_Ext2.y * MathTools::GetPerp(dir);

            if (dot(newWall.m_Ext2 - newWall.m_Ext1, ccwDirs[dirIdx]) < 0)
            {
              std::swap(newWall.m_Ext2, newWall.m_Ext1);
            }
          }
        }
      }
    }
    
    NavMesh result;
    result.m_Components = std::move(resComps);

    return result;
  }

  Optional<NavMesh::FoundFace> NavMesh::FindFace(Vec2 const& iPos) const
  {
    Vector<BoxIndexEntry> results;
    AABB2Df queryBox;
    queryBox.m_Data[0] = iPos - One<Vec2>();
    queryBox.m_Data[1] = iPos + One<Vec2>();

    for (uint32_t i = 0; i< m_Components.size(); ++i)
    {
      auto const& component = m_Components[i];
      component.m_FacesIdx.query(
        boost::geometry::index::intersects(queryBox),
        std::back_inserter(results));

      for (auto const& res : results)
      {
        if (res.first.Contains(iPos, 0))
        {
          FoundFace face = { i, res.second };
          return face;
        }
      }
    }

    return {};
  }

  namespace 
  {
    struct found_goal 
    {
      NavMesh::Graph::vertex_descriptor outEdge;
    };

    class astar_goal_visitor : public boost::default_astar_visitor
    {
    public:
      inline astar_goal_visitor(NavMesh::EdgeMap const& iEdges, uint32_t iGoalFace) 
        : m_Edges(iEdges)
        , m_Goal(iGoalFace)
      {
      }

      void examine_vertex(NavMesh::Graph::vertex_descriptor u, NavMesh::Graph const& g) 
      {
        eXl_ASSERT(u < m_Edges.size());
        auto const& desc = m_Edges[u];
        //if(desc != m_Edges.end())
        {
          if(desc.face1 == m_Goal
          || desc.face2 == m_Goal)
          {
            found_goal goal = {u};
            throw goal;
          }
        }
      }
    private:
      NavMesh::EdgeMap const& m_Edges;
      uint32_t m_Goal;
    };

    class distance_heuristic : public boost::astar_heuristic<NavMesh::Graph, float>
    {
    public:
      typedef boost::graph_traits<NavMesh::Graph>::vertex_descriptor Vertex;
      distance_heuristic(NavMesh::EdgeMap const& iEdges, Vec2 iGoalPt) 
        : m_Edges(iEdges)
        , m_GoalPt(iGoalPt)
      {}

      float operator()(Vertex u)
      {
        eXl_ASSERT(u < m_Edges.size());
        auto const& desc = m_Edges[u];
        //if(desc != m_Edges.end())
        {
          Vec2 edgeCenter = (desc.segment.m_Ext1 + desc.segment.m_Ext2) * 0.5;

          return length(edgeCenter - m_GoalPt);
        }

        return Mathf::MaxReal();
      }
    private:
      NavMesh::EdgeMap const& m_Edges;
      Vec2 m_GoalPt;
    };
  }

  Optional<uint32_t> NavMesh::Edge::CommonFace(Edge const& iOther) const
  {
    if(face1 == iOther.face1
    || face1 == iOther.face2)
    {
      return face1;
    }
    else 
    if(face2 == iOther.face1
    || face2 == iOther.face2)
    {
      return face2;
    }
    return {};
  }

  Vec2 GetDirToFace(AABB2Df const& iOrigFace, AABB2Df const& iDestFace)
  {
    int touchId = iOrigFace.Touch(iDestFace);
    switch(touchId)
    {
    case 1:
      return UnitX<Vec2>() * -1.0;
      break;
    case 2:
      return UnitX<Vec2>();
      break;
    case 3:
      return UnitY<Vec2>() * -1.0;
      break;
    case 4:
      return UnitY<Vec2>();
      break;
    default:
      //eXl_ASSERT_MSG(false, "Neighbouring faces not touching");
      return Zero<Vec2>();
      break;
    }
  }

  Optional<NavMesh::Path> NavMesh::FindPath(Vec2 const& iStart, Vec2 const& iEnd) const
  {
    Optional<FoundFace> faceStartId = FindFace(iStart);
    Optional<FoundFace> faceEndId = FindFace(iEnd);

    if(faceStartId && faceEndId)
    {
      if(*faceStartId == *faceEndId)
      {
        return NavMesh::Path();
      }

      if (faceStartId->m_Component != faceEndId->m_Component)
      {
        return {};
      }

      Component const& component = m_Components[faceStartId->m_Component];

      Face const& faceStart = component.m_Faces[faceStartId->m_Face];
      Face const& faceEnd = component.m_Faces[faceEndId->m_Face];

      eXl_ASSERT_REPAIR_RET(!faceStart.m_Edges.empty() && !faceStart.m_Edges.empty(), {})
      {
        IndexMap<Graph> outIndex(component.m_Graph);

        auto startVtx = faceStart.m_Edges.front();

        Vector<Graph::vertex_descriptor> p(boost::num_vertices(component.m_Graph), startVtx);
        try
        {
          boost::astar_search_tree
          (component.m_Graph, startVtx,
            distance_heuristic(component.m_FaceEdges, iEnd),
            boost::predecessor_map(boost::make_iterator_property_map(p.begin(), outIndex)).
            visitor(astar_goal_visitor(component.m_FaceEdges, faceEndId->m_Face)));
        }
        catch(found_goal& goal)
        {
          uint32_t curFaceId = faceEndId->m_Face;
          Vec2 goalPt(iEnd.x, iEnd.y);
          Path res;
          res.m_Component = faceStartId->m_Component;
          for(auto v = goal.outEdge;; v = p[v]) 
          {
            res.m_Edges.push_back(v);

            eXl_ASSERT_REPAIR_RET(v < component.m_FaceEdges.size(), {});

            auto const& edgeDesc = component.m_FaceEdges[v];
            //eXl_ASSERT_REPAIR_RET(edgeDesc != component.m_FaceEdges.end(), {})
            {
              uint32_t prevFaceId = edgeDesc.face1 == curFaceId ? edgeDesc.face2 : edgeDesc.face1;

              Segmentf const& seg = edgeDesc.segment;
              auto midPt = (seg.m_Ext1 + seg.m_Ext2) * 0.5;

              auto dirToGoal = goalPt - midPt;

              if(length(dirToGoal) < Mathf::ZeroTolerance())
              {
                dirToGoal = normalize(seg.m_Ext1 - seg.m_Ext2);
                res.m_EdgeDirs.push_back(dirToGoal);
              }
              else
              {
                Face const& curFace = component.m_Faces[curFaceId];
                Face const& prevFace = component.m_Faces[prevFaceId];

                auto perpDir = GetDirToFace(prevFace.m_Box, curFace.m_Box);

                res.m_EdgeDirs.push_back(perpDir);
              }

              goalPt = midPt;
              curFaceId = prevFaceId;
            }

            if(p[v] == v)
              break;
          }

          if(res.m_Edges.size() >= 2)
          {
            auto const& edge1 = component.m_FaceEdges[*(--res.m_Edges.end())];
            auto const& edge2 = component.m_FaceEdges[*(--(--res.m_Edges.end()))];

            auto commonFaceIdx = edge1.CommonFace(edge2);

            eXl_ASSERT_REPAIR_RET(!(!commonFaceIdx), {});
            {
              if(*commonFaceIdx == faceStartId->m_Face)
              {
                res.m_Edges.pop_back();
                res.m_EdgeDirs.pop_back();
              }
            }
          }

          if(res.m_Edges.size() >= 2)
          {
            auto const& edge1 = component.m_FaceEdges[*(res.m_Edges.begin())];
            auto const& edge2 = component.m_FaceEdges[*(++res.m_Edges.begin())];

            auto commonFaceIdx = edge1.CommonFace(edge2);

            eXl_ASSERT_REPAIR_RET(!(!commonFaceIdx), {});
            {
              if(*commonFaceIdx == faceEndId->m_Face)
              {
                res.m_Edges.erase(res.m_Edges.begin());
                res.m_EdgeDirs.erase(res.m_EdgeDirs.begin());
              }
            }
          }

          return res;
        }
      }
    }

    return {};
  }
}