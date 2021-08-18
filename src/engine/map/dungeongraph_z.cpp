/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/dungeongraph_z.hpp>

#include <math/math.hpp>

namespace eXl
{
  DungeonGraph::ContentName DungeonGraph_Z::Entrance() { return ContentName("Entrance"); }
  DungeonGraph::ContentName DungeonGraph_Z::FinalRoom() { return ContentName("FinalRoom"); }
  DungeonGraph::ContentName DungeonGraph_Z::Key() { return ContentName("Key"); }
  DungeonGraph::ContentName DungeonGraph_Z::Room() { return ContentName("Room"); }
  DungeonGraph::ContentName DungeonGraph_Z::Treasure() { return ContentName("Treasure"); }

  IMPLEMENT_RTTI(DungeonGraph_Z::EdgeProperties_Z);

  struct DungeonGraph_Z::MatchCtx : ES_RuleSystem::UserMatchContext
  {
    DECLARE_RTTI(MatchCtx, ES_RuleSystem::UserMatchContext);

    MatchCtx(DungeonGraph_Z& iGraph)
      : m_Graph(iGraph)
    {}
    DungeonGraph_Z& m_Graph;
  };

  struct DungeonGraph_Z::RewriteCtx : ES_RuleSystem::UserRewriteContext
  {
    DECLARE_RTTI(RewriteCtx, ES_RuleSystem::UserRewriteContext);

    RewriteCtx(DungeonGraph_Z& iGraph)
      : m_Graph(iGraph)
    {}
    DungeonGraph_Z& m_Graph;
  };

  IMPLEMENT_RTTI(DungeonGraph_Z::MatchCtx);
  IMPLEMENT_RTTI(DungeonGraph_Z::RewriteCtx);

  void DungeonGraph_Z::NodeAdded(Graph& iGraph, GraphVtx iVtx)
  {
    DungeonGraph::NodeAdded(iGraph, iVtx);
    uint32_t nodeId = GetIndex(iVtx);
    m_Nodes.resize(Mathi::Max(nodeId + 1, m_MaxVtxId));
    m_Nodes[nodeId] = std::make_unique<NodeProperties>();
    boost::put(boost::vertex_name, iGraph, iVtx, m_Nodes[nodeId].get());
  }

  void DungeonGraph_Z::EdgeAdded(Graph& iGraph, GraphEdge iEdge)
  {
    DungeonGraph::EdgeAdded(iGraph, iEdge);
    uint32_t edgeId = GetIndex(iEdge);
    m_Edges.resize(Mathi::Max(edgeId + 1, m_MaxEdgeId));
    m_Edges[edgeId] = std::make_unique<EdgeProperties_Z>();
    boost::put(boost::edge_name, iGraph, iEdge, m_Edges[edgeId].get());
  }

  void DungeonGraph_Z::NodeRemoved(Graph const& iGraph, GraphVtx iVtx)
  {
    m_Nodes[GetIndex(iVtx)] = nullptr;
    DungeonGraph::NodeRemoved(iGraph, iVtx);
  }

  void DungeonGraph_Z::EdgeRemoved(Graph const& iGraph, GraphEdge iEdge)
  {
    m_Edges[GetIndex(iEdge)] = nullptr;
    DungeonGraph::EdgeRemoved(iGraph, iEdge);
  }

  void DungeonGraph_Z::NodeIdxChanged(Graph& iGraph, GraphVtx iVtx, uint32_t iFrom, uint32_t iTo)
  {
    std::swap(m_Nodes[iFrom], m_Nodes[iTo]);
  }

  void DungeonGraph_Z::EdgeIdxChanged(Graph& iGraph, GraphEdge iEdge, uint32_t iFrom, uint32_t iTo)
  {
    std::swap(m_Edges[iFrom], m_Edges[iTo]);
  }

  std::pair<DungeonGraph::GraphVtx, DungeonGraph::NodeProperties*> DungeonGraph_Z::AddNode()
  {
    auto newVtx = boost::add_vertex(m_Graph);
    NodeAdded(m_Graph, newVtx);

    return std::make_pair(newVtx, DungeonGraph::GetProperties(newVtx));
  }

  std::pair<DungeonGraph::GraphEdge, DungeonGraph_Z::EdgeProperties_Z*> DungeonGraph_Z::AddEdge(GraphVtx iNode1, GraphVtx iNode2)
  {
    auto newEdge = boost::add_edge(iNode1, iNode2, m_Graph).first;

    EdgeAdded(m_Graph, newEdge);
    
    return std::make_pair(newEdge, GetProperties(newEdge));
  }

  void DungeonGraph_Z::MakeSingleRoom(unsigned int iRoomSize)
  {
    auto throneRoom = AddNode();
    throneRoom.second->AddContent(Room(), iRoomSize);
  }

