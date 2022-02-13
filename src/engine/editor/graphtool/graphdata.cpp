#include "graphdata.hpp"
#include <gen/graphutils.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <core/type/tagtype.hpp>

namespace eXl
{
    IMPLEMENT_RTTI(LevelNodeData);
    IMPLEMENT_RTTI(LevelEdgeData);
    IMPLEMENT_RTTI(LevelMatchContext);
    IMPLEMENT_RTTI(LevelRewriteContext);
    IMPLEMENT_TAG_TYPE_EX(GraphWrapper, Graph);
    
    ObjectHandle GraphWrapper::GetNodeObject(ES_RuleSystem::GraphVtx iVtx) const
    {
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_Graph, iVtx));
      eXl_ASSERT_REPAIR_RET(nodeData != nullptr, ObjectHandle());

      return nodeData->m_Object;
    }

    ObjectHandle GraphWrapper::GetEdgeObject(ES_RuleSystem::GraphEdge iEdge) const
    {
      LevelEdgeData const* edgeData = LevelEdgeData::DynamicCast(boost::get(boost::edge_name, m_Graph, iEdge));
      eXl_ASSERT_REPAIR_RET(edgeData != nullptr, ObjectHandle());

      return edgeData->m_Object;
    }

    Vector<ObjectHandle> GraphWrapper::GetEdges(ObjectHandle iNode) const
    {
      Vector<ObjectHandle> edges;
      LevelNodeData const* data = m_NodeData.Get(iNode);
      eXl_ASSERT_REPAIR_RET(data != nullptr, edges);

      for (auto edge : OutEdgesIter(m_Graph, data->m_Vtx))
      {
        LevelEdgeData const* edgeData = LevelEdgeData::DynamicCast(boost::get(boost::edge_name, m_Graph, edge));
        eXl_ASSERT_REPAIR_BEGIN(edgeData != nullptr)
        {
          continue;
        }

        edges.push_back(edgeData->m_Object);
      }

      return edges;
    }

    ObjectHandle GraphWrapper::GetTargetNode(ObjectHandle iSource, ObjectHandle iEdge) const
    {
      LevelNodeData const* nodeData = m_NodeData.Get(iSource);
      eXl_ASSERT_REPAIR_RET(nodeData != nullptr, ObjectHandle());

      LevelEdgeData const* edgeData = m_EdgeData.Get(iEdge);
      eXl_ASSERT_REPAIR_RET(edgeData != nullptr, ObjectHandle());

      ES_RuleSystem::GraphVtx target = GetTarget(nodeData->m_Vtx, edgeData->m_Edge);
      eXl_ASSERT_REPAIR_RET(target != m_Graph.null_vertex(), ObjectHandle());

      LevelNodeData const* targetNodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_Graph, target));
      eXl_ASSERT_REPAIR_RET(targetNodeData != nullptr, ObjectHandle());

      return targetNodeData->m_Object;
    }

    ObjectHandle GraphWrapper::AddNode(ES_RuleSystem::GraphVtx iVtx)
    {
      ObjectHandle nodeObject = m_World.CreateObject();
      LevelNodeData& nodeData = m_NodeData.GetOrCreate(nodeObject);
      nodeData.m_Object = nodeObject;
      nodeData.m_Vtx = iVtx;
      boost::put(boost::vertex_name, m_Graph, iVtx, &nodeData);

      return nodeObject;
    }

    void GraphWrapper::RemoveNode(ES_RuleSystem::GraphVtx iVtx)
    {
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_Graph, iVtx));
      eXl_ASSERT_REPAIR_RET(nodeData != nullptr, void());

      boost::put(boost::vertex_name, m_Graph, iVtx, nullptr);
      ObjectHandle nodeObject = nodeData->m_Object;
      m_NodeData.Erase(nodeObject);
      m_World.DeleteObject(nodeObject);
    }

    ObjectHandle GraphWrapper::AddEdge(ES_RuleSystem::GraphEdge iEdge)
    {
      ObjectHandle edgeObject = m_World.CreateObject();
      LevelEdgeData& edgeData = m_EdgeData.GetOrCreate(edgeObject);
      edgeData.m_Object = edgeObject;
      edgeData.m_Edge = iEdge;
      boost::put(boost::edge_name, m_Graph, iEdge, &edgeData);
      
      return edgeObject;
    }

    void GraphWrapper::RemoveEdge(ES_RuleSystem::GraphEdge iEdge)
    {
      LevelEdgeData const* edgeData = LevelEdgeData::DynamicCast(boost::get(boost::edge_name, m_Graph, iEdge));
      eXl_ASSERT_REPAIR_RET(edgeData != nullptr, void());

      boost::put(boost::edge_name, m_Graph, iEdge, nullptr);
      ObjectHandle edgeObject = edgeData->m_Object;
      m_EdgeData.Erase(edgeObject);
      m_World.DeleteObject(edgeObject);
    }

    IMPLEMENT_RTTI(RewriteSystem);

    using RewriteSystemLoader = TResourceLoader <RewriteSystem, ResourceLoader>;

    void RewriteSystem::Init()
    {
      BehaviourDesc desc;
      desc.behaviourName = "RewriteRule";
      desc.functions.insert(std::make_pair("CheckNode", FunDesc::Create<bool(GraphWrapper&, uint32_t, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("CheckEdge", FunDesc::Create<bool(GraphWrapper&, uint32_t, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("CheckMatch", FunDesc::Create<bool(GraphWrapper&, Vector<ObjectHandle>)>()));
      desc.functions.insert(std::make_pair("CreateNode", FunDesc::Create<void(GraphWrapper&, uint32_t, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("CreateEdge", FunDesc::Create<void(GraphWrapper&, uint32_t, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("RemoveNode", FunDesc::Create<void(GraphWrapper&, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("RemoveEdge", FunDesc::Create<void(GraphWrapper&, ObjectHandle)>()));

      LuaScriptSystem::AddBehaviourDesc(desc);
      ResourceManager::AddLoader(&RewriteSystemLoader::Get(), RewriteSystem::StaticRtti());
    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    RewriteSystem* RewriteSystem::Create(Path const& iDir, String const& iName)
    {
      return RewriteSystemLoader::Get().Create(iDir, iName);
    }
#endif

    RewriteSystem::RewriteSystem(ResourceMetaData& iMeta)
      : Resource(iMeta)
    {}

    RewriteSystem::~RewriteSystem() = default;

    Name RewriteSystem::GetAnyTag()
    {
      static Name s_Tag("<Any>");
      return s_Tag;
    }

    uint32_t RewriteSystem::ComputeHash()
    {
      return 0;
    }

    ResourceLoaderName RewriteSystem::StaticLoaderName()
    {
      return ResourceLoaderName("RewriteSystem");
    }

    Err RewriteSystem::Stream_Data(Streamer& iStreamer) const
    {
      return const_cast<RewriteSystem*>(this)->Serialize(Serializer(iStreamer));
    }

    Err RewriteSystem::Unstream_Data(Unstreamer& iStreamer)
    {
      return Serialize(Serializer(iStreamer));
    }

    Err RewriteSystem::Serialize(Serializer iStreamer)
    {
      iStreamer.BeginStruct();

      iStreamer.PushKey("Tags");
      iStreamer.HandleMapSorted(m_Tags);
      iStreamer.PopKey();

      iStreamer.PushKey("Rules");
      iStreamer.HandleMapSorted(m_Rules);
      iStreamer.PopKey();

      iStreamer.EndStruct();

      return Err::Success;
    }
}