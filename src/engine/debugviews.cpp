
#include "debugviews.hpp"

#include <engine/common/debugtool.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/neighbours.hpp>

#include <engine/pathfinding/navmesh.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/map/dungeonlayout.hpp>
#include <math/mathtools.hpp>

namespace eXl
{

  //void DrawPath(HamiltonianPath& iPath, GfxDebugDrawer& iDrawer)
  //{
  //  unsigned int curPt = iPath.GetStart();
  //  unsigned int prevPt = curPt;
  //  unsigned int end = iPath.GetEnd();
  //  Vector2i prevPos = iPath.GetPoint(curPt).pos;
  //
  //  while(curPt != end)
  //  {
  //    auto& point = iPath.GetPoint(curPt);
  //
  //    iDrawer.DrawLine(Vec3(prevPos.x, prevPos.y, 0.0), Vec3(point.pos.x, point.pos.y, 0.0), Vec4(1.0, 0.0, 1.0, 1.0));
  //    unsigned int nextPt = point.neigh1 == prevPt ? point.neigh2 : point.neigh1;
  //    prevPt = curPt;
  //    curPt = nextPt;
  //    prevPos = point.pos;
  //  }
  //}

  void DrawRooms(Vector<Room> const& iRooms, DebugTool::Drawer& iDrawer)
  {
    for (auto& face : iRooms)
    {
      auto const& box = face.m_Box;

      Vec3 boxMin(box.m_Data[0].x, box.m_Data[0].y, 0.0);
      Vec3 boxMax(box.m_Data[1].x, box.m_Data[1].y, 0.0);

      iDrawer.DrawLine(boxMin, Vec3(boxMin.x, boxMax.y, 0), Vec4(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMin, Vec3(boxMax.x, boxMin.y, 0), Vec4(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMax, Vec3(boxMin.x, boxMax.y, 0), Vec4(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMax, Vec3(boxMax.x, boxMin.y, 0), Vec4(1.0, 0.0, 1.0, 1.0));
    }
  }

  void DrawNavMesh(NavMesh const& iMesh, DebugTool::Drawer& iDrawer)
  {
    for (auto& component : iMesh.GetComponents())
    {
      for (auto& face : component.m_Faces)
      {
        auto const& box = face.m_Box;

        Vec3 boxMin = Vec3(box.m_Data[0], 0);
        Vec3 boxMax = Vec3(box.m_Data[1], 0);

        iDrawer.DrawLine(boxMin, Vec3(boxMin.x, boxMax.y, 0.0), Vec4(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMin, Vec3(boxMax.x, boxMin.y, 0.0), Vec4(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMax, Vec3(boxMin.x, boxMax.y, 0.0), Vec4(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMax, Vec3(boxMax.x, boxMin.y, 0.0), Vec4(1.0, 0.0, 1.0, 1.0));

        for (auto const& seg : face.m_Walls)
        {
          iDrawer.DrawLine(Vec3(seg.m_Ext1, 0), Vec3(seg.m_Ext2, 0), Vec4(1.0, 1.0, 1.0, 1.0));
        }
      }

      for (auto vertices = boost::vertices(component.m_Graph); vertices.first != vertices.second; ++vertices.first)
      {
        auto curVtx = *vertices.first;
        auto const& curSeg = component.m_FaceEdges[curVtx].segment;
        Vec3 curMidSeg((curSeg.m_Ext1.x + curSeg.m_Ext2.x) * 0.5,
          (curSeg.m_Ext1.y + curSeg.m_Ext2.y) * 0.5,
          0.0);

        for (auto edges = boost::out_edges(curVtx, component.m_Graph); edges.first != edges.second; ++edges.first)
        {
          auto otherVtx = edges.first->m_source == curVtx ? edges.first->m_target : edges.first->m_source;
          auto const& otherSeg = component.m_FaceEdges[otherVtx].segment;

          Vec3 otherMidSeg((otherSeg.m_Ext1.x + otherSeg.m_Ext2.x) * 0.5,
            (otherSeg.m_Ext1.y + otherSeg.m_Ext2.y) * 0.5,
            0.0);

          iDrawer.DrawLine(curMidSeg, otherMidSeg, Vec4(1.0, 0.0, 0.0, 1.0));
        }
      }
    }
  }

  void DrawNeigh(NeighborhoodExtraction& iNeigh, Transforms& iTrans, DebugTool::Drawer& iDrawer)
  {
    for (auto& obj : iNeigh.GetObjects())
    {
      NeighborhoodExtraction::Neigh const& neighs = iNeigh.GetNeigh()[obj.second];

      Mat4 const& curTrans = iTrans.GetWorldTransform(obj.first);
      for (uint32_t const* neighIdx = neighs.neighbors; neighIdx < neighs.neighbors + neighs.numNeigh; ++neighIdx)
      {
        Mat4 const& otherTrans = iTrans.GetWorldTransform(iNeigh.GetHandles()[*neighIdx]);

        iDrawer.DrawLine(curTrans[3], otherTrans[3], Vec4(0.0, 0.0, 1.0, 1.0));
      }
    }
  }

  void DrawNavigator(NavigatorSystem& iNav, Transforms& iTrans, DebugTool::Drawer& iDrawer)
  {
    iNav.GetAgents().Iterate([&](NavigatorSystem::Agent& agent, NavigatorSystem::Agents::Handle)
    {
      NavigatorSystem::Obstacle agentObs = iNav.GetObstacles().Get(agent.m_Obstacle);
      Mat4 const& curTrans = iTrans.GetWorldTransform(agentObs.m_Object);
      Vec3 const& curPos = curTrans[3];

      Vec4 agentColor = [&]
      {
        if (agent.m_UnstuckEnabled) return Vec4(1.0, 1.0, 0.0, 1.0);
        if (agent.m_PriorityAvoidance) return Vec4(0.7, 0.0, 0.7, 1.0);
        if (agent.m_HasPriority) return Vec4(0.0, 1.0, 0.0, 1.0);
        return Vec4(1.0, 1.0, 1.0, 1.0);
      }();

      iDrawer.DrawLine(curPos, curPos + agentObs.m_Dir * 4, agentColor);

      Vec3 correctedVel = agent.m_CorrectedVelocity;
      float len = NormalizeAndGetLength(correctedVel);

      iDrawer.DrawLine(curPos, curPos + correctedVel * (len / agentObs.m_Speed) * 4, Vec4(1.0, 0.0, 0.0, 1.0));

    });
  }

}