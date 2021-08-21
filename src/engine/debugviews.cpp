
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
  //    iDrawer.DrawLine(Vector3f(prevPos.X(), prevPos.Y(), 0.0), Vector3f(point.pos.X(), point.pos.Y(), 0.0), Vector4f(1.0, 0.0, 1.0, 1.0));
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

      Vector3f boxMin(box.m_Data[0].X(), box.m_Data[0].Y(), 0.0);
      Vector3f boxMax(box.m_Data[1].X(), box.m_Data[1].Y(), 0.0);

      iDrawer.DrawLine(boxMin, Vector3f(boxMin.X(), boxMax.Y()), Vector4f(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMin, Vector3f(boxMax.X(), boxMin.Y()), Vector4f(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMax, Vector3f(boxMin.X(), boxMax.Y()), Vector4f(1.0, 0.0, 1.0, 1.0));
      iDrawer.DrawLine(boxMax, Vector3f(boxMax.X(), boxMin.Y()), Vector4f(1.0, 0.0, 1.0, 1.0));
    }
  }

  void DrawNavMesh(NavMesh const& iMesh, DebugTool::Drawer& iDrawer)
  {
    for (auto& component : iMesh.GetComponents())
    {
      for (auto& face : component.m_Faces)
      {
        auto const& box = face.m_Box;

        Vector3f boxMin = MathTools::To3DVec(box.m_Data[0]);
        Vector3f boxMax = MathTools::To3DVec(box.m_Data[1]);

        iDrawer.DrawLine(boxMin, Vector3f(boxMin.X(), boxMax.Y(), 0.0), Vector4f(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMin, Vector3f(boxMax.X(), boxMin.Y(), 0.0), Vector4f(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMax, Vector3f(boxMin.X(), boxMax.Y(), 0.0), Vector4f(1.0, 0.0, 1.0, 1.0));
        iDrawer.DrawLine(boxMax, Vector3f(boxMax.X(), boxMin.Y(), 0.0), Vector4f(1.0, 0.0, 1.0, 1.0));

        for (auto const& seg : face.m_Walls)
        {
          iDrawer.DrawLine(MathTools::To3DVec(seg.m_Ext1), MathTools::To3DVec(seg.m_Ext2), Vector4f(1.0, 1.0, 1.0, 1.0));
        }
      }

      for (auto vertices = boost::vertices(component.m_Graph); vertices.first != vertices.second; ++vertices.first)
      {
        auto curVtx = *vertices.first;
        auto const& curSeg = component.m_FaceEdges[curVtx].segment;
        Vector3f curMidSeg((curSeg.m_Ext1.X() + curSeg.m_Ext2.X()) * 0.5,
          (curSeg.m_Ext1.Y() + curSeg.m_Ext2.Y()) * 0.5,
          0.0);

        for (auto edges = boost::out_edges(curVtx, component.m_Graph); edges.first != edges.second; ++edges.first)
        {
          auto otherVtx = edges.first->m_source == curVtx ? edges.first->m_target : edges.first->m_source;
          auto const& otherSeg = component.m_FaceEdges[otherVtx].segment;

          Vector3f otherMidSeg((otherSeg.m_Ext1.X() + otherSeg.m_Ext2.X()) * 0.5,
            (otherSeg.m_Ext1.Y() + otherSeg.m_Ext2.Y()) * 0.5,
            0.0);

          iDrawer.DrawLine(curMidSeg, otherMidSeg, Vector4f(1.0, 0.0, 0.0, 1.0));
        }
      }
    }
  }

  void DrawNeigh(NeighborhoodExtraction& iNeigh, Transforms& iTrans, DebugTool::Drawer& iDrawer)
  {
    for (auto& obj : iNeigh.GetObjects())
    {
      NeighborhoodExtraction::Neigh const& neighs = iNeigh.GetNeigh()[obj.second];

      Matrix4f const& curTrans = iTrans.GetWorldTransform(obj.first);
      Vector3f const& curPos = *reinterpret_cast<Vector3f const*>(curTrans.m_Data + 12);
      for (uint32_t const* neighIdx = neighs.neighbors; neighIdx < neighs.neighbors + neighs.numNeigh; ++neighIdx)
      {
        Matrix4f const& otherTrans = iTrans.GetWorldTransform(iNeigh.GetHandles()[*neighIdx]);
        Vector3f const& otherPos = *reinterpret_cast<Vector3f const*>(otherTrans.m_Data + 12);

        iDrawer.DrawLine(curPos, otherPos, Vector4f(0.0, 0.0, 1.0, 1.0));
      }
    }
  }

  void DrawNavigator(NavigatorSystem& iNav, Transforms& iTrans, DebugTool::Drawer& iDrawer)
  {
    iNav.GetAgents().Iterate([&](NavigatorSystem::Agent& agent, NavigatorSystem::Agents::Handle)
    {
      NavigatorSystem::Obstacle agentObs = iNav.GetObstacles().Get(agent.m_Obstacle);
      Matrix4f const& curTrans = iTrans.GetWorldTransform(agentObs.m_Object);
      Vector3f const& curPos = *reinterpret_cast<Vector3f const*>(curTrans.m_Data + 12);

      Vector4f agentColor = [&]
      {
        if (agent.m_UnstuckEnabled) return Vector4f(1.0, 1.0, 0.0, 1.0);
        if (agent.m_PriorityAvoidance) return Vector4f(0.7, 0.0, 0.7, 1.0);
        if (agent.m_HasPriority) return Vector4f(0.0, 1.0, 0.0, 1.0);
        return Vector4f(1.0, 1.0, 1.0, 1.0);
      }();

      iDrawer.DrawLine(curPos, curPos + agentObs.m_Dir * 4, agentColor);

      Vector3f correctedVel = agent.m_CorrectedVelocity;
      float len = correctedVel.Normalize();

      iDrawer.DrawLine(curPos, curPos + correctedVel * (len / agentObs.m_Speed) * 4, Vector4f(1.0, 0.0, 0.0, 1.0));

    });
  }

}