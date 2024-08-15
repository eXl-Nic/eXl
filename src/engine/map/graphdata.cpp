#include <engine/map/graphdata.hpp>
#include <gen/graphutils.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <core/type/tagtype.hpp>
#include <engine/game/commondef.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/function_property_map.hpp>

#include <core/stream/textreader.hpp>
#include <core/stream/jsonstreamer.hpp>
#include <core/stream/jsonunstreamer.hpp>

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
    IMPLEMENT_SERIALIZE_METHODS(RewriteSystemRsc::RuleAdditionalData)
    IMPLEMENT_SERIALIZE_METHODS(RewriteSystemRsc::TagAdditionalData)

    //PropertySheetName RoomLayoutInfo::PropertyName() {
    //  static PropertySheetName s_Name("RoomLayoutInfo");
    //  return s_Name;
    //}
    
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
    IMPLEMENT_RTTI(RewriteSystemRsc);

    using RewriteSystemLoader = TResourceLoader <RewriteSystemRsc, ResourceLoader>;

    LUA_REG_FUN(BindGraphWrappers)
    {
      TypeManager::GetType<RoomLayoutInfo>()->RegisterLua(iState);

      luabind::module(iState, "eXl")[
        luabind::class_<GraphWrapper>()
          .def("GetEdges", &GraphWrapper::GetEdges)
          .def("GetTargetNode", &GraphWrapper::GetTargetNode)
          .def("GetNodeTag", &GraphWrapper::GetNodeTag)
          .def("GetEdgeTag", &GraphWrapper::GetEdgeTag)
          .def("FindPath", &GraphWrapper::FindPath)
          ,

          luabind::class_<MatchWrapper>()
          .def("Graph", &MatchWrapper::GetGraph),

          luabind::class_<RewriteWrapper>()
          .def("SourceGraph", &RewriteWrapper::GetSrcGraph)
          .def("TargetGraph", &RewriteWrapper::GetDstGraph)
          .def("Match", &RewriteWrapper::GetMatch),

          luabind::class_<GraphFactoryWrapper>()
          .def("SetDebugString", &GraphFactoryWrapper::SetDebugString)

      ];
      return 0;
    }

    Type const* Get_eXl__RoomLayoutInfo_Type() 
    {
      static TupleType const* layoutInfoType = TypeManager::BeginNativeTypeRegistration<RoomLayoutInfo>("RoomLayoutInfo")
        .AddField("m_CollapseNode", &RoomLayoutInfo::m_CollapseNode)
        .AddField("m_Layout", &RoomLayoutInfo::m_Layout)
        .AddField("m_RoomSizes", &RoomLayoutInfo::m_RoomSizes)
        .AddField("m_TerrainType", &RoomLayoutInfo::m_TerrainType)
        .EndRegistration();

      return layoutInfoType;
    }

    void RewriteSystemRsc::Init()
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

      ResourceManager::AddLoader(&RewriteSystemLoader::Get(), RewriteSystemRsc::StaticRtti(), RewriteSystemRsc::GetType());

      EngineCommon::GetBaseProperties().RegisterPropertySheet<RoomLayoutInfo>("RoomLayoutInfo", true);

      LuaManager::AddRegFun(&BindGraphWrappers);
    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    RewriteSystemRsc* RewriteSystemRsc::Create(Path const& iDir, String const& iName)
    {
      return RewriteSystemLoader::Get().CreateAt(iDir, iName);
    }
