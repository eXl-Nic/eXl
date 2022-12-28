#include "graphdata.hpp"
#include <gen/graphutils.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <core/type/tagtype.hpp>
#include <engine/game/commondef.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/function_property_map.hpp>

namespace eXl
{
    IMPLEMENT_RTTI(LevelNodeData);
    IMPLEMENT_RTTI(LevelEdgeData);
    IMPLEMENT_RTTI(LevelMatchContext);
    IMPLEMENT_RTTI(LevelRewriteContext);
    IMPLEMENT_TAG_TYPE(GraphWrapper);
    IMPLEMENT_TAG_TYPE(MatchWrapper);
    IMPLEMENT_TAG_TYPE(RewriteWrapper);
    IMPLEMENT_TAG_TYPE(GraphFactoryWrapper);

    PropertySheetName RoomLayoutInfo::PropertyName() {
      static PropertySheetName s_Name("RoomLayoutInfo");
      return s_Name;
    }
    
    void LevelNodeData::CopyNode(ES_RuleSystem::GraphVtx iVtx) const
    {
      m_Vtx = iVtx;
    }

    void LevelEdgeData::CopyEdge(ES_RuleSystem::GraphEdge iEdge) const
    {
      m_Edge = iEdge;
    }

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

    struct GraphFilter
    {
      GraphFilter()
      {}

      bool operator()(const ES_RuleSystem::GraphVtx& e) const
      {
        return m_ValidVtx ? m_ValidVtx->count(e) > 0 : true;
      }

      bool operator()(const ES_RuleSystem::GraphEdge& e) const
      {
        return m_ValidEdge ? m_ValidEdge->count(e) > 0 : true;
      }
      
      UnorderedSet<ES_RuleSystem::GraphVtx> const* m_ValidVtx = nullptr;
      UnorderedSet<ES_RuleSystem::GraphEdge> const* m_ValidEdge = nullptr;
    };

