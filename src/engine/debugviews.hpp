#pragma once

#include <core/base/containers.hpp>
#include <dunatk/common/debugtool.hpp>


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