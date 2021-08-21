#pragma once

#include <core/containers.hpp>
#include <engine/common/debugtool.hpp>


namespace eXl
{
  struct Room;
  class NavMesh;
  class NeighborhoodExtraction;
  class Transforms;
  class NavigatorSystem;

  void DrawRooms(Vector<Room> const& iRooms, DebugTool::Drawer& iDrawer);

  void DrawNavMesh(NavMesh const& iMesh, DebugTool::Drawer& iDrawer);

  void DrawNeigh(NeighborhoodExtraction& iNeigh, Transforms& iTrans, DebugTool::Drawer& iDrawer);

  void DrawNavigator(NavigatorSystem& iNav, Transforms& iTrans, DebugTool::Drawer& iDrawer);
}