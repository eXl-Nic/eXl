#pragma once

#include <engine/map/dungeongraph.hpp>

namespace eXl
{
  struct ResourceDungeonDesc
  {
    uint32_t m_MaxResourceSize = 2;
    uint32_t m_MaxNodeSize = 4;
    uint32_t m_MaxRoomSize = 8;
    uint32_t m_MaxRelaySize = 6;
    uint32_t m_MaxRelayDegree = 6;
    float    m_RandomRelaySplitChance = 0.5;

    uint32_t m_Resources = 30;
  };

  class EXL_ENGINE_API DungeonGraph_Res : public DungeonGraph
  {
  public:

    static ContentName Resource();
    static ContentName ResourceRelay();
    static ContentName ThroneRoom();
    static ContentName TreasureRoom();
    static ContentName Library();
    static ContentName RestRoom();

    DungeonGraph_Res() = default;
    DungeonGraph_Res(DungeonGraph_Res const&) = delete;
    DungeonGraph_Res& operator=(DungeonGraph_Res const&) = delete;

    struct EXL_ENGINE_API EdgeProperties_Res : public EdgeProperties
    {
      DECLARE_RTTI(EdgeProperties_Res, EdgeProperties);

      EdgeProperties_Res()
      {
        m_PhysicalConnection = true;
      }

      uint32_t m_ResourceFlow;
      Graph::vertex_descriptor m_ResourceSrc = Graph::null_vertex();
    };

    typedef Vector<std::unique_ptr<NodeProperties>> Nodes;
    typedef Vector<std::unique_ptr<EdgeProperties_Res>> Edges;

    void MakeSingleRoom(unsigned int iRoomSize);

    void InitRes();

    void ExpandResources(Random& iRand);

    void ExpandRooms(Random& iRand);

    Graph const& GetGraph() const { return m_Graph; }
    Nodes const& GetNodes() const { return m_Nodes; }
    Edges const& GetEdges() const { return m_Edges; }

    boost::property_map<Graph, boost::vertex_index_t>::type GetIndexMap() { return boost::get(boost::vertex_index, m_Graph); }
    boost::property_map<Graph, boost::vertex_index_t>::const_type GetIndexMap() const { return boost::get(boost::vertex_index, m_Graph); }

  private:

    std::pair<GraphVtx, NodeProperties*> AddNode();
    std::pair<GraphEdge, EdgeProperties_Res*> AddEdge(GraphVtx, GraphVtx);
    std::pair<GraphEdge, EdgeProperties_Res*> AddEdge(std::pair<GraphVtx, NodeProperties*> const& iNode1
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
    
    ResourceDungeonDesc m_Desc;
  };
}