  void DungeonGraph_Z::Init()
  {
    auto entrance = AddNode();
    entrance.second->AddContent(Entrance(), 4);
    entrance.second->SetPhysical(true);

    auto finalRoom = AddNode();
    finalRoom.second->AddContent(FinalRoom(), 4);
    finalRoom.second->SetPhysical(true);

    auto link = AddEdge(entrance, finalRoom);
    link.second->m_PhysicalConnection = true;

    auto nodeRemoved = [](ES_RuleSystem::RewriteCtx& iCtx, GraphVtx iVtx)
    {
      DungeonGraph_Z& self = RewriteCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      self.NodeRemoved(iCtx.graph, iVtx);
    };

    auto edgeRemoved = [](ES_RuleSystem::RewriteCtx& iCtx, GraphEdge iEdge)
    {
      DungeonGraph_Z& self = RewriteCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      self.EdgeRemoved(iCtx.graph, iEdge);
    };

    auto createNewRoom = [](ES_RuleSystem::RewriteCtx& iCtx, GraphVtx iVtx)
    {
      DungeonGraph_Z& self = RewriteCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      self.NodeAdded(iCtx.finalGraph, iVtx);
      auto& roomDesc = self.m_Nodes[boost::get(boost::vertex_index, iCtx.finalGraph, iVtx)];
      roomDesc->AddContent(Room(), 4);
      roomDesc->SetPhysical(true);
    };

    auto createNewDoorway = [](ES_RuleSystem::RewriteCtx& iCtx, GraphEdge iEdge)
    {
      DungeonGraph_Z& self = RewriteCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      self.EdgeAdded(iCtx.finalGraph, iEdge);
      auto& edgeDesc = self.m_Edges[boost::get(boost::edge_index, iCtx.finalGraph, iEdge)];
      edgeDesc->m_PhysicalConnection = true;
    };

    auto isRoom = [](ES_RuleSystem::MatchCtx& iCtx, GraphVtx iVtx)
    {
      DungeonGraph_Z& self = MatchCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      return self.GetProperties(iVtx)->IsPhysical();
    };

    auto isNotFinalNode = [](ES_RuleSystem::MatchCtx& iCtx, GraphVtx iVtx)
    {
      DungeonGraph_Z& self = MatchCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      auto props = self.GetProperties(iVtx);
      return props->IsPhysical() && !props->HasContent(FinalRoom());
    };

    auto isNotTooMuchConnectionEntrance = [](ES_RuleSystem::MatchCtx& iCtx, GraphVtx iVtx)
    {
      DungeonGraph_Z& self = MatchCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      auto props = self.GetProperties(iVtx);
      return props->IsPhysical() && (!props->HasContent(Entrance()) || boost::degree(iVtx, iCtx.graph) < 2);
    };

    auto canBranch = [isNotFinalNode, isNotTooMuchConnectionEntrance](ES_RuleSystem::MatchCtx& iCtx, GraphVtx iVtx)
    {
      return isNotFinalNode(iCtx, iVtx) && isNotTooMuchConnectionEntrance(iCtx, iVtx);
    };

    auto isDoorway = [](ES_RuleSystem::MatchCtx& iCtx, GraphEdge iEdge)
    {
      DungeonGraph_Z& self = MatchCtx::DynamicCast(iCtx.userCtx)->m_Graph;
      return self.GetProperties(iEdge)->m_PhysicalConnection;
    };

    m_CorridorRoomRule = m_Rules.StartRule().AddNode(0, isRoom).AddNode(0, isRoom)
      .AddCutConnection(0, 1, 0, isDoorway, edgeRemoved)
      .AddNewNode(0, createNewRoom)
      .AddNewConnection(0, 2, 0, -1, 0, createNewDoorway)
      .AddNewConnection(1, 2, 0, -1, 0, createNewDoorway)
      .End();

    m_CycleRoomRule = m_Rules.StartRule().AddNode(1, canBranch).AddNode(1, canBranch)
      .AddCutConnection(0, 1, 0, isDoorway, edgeRemoved)
      .AddNewNode(0, createNewRoom).AddNewNode(0, createNewRoom)
      .AddNewConnection(0, 2, -1, -1, 0, createNewDoorway)
      .AddNewConnection(1, 2, -1, -1, 0, createNewDoorway)
      .AddNewConnection(0, 3, -1, -1, 0, createNewDoorway)
      .AddNewConnection(1, 3, -1, -1, 0, createNewDoorway)
      .End();

    m_BranchRoomRule = m_Rules.StartRule().AddNode(2, isNotFinalNode).AddNode(0)
      .AddConnection(0, 1, 0, isDoorway)
      .AddNewNode(0, createNewRoom)
      .AddNewConnection(0, 2, -1, -1, 0, createNewDoorway)
      .End();
  }

  void DungeonGraph_Z::ApplyRoomRule(Random& iRand)
  {
    uint32_t ruleToApply = iRand.Generate() % 3;
    switch (ruleToApply)
    {
    case 0:
      ruleToApply = m_CorridorRoomRule;
      break;
    case 1:
      ruleToApply = m_CycleRoomRule;
      break;
    default:
      ruleToApply = m_BranchRoomRule;
      break;
    }

    MatchCtx mCtx(*this);
    auto matches = m_Rules.FindRuleMatch(ruleToApply, m_Graph, &mCtx);

    if (matches.empty())
    {
      return;
    }

    uint32_t matchToReplace = iRand.Generate() % matches.size();

    RewriteCtx rCtx(*this);
    Graph newGraph;
    m_Rules.ApplyRule(m_Graph, newGraph, ruleToApply, matches[matchToReplace], &rCtx);

    m_Graph = newGraph;
    DefragIds(m_Graph);
  }

}