#endif

    RewriteSystemRsc::RewriteSystemRsc(ResourceMetaData& iMeta)
      : Resource(iMeta)
    {}

    RewriteSystemRsc::~RewriteSystemRsc() = default;

    uint32_t RewriteSystemRsc::ComputeHash()
    {
      return 0;
    }

    ResourceLoaderName RewriteSystemRsc::StaticLoaderName()
    {
      return ResourceLoaderName("RewriteSystem");
    }

    void FindCustomData(String const& customData, JSONUnstreamer::ElementDesc& eXlCustomData, uint32_t& numCustomElems, bool fullEntry)
    {
      numCustomElems = 0;
      eXlCustomData.elemBegin = eXlCustomData.elemEnd = 0;
      if(customData.empty())
      {
        return;
      }

      StringViewReader strReader(String(), &(*customData.begin()), &customData.back() + 1);
      JSONUnstreamer unstreamer(&strReader);
      if (unstreamer.Begin())
      {
        if (unstreamer.BeginStruct())
        {
          numCustomElems = reinterpret_cast<JSONUnstreamer::ElemStruct const&>(unstreamer.GetCurrentElement()).m_Fields.size();
          if (unstreamer.PushKey("eXl_Data"))
          {
            eXlCustomData = unstreamer.GetCurrentElement();
            if (fullEntry) {
              size_t off = customData.rfind(",", eXlCustomData.elemBegin - 1);
              if (off == String::npos)
              {
                off = customData.rfind("{", eXlCustomData.elemBegin - 1);
              }
              eXl_ASSERT(off != String::npos);
              ++off;
              eXlCustomData.elemBegin = off;
            }
            unstreamer.PopKey();
          }
          unstreamer.End();
        }
        unstreamer.End();
      }
    }

    void PatchCustomData(String& customData, String const& curData, JSONUnstreamer::ElementDesc& eXlCustomData, uint32_t& numCustomElems)
    {
      if (eXlCustomData.elemEnd > eXlCustomData.elemBegin)
      {
        if (!curData.empty()) 
        {
          customData.replace(eXlCustomData.elemBegin, eXlCustomData.elemEnd - eXlCustomData.elemBegin, "\"eXl_Data\" : " + curData);
        }
        else
        {
          customData.erase(eXlCustomData.elemBegin, eXlCustomData.elemEnd - eXlCustomData.elemBegin);
        }
      }
      else if(!curData.empty())
      {
        if(customData.empty())
        {
          customData = "{}";
        }
        eXl_ASSERT(customData.back() == '}');
        if (numCustomElems > 0)
        {
          customData.replace(customData.end() - 1, customData.end(), ",}");
        }
        customData.replace(customData.end() - 1, customData.end(), "\"eXl_Data\" : " + curData + "}");
      }
    }

    Err RewriteSystemRsc::RuleAdditionalData::Serialize(Serializer serializer) 
    {
      Err err = serializer.BeginStruct();
      err &= serializer.PushKey("Script");
      err &= serializer &= m_Script;
      err &= serializer.PopKey();
      err &= serializer.EndStruct();

      return Err::Success;
    }

    Err RewriteSystemRsc::TagAdditionalData::Serialize(Serializer serializer)
    {
      Err err = serializer.BeginStruct();
      err &= serializer.PushKey("Archetype");
      err &= (serializer &= m_Archetype);
      err &= serializer.PopKey();
      err &= serializer.EndStruct();

      return err;
    }

    Err RewriteSystemRsc::Stream_Data(Streamer& iStreamer) const
    {
      RewriteSystem& sys = const_cast<RewriteSystem&>(m_Sys);
      for (auto& rule : sys.m_Rules)
      {
        String& customData = rule.second.m_RuleCustomData;
        
        JSONUnstreamer::ElementDesc eXlCustomData;
        uint32_t numCustomElems;
        FindCustomData(customData, eXlCustomData, numCustomElems, true);

        String curData;
        auto ruleAddData = m_Rules.find(rule.first);
        if (ruleAddData != m_Rules.end()) 
        {
          std::stringstream sstream;
          JSONStreamer streamer(&sstream);
          Err res = streamer.Begin();
          ruleAddData->second.Stream(streamer);
          res = streamer.End();
          curData.append( sstream.str() );
        }

        PatchCustomData(customData, curData, eXlCustomData, numCustomElems);
      }

      for (auto& tag : sys.m_Tags)
      {
        String& customData = tag.second.m_TagCustomData;
        JSONUnstreamer::ElementDesc eXlCustomData;
        uint32_t numCustomElems;
        FindCustomData(customData, eXlCustomData, numCustomElems, true);

        String curData;
        auto tagAddData = m_Tags.find(tag.first);
        if (tagAddData != m_Tags.end())
        {
          std::stringstream sstream;
          JSONStreamer streamer(&sstream);
          Err res = streamer.Begin();
          tagAddData->second.Stream(streamer);
          res = streamer.End();
          curData = sstream.str();
        }

        PatchCustomData(customData, curData, eXlCustomData, numCustomElems);
      }

      return m_Sys.Stream(iStreamer);
    }

    Err RewriteSystemRsc::Unstream_Data(Unstreamer& iStreamer)
    {
      Err err = m_Sys.Unstream(iStreamer);
      if ( err ) 
      {
        for (auto const& rule : m_Sys.m_Rules)
        {
          String const& customData = rule.second.m_RuleCustomData;

          JSONUnstreamer::ElementDesc eXlCustomData;
          uint32_t numCustomElems;
          FindCustomData(customData, eXlCustomData, numCustomElems, false);
          if (eXlCustomData.elemEnd <= eXlCustomData.elemBegin) {
            continue;
          }
          StringViewReader strReader(String(), &(*customData.begin()) + eXlCustomData.elemBegin, &(*customData.begin()) + eXlCustomData.elemEnd);
          JSONUnstreamer unstreamer(&strReader);
          if (unstreamer.Begin())
          {
            RuleAdditionalData addData;
            Err err = addData.Unstream(unstreamer);
            if (err) 
            {
              m_Rules.insert(std::make_pair(rule.first, addData));
            }
            unstreamer.End();
          }
        }
        for (auto const& tag : m_Sys.m_Tags)
        {
          String const& customData = tag.second.m_TagCustomData;

          JSONUnstreamer::ElementDesc eXlCustomData;
          uint32_t numCustomElems;
          FindCustomData(customData, eXlCustomData, numCustomElems, false);
          if (eXlCustomData.elemEnd <= eXlCustomData.elemBegin) {
            continue;
          }
          StringViewReader strReader(String(), &(*customData.begin()) + eXlCustomData.elemBegin, &(*customData.begin()) + eXlCustomData.elemEnd);
          JSONUnstreamer unstreamer(&strReader);
          if (unstreamer.Begin())
          {
            TagAdditionalData addData;
            Err err = addData.Unstream(unstreamer);
            if (err)
            {
              m_Tags.insert(std::make_pair(tag.first, addData));
            }
            unstreamer.End();
          }
        }
      }

      return err;
    }
}