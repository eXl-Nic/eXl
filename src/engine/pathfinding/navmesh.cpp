#include <engine/pathfinding/navmesh.hpp>
#include <boost/graph/connected_components.hpp>
#include <engine/common/graphutils.hpp>
#include <math/mathtools.hpp>
#include <boost/graph/astar_search.hpp>
#include <engine/pathfinding/penumbratools.hpp>

namespace eXl
{
  Vector2f NavMesh::Face::GetTreadmillDir(Vector2f const& iPos) const
  {
    Vector2f faceCenter = m_Box.GetCenter();
    Vector2f halfSize = m_Box.GetSize() * 0.5;
    Vector2f otherCorner(halfSize.X(), -halfSize.Y());
    Vector2f relPos = iPos - faceCenter;

    static const Vector2f treadMillDirs[] = 
    {
      -Vector2f::UNIT_Y,
       Vector2f::UNIT_X,
       Vector2f::UNIT_Y,
      -Vector2f::UNIT_X
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
    int32_t sign = relPos.m_Data[otherAxis] > 0 ? 1 : 0;
    quadrant = otherAxis + sign * 2;
    if (m_HasSide[quadrant])
    {
      return treadMillDirs[quadrant];
    }

    return MathTools::GetPerp(relPos);
  };

  template <typename Real>
  boost::optional<Segment<Real>> Touch(AABB2D<Real> const& iBox1, AABB2D<Real> const& iBox2)
  {
    Segment<Real> res;

    if(iBox2.m_Data[1].Y() - iBox1.m_Data[0].Y() >= Math<Real>::ZERO_TOLERANCE && iBox1.m_Data[1].Y() - iBox2.m_Data[0].Y() >= Math<Real>::ZERO_TOLERANCE)
    {
      res.m_Ext1.Y() = Math<Real>::Max(iBox1.m_Data[0].Y(), iBox2.m_Data[0].Y());
      res.m_Ext2.Y() = Math<Real>::Min(iBox1.m_Data[1].Y(), iBox2.m_Data[1].Y());

      Real val = iBox1.m_Data[0].X() - iBox2.m_Data[1].X();
      if(val <= Math<Real>::ZERO_TOLERANCE && val >= -Math<Real>::ZERO_TOLERANCE)
      {
        res.m_Ext1.X() = res.m_Ext2.X() = iBox1.m_Data[0].X();
        return res;
      }

      val = iBox1.m_Data[1].X() - iBox2.m_Data[0].X();
      if(val <= Math<Real>::ZERO_TOLERANCE && val >= -Math<Real>::ZERO_TOLERANCE)
      {
        res.m_Ext1.X() = res.m_Ext2.X() = iBox1.m_Data[1].X();
        return res;
      }
    }

    if(iBox2.m_Data[1].X() - iBox1.m_Data[0].X() >= Math<Real>::ZERO_TOLERANCE && iBox1.m_Data[1].X() - iBox2.m_Data[0].X() >= Math<Real>::ZERO_TOLERANCE)
    {
      res.m_Ext1.X() = Math<Real>::Max(iBox1.m_Data[0].X(), iBox2.m_Data[0].X());
      res.m_Ext2.X() = Math<Real>::Min(iBox1.m_Data[1].X(), iBox2.m_Data[1].X());

      Real val = iBox1.m_Data[0].Y() - iBox2.m_Data[1].Y();
      if(val <= Math<Real>::ZERO_TOLERANCE && val >= -Math<Real>::ZERO_TOLERANCE)
      {
        res.m_Ext1.Y() = res.m_Ext2.Y() = iBox1.m_Data[0].Y(); 
        return res;
      }

      val = iBox1.m_Data[1].Y() - iBox2.m_Data[0].Y();
      if(val <= Math<Real>::ZERO_TOLERANCE && val >= -Math<Real>::ZERO_TOLERANCE)
      {
        res.m_Ext1.Y() = res.m_Ext2.Y() = iBox1.m_Data[1].Y(); 
        return res;
      }
    }

    return boost::none;
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
      queryBox.m_Data[0] -= Vector2f::ONE;
      queryBox.m_Data[1] += Vector2f::ONE;

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

      Vector2f faceDim = face.m_Box.GetSize();

      Vector2f corners[] = 
      {
        face.m_Box.m_Data[0],
        face.m_Box.m_Data[0] + faceDim.X() * Vector2f::UNIT_X,
        face.m_Box.m_Data[0] + faceDim.Y() * Vector2f::UNIT_Y,
        face.m_Box.m_Data[0] + faceDim.X() * Vector2f::UNIT_X + faceDim.Y() * Vector2f::UNIT_Y
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

        Vector2f vec = (seg1.segment.m_Ext1 + seg1.segment.m_Ext2) * 0.5 - (seg2.segment.m_Ext1 + seg2.segment.m_Ext2) * 0.5;
        boost::property_map<Graph, boost::edge_weight_t>::type propMapW = boost::get(boost::edge_weight, curRes.m_Graph);

        boost::put(propMapW, edgeId.first, vec.Length());
      }
    }

    Penumbra openings;

    for(auto& component : resComps)
    {
      for(auto& face : component.m_Faces)
      {
        Vector2f faceCenter = face.m_Box.GetCenter();
        Vector2f faceDim = face.m_Box.GetSize();

        if(face.m_Edges.empty())
        {
          Vector2f corners[] = 
          {
            face.m_Box.m_Data[0],
            face.m_Box.m_Data[0] + faceDim.X() * Vector2f::UNIT_X,
            face.m_Box.m_Data[0] + faceDim.Y() * Vector2f::UNIT_Y,
            face.m_Box.m_Data[0] + faceDim.X() * Vector2f::UNIT_X + faceDim.Y() * Vector2f::UNIT_Y
          };

          face.m_Walls.push_back(Segmentf({corners[2], corners[0]}));
          face.m_Walls.push_back(Segmentf({corners[0], corners[1]}));
          face.m_Walls.push_back(Segmentf({corners[1], corners[3]}));
          face.m_Walls.push_back(Segmentf({corners[3], corners[2]}));
          for (auto& val : face.m_HasSide) { val = true; }
          continue;
        }

        Vector2f ccwDirs[4] =
        {
           Vector2f::UNIT_Y,
          -Vector2f::UNIT_X,
          -Vector2f::UNIT_Y,
           Vector2f::UNIT_X
        };

        for (auto& val : face.m_HasSide) { val = false; }
        for(uint32_t dirIdx = 0; dirIdx < 4; ++dirIdx)
        {
          float sign = dirIdx >= 2 ? -1.0: 1.0;
          int32_t axis = dirIdx % 2;
          
          Vector2f dir;
          dir.m_Data[axis] = sign;

          openings.Start(faceCenter, 0.0, dir, faceDim.SquaredLength());

          openings.AddSegment(Segmentf({faceCenter - dir, faceCenter + faceDim.m_Data[axis] * dir - faceDim.m_Data[1 - axis] * MathTools::Perp(dir)}), 0.0);
          openings.AddSegment(Segmentf({faceCenter - dir, faceCenter + faceDim.m_Data[axis] * dir + faceDim.m_Data[1 - axis] * MathTools::Perp(dir)}), 0.0);

          for(auto segIdx : face.m_Edges)
          {
            Segmentf const& seg = component.m_FaceEdges[segIdx].segment;
            Vector2f segDir = seg.m_Ext2 - seg.m_Ext1;
            int32_t segAxis = segDir.X() == 0 ? 0 : 1;
            float segSign = Mathf::Sign(seg.m_Ext1.m_Data[axis] - faceCenter.m_Data[axis]);

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
            newWall.m_Ext1.X() = faceDim.m_Data[axis] * 0.5;
            newWall.m_Ext1.Y() = newWall.m_Ext1.X() * localRanges.m_Ext1.Y() / localRanges.m_Ext1.X();
            newWall.m_Ext1 = faceCenter + newWall.m_Ext1.X() * dir + newWall.m_Ext1.Y() * MathTools::GetPerp(dir);

            newWall.m_Ext2.X() = faceDim.m_Data[axis] * 0.5;
            newWall.m_Ext2.Y() = newWall.m_Ext2.X() * localRanges.m_Ext2.Y() / localRanges.m_Ext2.X();
            newWall.m_Ext2 = faceCenter + newWall.m_Ext2.X() * dir + newWall.m_Ext2.Y() * MathTools::GetPerp(dir);

            if ((newWall.m_Ext2 - newWall.m_Ext1).Dot(ccwDirs[dirIdx]) < 0)
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

  boost::optional<NavMesh::FoundFace> NavMesh::FindFace(Vector2f const& iPos) const
  {
    Vector<BoxIndexEntry> results;
    AABB2Df queryBox;
    queryBox.m_Data[0] = iPos - Vector2f::ONE;
    queryBox.m_Data[1] = iPos + Vector2f::ONE;

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

    return boost::none;
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
      distance_heuristic(NavMesh::EdgeMap const& iEdges, Vector2f iGoalPt) 
        : m_Edges(iEdges)
        , m_GoalPt(iGoalPt)
      {}

      float operator()(Vertex u)
      {
        eXl_ASSERT(u < m_Edges.size());
        auto const& desc = m_Edges[u];
        //if(desc != m_Edges.end())
        {
          Vector2f edgeCenter = (desc.segment.m_Ext1 + desc.segment.m_Ext2) * 0.5;

          return (edgeCenter - m_GoalPt).Length();
        }

        return FLT_MAX;
      }
    private:
      NavMesh::EdgeMap const& m_Edges;
      Vector2f m_GoalPt;
    };
  }

  boost::optional<uint32_t> NavMesh::Edge::CommonFace(Edge const& iOther) const
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
    return boost::none;
  }

  Vector2f GetDirToFace(AABB2Df const& iOrigFace, AABB2Df const& iDestFace)
  {
    int touchId = iOrigFace.Touch(iDestFace);
    switch(touchId)
    {
    case 1:
      return Vector2f::UNIT_X * -1.0;
      break;
    case 2:
      return Vector2f::UNIT_X;
      break;
    case 3:
      return Vector2f::UNIT_Y * -1.0;
      break;
    case 4:
      return Vector2f::UNIT_Y;
      break;
    default:
      //eXl_ASSERT_MSG(false, "Neighbouring faces not touching");
      return Vector2f::ZERO;
      break;
    }
  }

  boost::optional<NavMesh::Path> NavMesh::FindPath(Vector2f const& iStart, Vector2f const& iEnd) const
  {
    boost::optional<FoundFace> faceStartId = FindFace(iStart);
    boost::optional<FoundFace> faceEndId = FindFace(iEnd);

    if(faceStartId && faceEndId)
    {
      if(*faceStartId == *faceEndId)
      {
        return NavMesh::Path();
      }

      if (faceStartId->m_Component != faceEndId->m_Component)
      {
        return boost::none;
      }

      Component const& component = m_Components[faceStartId->m_Component];

      Face const& faceStart = component.m_Faces[faceStartId->m_Face];
      Face const& faceEnd = component.m_Faces[faceEndId->m_Face];

      eXl_ASSERT_REPAIR_RET(!faceStart.m_Edges.empty() && !faceStart.m_Edges.empty(), boost::none)
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
          Vector2f goalPt(iEnd.X(), iEnd.Y());
          Path res;
          res.m_Component = faceStartId->m_Component;
          for(auto v = goal.outEdge;; v = p[v]) 
          {
            res.m_Edges.push_back(v);

            eXl_ASSERT_REPAIR_RET(v < component.m_FaceEdges.size(), boost::none);

            auto const& edgeDesc = component.m_FaceEdges[v];
            //eXl_ASSERT_REPAIR_RET(edgeDesc != component.m_FaceEdges.end(), boost::none)
            {
              uint32_t prevFaceId = edgeDesc.face1 == curFaceId ? edgeDesc.face2 : edgeDesc.face1;

              Segmentf const& seg = edgeDesc.segment;
              auto midPt = (seg.m_Ext1 + seg.m_Ext2) * 0.5;

              auto dirToGoal = goalPt - midPt;

              if(dirToGoal.Length() < Mathf::ZERO_TOLERANCE)
              {
                dirToGoal = (seg.m_Ext1 - seg.m_Ext2);
                dirToGoal.Normalize();
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

            eXl_ASSERT_REPAIR_RET(!(!commonFaceIdx), boost::none);
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

            eXl_ASSERT_REPAIR_RET(!(!commonFaceIdx), boost::none);
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

    return boost::none;
  }
}