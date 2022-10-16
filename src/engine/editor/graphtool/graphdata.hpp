#pragma once

#include <gen/pregraph.hpp>
#include <engine/common/world.hpp>
#include <engine/common/gamedata.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luaeventhandler.hpp>


namespace eXl
{
  struct LevelNodeData : public ES_RuleSystem::NodeData
  {
    DECLARE_RTTI(LevelNodeData, ES_RuleSystem::NodeData);

    void CopyNode(ES_RuleSystem::GraphVtx iVtx) const override;

    ObjectHandle m_Object;
    mutable ES_RuleSystem::GraphVtx m_Vtx;
    Name m_Tag;
    String m_DebugString;
  };

  struct LevelEdgeData : public ES_RuleSystem::EdgeData
  {
    DECLARE_RTTI(LevelEdgeData, ES_RuleSystem::EdgeData);

    void CopyEdge(ES_RuleSystem::GraphEdge iVtx) const override;

    ObjectHandle m_Object;
    mutable ES_RuleSystem::GraphEdge m_Edge;
    Name m_Tag;
  };

  struct GraphWrapper
  {
    GraphWrapper(World& iWorld
      , ES_RuleSystem::Graph& iGraph
      , DenseGameDataStorage<LevelNodeData>& iNodeData
      , DenseGameDataStorage<LevelEdgeData>& iEdgeData)
      : m_World(iWorld)
      , m_Graph(iGraph)
      , m_NodeData(iNodeData)
      , m_EdgeData(iEdgeData)
    {}
    
    ObjectHandle GetNodeObject(ES_RuleSystem::GraphVtx iVtx) const;
    ObjectHandle GetEdgeObject(ES_RuleSystem::GraphEdge iEdge) const;

    Vector<ObjectHandle> GetEdges(ObjectHandle iNode) const;
    ObjectHandle GetTargetNode(ObjectHandle iSource, ObjectHandle iEdge) const;

    Name GetEdgeTag(ObjectHandle iEdge) const;
    Name GetNodeTag(ObjectHandle iNode) const;

    ObjectHandle AddNode(ES_RuleSystem::GraphVtx iVtx);
    void RemoveNode(ES_RuleSystem::GraphVtx iVtx);
    
    ObjectHandle AddEdge(ES_RuleSystem::GraphEdge iEdge);
    void RemoveEdge(ES_RuleSystem::GraphEdge iEdge);

    World& m_World;
    ES_RuleSystem::Graph& m_Graph;

    DenseGameDataStorage<LevelNodeData>& m_NodeData;
    DenseGameDataStorage<LevelEdgeData>& m_EdgeData;
  };

  struct MatchWrapper
  {
    MatchWrapper(GraphWrapper const& iGraph)
      : m_Graph(iGraph)
    {}

    GraphWrapper const& GetGraph() const { return m_Graph; }

    GraphWrapper const& m_Graph;
  };

  struct RewriteWrapper
  {
    RewriteWrapper(GraphWrapper const& iSrcGraph
      , GraphWrapper const& iDstGraph
      , Vector<ES_RuleSystem::GraphVtx> const& iMatch);
    GraphWrapper const& GetSrcGraph() const { return m_SrcGraph; }
    GraphWrapper const& GetDstGraph() const { return m_DstGraph; }
    Vector<ObjectHandle> const& GetMatch() const { return m_Match; }

    GraphWrapper const& m_SrcGraph;
    GraphWrapper const& m_DstGraph;
    Vector<ObjectHandle> m_Match;
  };

  class RewriteSystem;

  struct GraphFactoryWrapper 
  {
    GraphFactoryWrapper(GraphWrapper& iDstGraph, RewriteSystem const& iSystem) : m_DstGraph(iDstGraph), m_System(iSystem){}

    ObjectHandle CreateNode(Name iTag) const;
    void SetDebugString(ObjectHandle, const char* iStr) const;
    ObjectHandle CreateEdge(ObjectHandle iNode1, ObjectHandle iNode2, Name iTag) const;

    GraphWrapper& m_DstGraph;
    RewriteSystem const& m_System;
  };

  DECLARE_TYPE_EX(GraphWrapper, eXl__GraphWrapper, );
  DECLARE_TYPE_EX(GraphFactoryWrapper, eXl__GraphFactoryWrapper, );

  struct LevelMatchContext : public ES_RuleSystem::UserMatchContext
  {
    DECLARE_RTTI(LevelMatchContext, ES_RuleSystem::UserMatchContext);

    LevelMatchContext(GraphWrapper& iWrapper)
      : m_Wrapper(iWrapper)
    {}

    GraphWrapper& m_Wrapper;
  };

  struct LevelRewriteContext : public ES_RuleSystem::UserRewriteContext
  {
    DECLARE_RTTI(LevelRewriteContext, ES_RuleSystem::UserRewriteContext);

    LevelRewriteContext(GraphWrapper& iWrapper)
      : m_Wrapper(iWrapper)
    {}

    GraphWrapper& m_Wrapper;
  };

  DECLARE_TYPE_EX(MatchWrapper, eXl__MatchWrapper, );
  DECLARE_TYPE_EX(RewriteWrapper, eXl__RewriteWrapper, );

  struct Rule
  {
    EXL_REFLECT;

    Vector<Name> m_ContextNodes;
    Vector<Name> m_CreateNodes;
    Vector<Name> m_CutNodes;

    struct Edge
    {
      EXL_REFLECT;
      Name tag;
      uint32_t nodes[2];
    };

    struct NewEdge
    {
      EXL_REFLECT;
      Name tag;
      uint32_t nodes[2];
      uint32_t port[2];
    };

    Vector<Edge> m_ContextEdges;
    Vector<Edge> m_CutEdge;
    Vector<NewEdge> m_NewEdge;

    ResourceHandle<LuaEventHandler> m_RewriteScript;
  };

  struct TagDef
  {
    EXL_REFLECT;
    ResourceHandle<Archetype> m_Archetype;
    bool m_IsNodeTag;
  };

  class RewriteSystem : public Resource
  {
    DECLARE_RTTI(RewriteSystem, Resource);
  public:

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static RewriteSystem* Create(Path const& iDir, String const& iName);
#endif

    ~RewriteSystem();

    static ResourceLoaderName StaticLoaderName();
    uint32_t ComputeHash() override;

    UnorderedMap<String, Rule> m_Rules;
    UnorderedMap<Name, TagDef> m_Tags;

    static Name GetAnyTag();

  protected:
    friend TResourceLoader <RewriteSystem, ResourceLoader>;

    RewriteSystem(ResourceMetaData&);

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
    Err Serialize(Serializer iStreamer);
  };
}