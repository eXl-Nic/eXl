/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/map/dungeongraph.hpp>

namespace eXl
{
  class EXL_ENGINE_API DungeonGraph_Z : public DungeonGraph
  {
  public:

    static ContentName FinalRoom();
    static ContentName Entrance();
    static ContentName Room();
    static ContentName Key();
    static ContentName Treasure();

    DungeonGraph_Z() = default;
    DungeonGraph_Z(DungeonGraph_Z const&) = delete;
    DungeonGraph_Z& operator=(DungeonGraph_Z const&) = delete;

    struct EXL_ENGINE_API EdgeProperties_Z : public EdgeProperties
    {
      DECLARE_RTTI(EdgeProperties_Z, EdgeProperties);
      bool m_Locks;
      Graph::vertex_descriptor m_Key = Graph::null_vertex();
      // != null, means directed
      Graph::vertex_descriptor m_Direction = Graph::null_vertex();
    };

    typedef Vector<std::unique_ptr<NodeProperties>> Nodes;
    typedef Vector<std::unique_ptr<EdgeProperties_Z>> Edges;

    void MakeSingleRoom(unsigned int iRoomSize);

    void Init();

    void ApplyRoomRule(Random& iRand);

    Graph const& GetGraph() const { return m_Graph; }
    Nodes const& GetNodes() const { return m_Nodes; }
    Edges const& GetEdges() const { return m_Edges; }

    boost::property_map<Graph, boost::vertex_index_t>::type GetIndexMap() { return boost::get(boost::vertex_index, m_Graph); }
    boost::property_map<Graph, boost::vertex_index_t>::const_type GetIndexMap() const { return boost::get(boost::vertex_index, m_Graph); }

  protected:

    EdgeProperties_Z* GetProperties(GraphEdge iEdge) { return EdgeProperties_Z::DynamicCast(DungeonGraph::GetProperties(iEdge)); }
    NodeProperties* GetProperties(GraphVtx iVtx) { return DungeonGraph::GetProperties(iVtx); }
    

    std::pair<GraphVtx, NodeProperties*> AddNode();
    std::pair<GraphEdge, EdgeProperties_Z*> AddEdge(GraphVtx, GraphVtx);
    std::pair<GraphEdge, EdgeProperties_Z*> AddEdge(std::pair<GraphVtx, NodeProperties*> const& iNode1
      , std::pair<GraphVtx, NodeProperties*> const& iNode2)
    {
      return AddEdge(iNode1.first, iNode2.first);
    }

    void NodeAdded(Graph& iGraph, GraphVtx) override;
    void EdgeAdded(Graph& iGraph, GraphEdge) override;
    void NodeRemoved(Graph const& iGraph, GraphVtx) override;
    void EdgeRemoved(Graph const& iGraph, GraphEdge) override;
    void NodeIdxChanged(Graph& iGraph, GraphVtx iVtx, uint32_t iFrom, uint32_t iTo) override;
    void EdgeIdxChanged(Graph& iGraph, GraphEdge iEdge, uint32_t iFrom, uint32_t iTo) override;

    Nodes m_Nodes;
    Edges m_Edges;

    struct MatchCtx;
    struct RewriteCtx;

    ES_RuleSystem m_Rules;

    uint32_t m_CorridorRoomRule;
    uint32_t m_CycleRoomRule;
    uint32_t m_BranchRoomRule;
  };
}

