#pragma once

#include <engine/enginelib.hpp>
#include <core/coredef.hpp>
#include <core/random.hpp>
#include <core/name.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <gen/pregraph.hpp>

namespace eXl
{
  class EXL_ENGINE_API DungeonGraph
  {
  public:

    //struct ContentNameTag{};
    //using ContentName = Name;
    MAKE_NAME_DECL(ContentName);

    //static const ContentName s_Default;

    struct NodeContent
    {
      ContentName m_Type;
      uint32_t m_Size;
    };

    struct EXL_ENGINE_API NodeProperties : public ES_RuleSystem::NodeData
    {
      DECLARE_RTTI(NodeProperties, ES_RuleSystem::NodeData);

      void AddContent(ContentName iType, uint32_t iSize);

      bool HasContent(ContentName iType) const;

      NodeContent* GetContent(ContentName iType);

      NodeContent const* GetContent(ContentName iType) const;

      void SetContentSize(ContentName iType, uint32_t iSize);

      void RemoveContent(ContentName iType);

      uint32_t const& GetSize() const { return m_Size; }

      List<NodeContent>::const_iterator begin() const { return m_Contents.begin(); }

      List<NodeContent>::const_iterator end() const { return m_Contents.end(); }

      void SetPhysical(bool iPhysical) { m_PhysicalNode = true; }
      bool IsPhysical() const { return m_PhysicalNode; }

    protected:

      List<NodeContent> m_Contents;
      uint32_t m_Size;
      uint32_t m_ContentFlags;
      bool m_PhysicalNode;
    };

    using Graph = ES_RuleSystem::Graph;
    using GraphVtx = ES_RuleSystem::GraphVtx;
    using GraphEdge = ES_RuleSystem::GraphEdge;

    struct EXL_ENGINE_API EdgeProperties : public ES_RuleSystem::EdgeData
    {
      DECLARE_RTTI(EdgeProperties, ES_RuleSystem::EdgeData);

      bool m_PhysicalConnection;
    };

    //void MakeSingleRoom(unsigned int iRoomSize);

    void PrintGraph();

    Graph const& GetGraph() const { return m_Graph; }

    //boost::property_map<Graph, boost::vertex_index_t>::type GetIndexMap() { return boost::get(boost::vertex_index, m_Graph); }
    boost::property_map<Graph, boost::vertex_index_t>::const_type GetIndexMap() const { return boost::get(boost::vertex_index, m_Graph); }
    boost::property_map<Graph, boost::edge_index_t>::const_type GetEdgeIndexMap() const { return boost::get(boost::edge_index, m_Graph); }

    NodeProperties const* GetProperties(GraphVtx iVtx) const { return NodeProperties::DynamicCast(boost::get(boost::vertex_name, m_Graph, iVtx)); }
    EdgeProperties const* GetProperties(GraphEdge iEdge) const { return EdgeProperties::DynamicCast(boost::get(boost::edge_name, m_Graph, iEdge)); }

    uint32_t GetIndex(GraphVtx iVtx) const { return GetIndexMap()[iVtx]; }
    uint32_t GetIndex(GraphEdge iEdge) const { return GetEdgeIndexMap()[iEdge]; }

  protected:

    NodeProperties* GetProperties(GraphVtx iVtx) { return const_cast<NodeProperties*>(const_cast<DungeonGraph const*>(this)->GetProperties(iVtx)); }
    EdgeProperties* GetProperties(GraphEdge iEdge) { return const_cast<EdgeProperties*>(const_cast<DungeonGraph const*>(this)->GetProperties(iEdge)); }

    virtual void NodeAdded(Graph& iGraph, GraphVtx);
    virtual void EdgeAdded(Graph& iGraph, GraphEdge);
    virtual void NodeRemoved(Graph const& iGraph, GraphVtx);
    virtual void EdgeRemoved(Graph const& iGraph, GraphEdge);

    virtual void NodeIdxChanged(Graph& iGraph, GraphVtx iVtx, uint32_t iFrom, uint32_t iTo){}
    virtual void EdgeIdxChanged(Graph& iGraph, GraphEdge iEdge, uint32_t iFrom, uint32_t iTo){}

    void DefragIds(Graph& iGraph);

    Graph m_Graph;

    Set<uint32_t> m_AvailableVtxIdx;
    Set<uint32_t> m_AvailableEdgeIdx;
    uint32_t m_MaxVtxId = 0;
    uint32_t m_MaxEdgeId = 0;
  };

  MAKE_NAME_TYPE(DungeonGraph::ContentName);
}

