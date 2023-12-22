#pragma once


#include <gen/pregraph.hpp>
#include <engine/common/world.hpp>
#include <engine/common/gamedata.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luaeventhandler.hpp>
#include <core/lua/luabind/object.hpp>
#include <engine/game/commondef.hpp>
#include <gen/graphrules.hpp>

namespace eXl
{
  DECLARE_ENGINE_TYPE(RoomLayoutInfo);

  struct EXL_ENGINE_API LevelNodeData : public ES_RuleSystem::NodeData
  {
    DECLARE_RTTI(LevelNodeData, ES_RuleSystem::NodeData);

    void CopyNode(ES_RuleSystem::GraphVtx iVtx) const override;

    ObjectHandle m_Object;
    mutable ES_RuleSystem::GraphVtx m_Vtx;
    Name m_Tag;
    String m_DebugString;
  };

  struct EXL_ENGINE_API LevelEdgeData : public ES_RuleSystem::EdgeData
  {
    DECLARE_RTTI(LevelEdgeData, ES_RuleSystem::EdgeData);

    void CopyEdge(ES_RuleSystem::GraphEdge iVtx) const override;

    ObjectHandle m_Object;
    mutable ES_RuleSystem::GraphEdge m_Edge;
    Name m_Tag;
  };

  struct EXL_ENGINE_API GraphWrapper
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
    Vector<ObjectHandle> FindPath(ObjectHandle iStart, ObjectHandle iGoal, luabind::object iNodeFilter, luabind::object iEdgeFilter) const;

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

  struct EXL_ENGINE_API MatchWrapper
  {
    MatchWrapper(GraphWrapper const& iGraph)
      : m_Graph(iGraph)
    {}

    GraphWrapper const& GetGraph() const { return m_Graph; }

    GraphWrapper const& m_Graph;
  };

  struct EXL_ENGINE_API RewriteWrapper
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

  struct EXL_ENGINE_API GraphFactoryWrapper
  {
    GraphFactoryWrapper(GraphWrapper& iDstGraph, RewriteSystem const& iSystem) : m_DstGraph(iDstGraph), m_System(iSystem){}

    void SetDebugString(ObjectHandle, const char* iStr) const;

    GraphWrapper& m_DstGraph;
    RewriteSystem const& m_System;
  };

  DECLARE_ENGINE_TYPE(GraphWrapper);
  DECLARE_ENGINE_TYPE(GraphFactoryWrapper);

  struct EXL_ENGINE_API LevelMatchContext : public ES_RuleSystem::UserMatchContext
  {
    DECLARE_RTTI(LevelMatchContext, ES_RuleSystem::UserMatchContext);

    LevelMatchContext(GraphWrapper& iWrapper)
      : m_Wrapper(iWrapper)
    {}

    GraphWrapper& m_Wrapper;
  };

  struct EXL_ENGINE_API LevelRewriteContext : public ES_RuleSystem::UserRewriteContext
  {
    DECLARE_RTTI(LevelRewriteContext, ES_RuleSystem::UserRewriteContext);

    LevelRewriteContext(GraphWrapper& iWrapper)
      : m_Wrapper(iWrapper)
    {}

    GraphWrapper& m_Wrapper;
  };

  DECLARE_ENGINE_TYPE(MatchWrapper);
  DECLARE_ENGINE_TYPE(RewriteWrapper);

  class EXL_ENGINE_API RewriteSystemRsc : public Resource
  {
    DECLARE_RTTI(RewriteSystemRsc, Resource);
  public:

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static RewriteSystemRsc* Create(Path const& iDir, String const& iName);
#endif

    ~RewriteSystemRsc();

    static ResourceLoaderName StaticLoaderName();
    uint32_t ComputeHash() override;

    RewriteSystem m_Sys;

    struct RuleAdditionalData {
      ResourceHandle<LuaEventHandler> m_Script;
      SERIALIZE_METHODS;
    };

    struct TagAdditionalData {
      ResourceHandle<Archetype> m_Archetype;
      SERIALIZE_METHODS;
    };

    UnorderedMap<String, RuleAdditionalData> m_Rules;
    UnorderedMap<Name, TagAdditionalData> m_Tags;

  protected:
    friend TResourceLoader <RewriteSystemRsc, ResourceLoader>;

    RewriteSystemRsc(ResourceMetaData&);

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
    Err Serialize(Serializer iStreamer);
  };
}

