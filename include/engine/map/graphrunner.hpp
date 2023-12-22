#pragma once

/*
Copyright 2009-2023 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/world.hpp>
#include <gen/graphrules.hpp>
#include <gen/pregraph.hpp>
#include <engine/map/graphdata.hpp>
#include <engine/common/data_tables/dense.hpp>

namespace eXl 
{
  class Random;
  class RewriteSystemRsc;
  struct EXL_ENGINE_API GraphRunner
  {
    using NodeData = DenseGameDataStorage<LevelNodeData>;
    using EdgeData = DenseGameDataStorage<LevelEdgeData>;

    GraphRunner(RewriteSystemRsc const & iSys, World & iWorld, NodeData& iNodeData, EdgeData& iEdgeData);

    World& m_World;
    const RewriteSystemRsc& m_SysRsc;
    const RewriteSystem& m_Sys;

    struct RuleItem {
      String ruleName;
      int application;
    };

    NodeData& m_NodeData;
    EdgeData& m_EdgeData;

    void RunRules(Random& iRand, Vector<RuleItem> const& iRules, ES_RuleSystem::Graph& oGraph);
    static void LayoutGraph(World& iWorld, Random& iRand, ES_RuleSystem::Graph const& iGraph, Vector<AABB2DPolygoni>& oRooms, Vector<AABB2DPolygoni>& oWalls, LuaEventHandler const* iLayoutScript);
  };
}