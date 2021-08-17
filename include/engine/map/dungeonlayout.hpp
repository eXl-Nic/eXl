#pragma once

#include <engine/enginelib.hpp>
#include "dungeongraph.hpp"
#include <math/aabb2d.hpp>

namespace eXl
{
   struct Room
   {
     AABB2Di m_Box;
     DungeonGraph::Graph::vertex_descriptor m_Node;
   };

   typedef Vector<Room> Layout;
   typedef Vector<Layout> LayoutCollection;

   EXL_ENGINE_API LayoutCollection LayoutGraph(DungeonGraph const& iGraph, Random& iRand);
}