    Vector<ObjectHandle> GraphWrapper::FindPath(ObjectHandle iStart, ObjectHandle iGoal, luabind::object iNodeFilter, luabind::object iEdgeFilter) const
    {
      auto dummyWhFunc = [](ES_RuleSystem::GraphEdge) { return 1.0; };

      LevelNodeData const* startData = m_NodeData.Get(iStart);
      eXl_ASSERT_REPAIR_RET(startData != nullptr, Vector<ObjectHandle>());

      LevelNodeData const* goalData = m_NodeData.Get(iGoal);
      eXl_ASSERT_REPAIR_RET(goalData != nullptr, Vector<ObjectHandle>());

      Vector<ES_RuleSystem::GraphVtx> p(boost::num_vertices(m_Graph), startData->m_Vtx);
      Vector<float> d(boost::num_vertices(m_Graph));
      Vector<ObjectHandle> path;
      ES_RuleSystem::GraphVtx curVtx = goalData->m_Vtx;
      ES_RuleSystem::GraphVtx prevVtx = curVtx;

      if (iNodeFilter.is_valid() || iEdgeFilter.is_valid()) 
      {
        GraphFilter filter;
        UnorderedSet<ES_RuleSystem::GraphVtx> validVtx;
        UnorderedSet<ES_RuleSystem::GraphEdge> validEdge;
        
        if (iNodeFilter)
        {
          LuaStateHandle curState = LuaManager::GetCurrentState();
          eXl_ASSERT(curState.GetState() != nullptr);
          m_NodeData.Iterate([&](ObjectHandle iHandle, const LevelNodeData& iNodeData) {
              auto callCtx = curState.PrepareCall(iNodeFilter);
              callCtx.PushArgs(iHandle);
              auto callRes = callCtx.Call(1);
              if (callRes && *callRes == 1) {
                lua_State* state = curState.GetState();
                unsigned int boolVal = lua_toboolean(state, lua_gettop(state));
                if (boolVal != 0) {
                  validVtx.insert(iNodeData.m_Vtx);
                }
              }
            }
          );
          filter.m_ValidVtx = &validVtx;
        }
        if (iEdgeFilter)
        {
          LuaStateHandle curState = LuaManager::GetCurrentState();
          eXl_ASSERT(curState.GetState() != nullptr);
          m_EdgeData.Iterate([&](ObjectHandle iHandle, const LevelEdgeData& iEdgeData) {
            auto callCtx = curState.PrepareCall(iEdgeFilter);
            callCtx.PushArgs(iHandle);
            auto callRes = callCtx.Call(1);
            if (callRes && *callRes == 1) {
              lua_State* state = curState.GetState();
              unsigned int boolVal = lua_toboolean(state, lua_gettop(state));
              if (boolVal != 0) {
                validEdge.insert(iEdgeData.m_Edge);
              }
            }
            });
          filter.m_ValidEdge = &validEdge;
        }

        if (filter.m_ValidVtx) {
          if (validVtx.count(startData->m_Vtx) == 0
            || validVtx.count(goalData->m_Vtx) == 0) {
            return Vector<ObjectHandle>();
          }
        }
        
        boost::filtered_graph<ES_RuleSystem::Graph, GraphFilter, GraphFilter> filteredGr(m_Graph, filter, filter);
        auto filteredIndexMap = MakeIndexMap(filteredGr);
        boost::dijkstra_shortest_paths(filteredGr, startData->m_Vtx,
          boost::make_iterator_property_map(p.begin(), filteredIndexMap),
          boost::make_iterator_property_map(d.begin(), filteredIndexMap),
          boost::make_function_property_map<ES_RuleSystem::GraphEdge>(dummyWhFunc),
          filteredIndexMap,
          std::less<float>(), boost::closed_plus<float>(), Mathf::MaxReal(), 0.0, boost::dijkstra_visitor<boost::null_visitor>());

        do
        {
          LevelNodeData const* targetNodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_Graph, curVtx));
          path.push_back(targetNodeData->m_Object);
          prevVtx = curVtx;
          curVtx = p[filteredIndexMap[curVtx]];
        } while (prevVtx != curVtx);
      }
      else
      {
        auto const& idxMap = boost::get(boost::vertex_index, m_Graph);
        boost::dijkstra_shortest_paths(m_Graph, startData->m_Vtx,
          boost::make_iterator_property_map(p.begin(), idxMap),
          boost::make_iterator_property_map(d.begin(), idxMap),
          boost::make_function_property_map<ES_RuleSystem::GraphEdge>(dummyWhFunc),
          idxMap,
          std::less<float>(), boost::closed_plus<float>(), Mathf::MaxReal(), 0.0, boost::dijkstra_visitor<boost::null_visitor>());

        do
        {
          LevelNodeData const* targetNodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_Graph, curVtx));
          path.push_back(targetNodeData->m_Object);
          prevVtx = curVtx;
          curVtx = p[idxMap[curVtx]];
        } while (prevVtx != curVtx);
      }
      
      if (path.empty() || path.back() != iStart)
      {
        return Vector<ObjectHandle>();
      }

      std::reverse(path.begin(), path.end());
      return path;
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

    Name GraphWrapper::GetEdgeTag(ObjectHandle iEdge) const
    {
      LevelEdgeData const* edgeData = m_EdgeData.Get(iEdge);
      eXl_ASSERT_REPAIR_RET(edgeData != nullptr, Name());

      return edgeData->m_Tag;
    }

    Name GraphWrapper::GetNodeTag(ObjectHandle iNode) const
    {
      LevelNodeData const* nodeData = m_NodeData.Get(iNode);
      eXl_ASSERT_REPAIR_RET(nodeData != nullptr, Name());

      return nodeData->m_Tag;
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

    RewriteWrapper::RewriteWrapper(GraphWrapper const& iSrcGraph
      , GraphWrapper const& iDstGraph
      , Vector<ES_RuleSystem::GraphVtx> const& iMatch)
      : m_SrcGraph(iSrcGraph)
      , m_DstGraph(iDstGraph)
    {
      for (auto vtx : iMatch)
      {
        m_Match.push_back(m_SrcGraph.GetNodeObject(vtx));
      }
    }

    ObjectHandle GraphFactoryWrapper::CreateNode(Name iTag) const 
    {
      auto iter = m_System.m_Tags.find(iTag);

      eXl_ASSERT_REPAIR_RET(iter != m_System.m_Tags.end(), ObjectHandle());
      eXl_ASSERT_REPAIR_RET(iter->second.m_IsNodeTag, ObjectHandle());

      auto newVtx = boost::add_vertex(m_DstGraph.m_Graph);
      ObjectHandle newObj = m_DstGraph.AddNode(newVtx);
      LevelNodeData* node = m_DstGraph.m_NodeData.Get(newObj);
      node->m_Tag = iTag;
      m_DstGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(newObj, iter->second.m_Archetype.GetOrLoad(), nullptr);

      return newObj;
    }

    void GraphFactoryWrapper::SetDebugString(ObjectHandle iNode, const char* iStr) const
    {
      if (iStr) 
      {
        if (LevelNodeData* node = m_DstGraph.m_NodeData.Get(iNode))
        {
          node->m_DebugString = iStr;
        }
      }
      
    }

    ObjectHandle GraphFactoryWrapper::CreateEdge(ObjectHandle iNode1, ObjectHandle iNode2, Name iTag) const
    {
      auto iter = m_System.m_Tags.find(iTag);

      eXl_ASSERT_REPAIR_RET(iter != m_System.m_Tags.end(), ObjectHandle());
      eXl_ASSERT_REPAIR_RET(!iter->second.m_IsNodeTag, ObjectHandle());

      LevelNodeData const* node1 = m_DstGraph.m_NodeData.Get(iNode1);
      LevelNodeData const* node2 = m_DstGraph.m_NodeData.Get(iNode2);

      eXl_ASSERT_REPAIR_RET(node1 && node2, ObjectHandle());

      auto newEdge = boost::add_edge(node1->m_Vtx, node2->m_Vtx, m_DstGraph.m_Graph);
      if(newEdge.second)
      {
        ObjectHandle newObj = m_DstGraph.AddEdge(newEdge.first);
        LevelEdgeData* edge = m_DstGraph.m_EdgeData.Get(newObj);
        edge->m_Tag = iTag; 
        m_DstGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(newObj, iter->second.m_Archetype.GetOrLoad(), nullptr);

        return newObj;
      }
      else 
      {
        return m_DstGraph.GetEdgeObject(newEdge.first);
      }
    }

    IMPLEMENT_RTTI(RewriteSystem);

    using RewriteSystemLoader = TResourceLoader <RewriteSystem, ResourceLoader>;

    LUA_REG_FUN(BindGraphWrappers)
    {
      luabind::module(iState, "eXl")[
        luabind::class_<GraphWrapper>("GraphWrapper")
          .def("GetEdges", &GraphWrapper::GetEdges)
          .def("GetTargetNode", &GraphWrapper::GetTargetNode)
          .def("GetNodeTag", &GraphWrapper::GetNodeTag)
          .def("GetEdgeTag", &GraphWrapper::GetEdgeTag)
          .def("FindPath", &GraphWrapper::FindPath)
          ,

          luabind::class_<MatchWrapper>("MatchWrapper")
          .def("Graph", &MatchWrapper::GetGraph),

          luabind::class_<RewriteWrapper>("RewriteWrapper")
          .def("SourceGraph", &RewriteWrapper::GetSrcGraph)
          .def("TargetGraph", &RewriteWrapper::GetDstGraph)
          .def("Match", &RewriteWrapper::GetMatch),

          luabind::class_<GraphFactoryWrapper>("GraphFactoryWrapper")
          .def("CreateNode", &GraphFactoryWrapper::CreateNode)
          .def("CreateEdge", &GraphFactoryWrapper::CreateEdge)
          .def("SetDebugString", &GraphFactoryWrapper::SetDebugString)

      ];

      return 0;
    }

    void RewriteSystem::Init()
    {
      EventsManifest::FunctionsMap functions;
      functions.insert(std::make_pair("CheckNode", FunDesc::Create<bool(MatchWrapper&, uint32_t, ObjectHandle)>()));
      functions.insert(std::make_pair("CheckEdge", FunDesc::Create<bool(MatchWrapper&, uint32_t, ObjectHandle)>()));
      functions.insert(std::make_pair("CheckMatch", FunDesc::Create<bool(MatchWrapper&, Vector<ObjectHandle>)>()));
      functions.insert(std::make_pair("CreateNode", FunDesc::Create<void(RewriteWrapper&, uint32_t, ObjectHandle)>()));
      functions.insert(std::make_pair("CreateEdge", FunDesc::Create<void(RewriteWrapper&, uint32_t, ObjectHandle)>()));
      functions.insert(std::make_pair("RemoveNode", FunDesc::Create<void(RewriteWrapper&, ObjectHandle)>()));
      functions.insert(std::make_pair("RemoveEdge", FunDesc::Create<void(RewriteWrapper&, ObjectHandle)>()));
      functions.insert(std::make_pair("PostRewrite", FunDesc::Create<void(RewriteWrapper&, GraphFactoryWrapper)>()));

      EngineCommon::GetBaseEvents().m_Interfaces.insert(std::make_pair("RewriteRule", functions));

      functions.clear();
      functions.insert(std::make_pair("ProcessRoom", FunDesc::Create<Vector<AABB2Di>(ObjectHandle, Vector<AABB2Di>)>()));

      EngineCommon::GetBaseEvents().m_Interfaces.insert(std::make_pair("LayoutPostProcess", functions));

      ResourceManager::AddLoader(&RewriteSystemLoader::Get(), RewriteSystem::StaticRtti(), RewriteSystem::GetType());
      EngineCommon::GetBaseProperties().RegisterPropertySheet<RoomLayoutInfo>("RoomLayoutInfo", true);

      LuaManager::AddRegFun(&BindGraphWrappers);
    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    RewriteSystem* RewriteSystem::Create(Path const& iDir, String const& iName)
    {
      return RewriteSystemLoader::Get().CreateAt(iDir, iName);
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

      iStreamer.PushKey("CurrentSequence");
      iStreamer &= m_CurSequence;
      iStreamer.PopKey();

      iStreamer.EndStruct();

      return Err::Success;
    }
}