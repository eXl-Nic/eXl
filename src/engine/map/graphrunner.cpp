/*
Copyright 2009-2023 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <engine/map/graphrunner.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <gen/graphutils.hpp>
#include <gen/graphlayout.hpp>

#include <core/random.hpp>

namespace eXl
{
  GraphRunner::GraphRunner(RewriteSystemRsc const & iSys, World& iWorld, NodeData& iNodeData, EdgeData& iEdgeData)
    : m_World(iWorld)
    , m_SysRsc(iSys)
    , m_Sys(iSys.m_Sys)
    , m_NodeData(iNodeData)
    , m_EdgeData(iEdgeData)
  {
  }

  class RuleData : public HeapObject
  {
    DECLARE_RefC;
  public:
    RewriteSystemRsc const* rewriteSysRsc;
    RewriteSystem const* rewriteSys;
    ObjectHandle ruleObject;
    Vector<Name> nodeTags;
    Vector<Name> newNodeTags;
    Vector<Name> edgeTags;
    Vector<Name> newEdgeTags;
  };
  IMPLEMENT_RefC(RuleData);

  struct SimMatchCtx : ES_RuleSystem::UserMatchContext
  {
    DECLARE_RTTI(SimMatchCtx, ES_RuleSystem::UserMatchContext);

    SimMatchCtx(GraphWrapper const& iSrc)
      : m_SourceGraph(iSrc)
    {}
    GraphWrapper const& m_SourceGraph;
  };

  struct SimRewriteCtx : ES_RuleSystem::UserRewriteContext
  {
    DECLARE_RTTI(SimRewriteCtx, ES_RuleSystem::UserRewriteContext);

    SimRewriteCtx(GraphWrapper const& iSrc, GraphWrapper& iDst)
      : m_SourceGraph(iSrc)
      , m_DestGraph(iDst)
    {}
    GraphWrapper const& m_SourceGraph;
    GraphWrapper& m_DestGraph;
  };

  IMPLEMENT_RTTI(SimMatchCtx);
  IMPLEMENT_RTTI(SimRewriteCtx);

  void GraphRunner::RunRules(Random& iRand, Vector<RuleItem> const& iRules, ES_RuleSystem::Graph& oGraph)
  {
    LuaScriptSystem& luaSys = *m_World.GetSystem<LuaScriptSystem>();
    oGraph.clear();
    m_NodeData.Clear();
    m_EdgeData.Clear();

    Vector<String> rules;
    UnorderedMap<String, uint32_t> rulesIdx;

    Vector<Name> tags;
    UnorderedMap<Name, uint32_t> tagsIdx;
    tagsIdx.insert(std::make_pair(RewriteSystem::GetAnyTag(), UINT32_MAX));

    Vector<ObjectHandle> ruleObjects;

    ES_RuleSystem sys;

    for (auto const& tag : m_Sys.m_Tags)
    {
      tags.push_back(tag.first);
      tagsIdx.insert(std::make_pair(tag.first, tags.size() - 1));
    }
    for (auto const& ruleEntry : m_Sys.m_Rules)
    {
      IntrusivePtr<RuleData> data = MakeRefCounted<RuleData>();
      data->rewriteSys = &m_Sys;
      data->rewriteSysRsc = &m_SysRsc;
      data->ruleObject = m_World.CreateObject();
      ruleObjects.push_back(data->ruleObject);

      auto iter = m_SysRsc.m_Rules.find(ruleEntry.first);
      if (LuaEventHandler const* script = iter != m_SysRsc.m_Rules.end() ? iter->second.m_Script.GetOrLoad() : nullptr)
      {
        if (script->m_InterfaceName == "RewriteRule")
        {
          luaSys.AddHandler(data->ruleObject, *script);
        }
      }

      auto checkNodeTag = [data](ES_RuleSystem::MatchCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphVtx iVtx)
      {
        SimMatchCtx const& ctx = *SimMatchCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle nodeObj = ctx.m_SourceGraph.GetNodeObject(iVtx);
        Name nodeTag = ctx.m_SourceGraph.m_NodeData.Get(nodeObj)->m_Tag;
        if (data->nodeTags[iIdx] == RewriteSystem::GetAnyTag()
          || data->nodeTags[iIdx] == nodeTag)
        {
          static Name const checkNodeEvt("RewriteRule::CheckNode");
          EventSystem& evtSys = *ctx.m_SourceGraph.m_World.GetSystem<EventSystem>();
          if (evtSys.GetEventHandlerInternal(data->ruleObject, checkNodeEvt) == nullptr)
          {
            return true;
          }

          MatchWrapper wrapper(ctx.m_SourceGraph);
          Optional<bool> ret = evtSys.Dispatch<bool>(data->ruleObject, checkNodeEvt, wrapper, iIdx, nodeObj);

          eXl_ASSERT_MSG_REPAIR_RET(ret, "Invalid return type for CheckNode function", false);

          return *ret;
        }

        return false;
      };

      auto checkEdgeTag = [data](ES_RuleSystem::MatchCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphEdge iEdge)
      {
        SimMatchCtx const& ctx = *SimMatchCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle edgeObj = ctx.m_SourceGraph.GetEdgeObject(iEdge);
        Name edgeTag = ctx.m_SourceGraph.m_EdgeData.Get(edgeObj)->m_Tag;
        if (data->edgeTags[iIdx] == RewriteSystem::GetAnyTag()
          || data->edgeTags[iIdx] == edgeTag)
        {
          static Name const checkEdgeEvt("RewriteRule::CheckEdge");
          EventSystem& evtSys = *ctx.m_SourceGraph.m_World.GetSystem<EventSystem>();
          if (evtSys.GetEventHandlerInternal(data->ruleObject, checkEdgeEvt) == nullptr)
          {
            return true;
          }

          MatchWrapper wrapper(ctx.m_SourceGraph);
          Optional<bool> ret = evtSys.Dispatch<bool>(data->ruleObject, checkEdgeEvt, wrapper, iIdx, edgeObj);

          eXl_ASSERT_MSG_REPAIR_RET(ret, "Invalid return type for CheckEdge function", false);

          return *ret;
        }

        return false;
      };

      auto checkMatch = [data](ES_RuleSystem::MatchCtx& iCtx, Vector<ES_RuleSystem::GraphVtx> const& iMatch)
      {
        SimMatchCtx const& ctx = *SimMatchCtx::DynamicCast(iCtx.userCtx);

        static Name const checkMatchEvt("RewriteRule::CheckMatch");
        EventSystem& evtSys = *ctx.m_SourceGraph.m_World.GetSystem<EventSystem>();
        if (evtSys.GetEventHandlerInternal(data->ruleObject, checkMatchEvt) == nullptr)
        {
          return true;
        }

        Vector<ObjectHandle> nodeObjects;

        for (auto vtx : iMatch)
        {
          nodeObjects.push_back(ctx.m_SourceGraph.GetNodeObject(vtx));
        }

        MatchWrapper wrapper(ctx.m_SourceGraph);
        Optional<bool> ret = evtSys.Dispatch<bool>(data->ruleObject, checkMatchEvt, wrapper, nodeObjects);

        eXl_ASSERT_MSG_REPAIR_RET(ret, "Invalid return type for CheckMatch function", false);

        return *ret;
      };

      auto createNode = [data](ES_RuleSystem::RewriteCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphVtx iVtx)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle nodeObj = ctx.m_DestGraph.AddNode(iVtx);
        Name tag = data->newNodeTags[iIdx];
        ctx.m_DestGraph.m_NodeData.Get(nodeObj)->m_Tag = tag;
        auto iter = data->rewriteSysRsc->m_Tags.find(tag);
        if (iter != data->rewriteSysRsc->m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          ctx.m_DestGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(nodeObj, arch, nullptr);
        }

        static Name const createNodeEvt("RewriteRule::CreateNode");
        EventSystem& evtSys = *ctx.m_SourceGraph.m_World.GetSystem<EventSystem>();
        RewriteWrapper wrapper(ctx.m_SourceGraph, ctx.m_DestGraph, iCtx.match);
        evtSys.Dispatch<void>(data->ruleObject, createNodeEvt, wrapper, iIdx, nodeObj);

      };

      auto createEdge = [data](ES_RuleSystem::RewriteCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphEdge iEdge)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle edgeObj = ctx.m_DestGraph.AddEdge(iEdge);
        Name tag = data->newEdgeTags[iIdx];
        ctx.m_DestGraph.m_EdgeData.Get(edgeObj)->m_Tag = tag;
        auto iter = data->rewriteSysRsc->m_Tags.find(tag);
        if (iter != data->rewriteSysRsc->m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          ctx.m_DestGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(edgeObj, arch, nullptr);
        }

        static Name const createEdgeEvt("RewriteRule::CreateEdge");
        EventSystem& evtSys = *ctx.m_SourceGraph.m_World.GetSystem<EventSystem>();
        RewriteWrapper wrapper(ctx.m_SourceGraph, ctx.m_DestGraph, iCtx.match);
        evtSys.Dispatch<void>(data->ruleObject, createEdgeEvt, wrapper, iIdx, edgeObj);
      };

      auto removeNode = [data](ES_RuleSystem::RewriteCtx& iCtx, ES_RuleSystem::GraphVtx iVtx)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ctx.m_DestGraph.RemoveNode(iVtx);
      };

      auto removeEdge = [data](ES_RuleSystem::RewriteCtx& iCtx, ES_RuleSystem::GraphEdge iEdge)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ctx.m_DestGraph.RemoveEdge(iEdge);
      };

      rules.push_back(ruleEntry.first);
      rulesIdx.insert(std::make_pair(ruleEntry.first, rules.size() - 1));
      auto const& rule = ruleEntry.second;

      ES_RuleSystem::RuleBuilder builder;

      for (auto const& node : rule.m_ContextNodes)
      {
        auto iter = tagsIdx.find(node);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", node.c_str()), void());

        builder.AddNode(iter->second, checkNodeTag);
        data->nodeTags.push_back(node);
      }

      for (auto const& node : rule.m_CutNodes)
      {
        auto iter = tagsIdx.find(node);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", node.c_str()), void());

        builder.AddCutNode(iter->second, checkNodeTag, removeNode);
        data->nodeTags.push_back(node);
      }

      for (auto const& node : rule.m_CreateNodes)
      {
        auto iter = tagsIdx.find(node);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", node.c_str()), void());

        builder.AddNewNode(iter->second, createNode);
        data->newNodeTags.push_back(node);
      }

      for (auto const& edgeDesc : rule.m_ContextEdges)
      {
        auto iter = tagsIdx.find(edgeDesc.tag);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", edgeDesc.tag.c_str()), void());

        builder.AddConnection(edgeDesc.nodes[0], edgeDesc.nodes[1], iter->second, checkEdgeTag);
        data->edgeTags.push_back(edgeDesc.tag);
      }

      for (auto const& edgeDesc : rule.m_CutEdge)
      {
        auto iter = tagsIdx.find(edgeDesc.tag);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", edgeDesc.tag.c_str()), void());

        builder.AddCutConnection(edgeDesc.nodes[0], edgeDesc.nodes[1], iter->second, checkEdgeTag, removeEdge);
        data->edgeTags.push_back(edgeDesc.tag);
      }

      for (auto const& edgeDesc : rule.m_NewEdge)
      {
        auto iter = tagsIdx.find(edgeDesc.tag);
        eXl_ASSERT_MSG_REPAIR_RET(iter != tagsIdx.end(), eXl_FORMAT("Tag %s not found", edgeDesc.tag.c_str()), void());

        builder.AddNewConnection(edgeDesc.nodes[0], edgeDesc.nodes[1], edgeDesc.port[0], edgeDesc.port[1], iter->second, createEdge);
        data->newEdgeTags.push_back(edgeDesc.tag);
      }
      builder.End(sys, checkMatch);
    }

    Vector<ObjectHandle> oldNodes;

    auto PostRewriteOps = [this](ES_RuleSystem::Graph& curGraph, 
      ES_RuleSystem::Graph& newGraph,
      GraphWrapper& srcGraphWrapper, 
      GraphWrapper& dstGraphWrapper,
      ObjectHandle ruleObj, 
      const ES_RuleSystem::VertexMatching& match,
      eXl::SimRewriteCtx& rewriteCtx) {
      static Name const postRewrite("RewriteRule::PostRewrite");
      EventSystem& evtSys = *rewriteCtx.m_DestGraph.m_World.GetSystem<EventSystem>();
      if (evtSys.GetEventHandlerInternal(ruleObj, postRewrite) != nullptr)
      {
        RewriteWrapper rwWrapper(srcGraphWrapper, dstGraphWrapper, match);
        GraphFactoryWrapper factory(dstGraphWrapper, m_Sys);
        evtSys.Dispatch<void>(ruleObj, postRewrite, rwWrapper, factory);
      }
      curGraph = newGraph;
      for (auto vtx : VerticesIter(curGraph))
      {
        ES_RuleSystem::NodeData const* data = boost::get(boost::vertex_name, curGraph, vtx);
        data->CopyNode(vtx);
      }
      for (auto const & edge : EdgesIter(curGraph))
      {
        ES_RuleSystem::EdgeData const* data = boost::get(boost::edge_name, curGraph, edge);
        data->CopyEdge(edge);
      }

      uint32_t nodeIdx = 0;
      for (auto vtx : eXl::VerticesIter(curGraph))
      {
        boost::put(boost::vertex_index, curGraph, vtx, nodeIdx++);
      }
    };

    for (uint32_t i = 0; i < iRules.size(); ++i)
    {
      oldNodes.clear();

      for (auto vtx : VerticesIter(oGraph))
      {
        LevelNodeData const* data = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, oGraph, vtx));
        oldNodes.push_back(data->m_Object);
      }
      for (auto const& edge : EdgesIter(oGraph))
      {
        LevelEdgeData const* data = LevelEdgeData::DynamicCast(boost::get(boost::edge_name, oGraph, edge));
        oldNodes.push_back(data->m_Object);
      }

      RuleItem const& item = iRules[i];

      auto iter = rulesIdx.find(item.ruleName);
      if (iter != rulesIdx.end())
      {
        GraphWrapper srcGraphWrapper(m_World, oGraph, m_NodeData, m_EdgeData);

        SimMatchCtx matchCtx(srcGraphWrapper);
        if (item.application > 0)
        {
          for (int i = 0; i < item.application; ++i) 
          {
            auto matchings = sys.FindRuleMatch(iter->second, oGraph, &matchCtx);
            if (matchings.size() > 0)
            {
              uint32_t matchToConsider = iRand.Generate() % matchings.size();

              ES_RuleSystem::Graph newGraph;
              GraphWrapper dstGraphWrapper(m_World, newGraph, m_NodeData, m_EdgeData);

              SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
              sys.ApplyRule(oGraph, newGraph, iter->second, matchings[matchToConsider], &rewriteCtx);
              PostRewriteOps(oGraph, newGraph, srcGraphWrapper, dstGraphWrapper, ruleObjects[iter->second], matchings[matchToConsider], rewriteCtx);
            }
          }
        }
        else
        {
          ES_RuleSystem::Graph newGraph;
          GraphWrapper dstGraphWrapper(m_World, newGraph, m_NodeData, m_EdgeData);

          SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
          sys.ApplyRuleParallel(oGraph, newGraph, iter->second, &matchCtx, &rewriteCtx);
          PostRewriteOps(oGraph, newGraph, srcGraphWrapper, dstGraphWrapper, ruleObjects[iter->second], ES_RuleSystem::VertexMatching(), rewriteCtx);
        }
      }
    }
  }

  void FindVtxToConnect(GameDataView<RoomLayoutInfo>& iLayoutData,
    ES_RuleSystem::Graph const& iGraph,
    ES_RuleSystem::GraphVtx iCollapseVtx,
    SmallVector<ES_RuleSystem::GraphVtx, 2>& oVtxToConnect,
    UnorderedSet<ES_RuleSystem::GraphVtx>& oCollapsed)
  {
    if (oCollapsed.count(iCollapseVtx) != 0)
    {
      return;
    }
    oCollapsed.insert(iCollapseVtx);
    for (auto edge : OutEdgesIter(iGraph, iCollapseVtx))
    {
      ES_RuleSystem::GraphVtx target = GetTarget(iCollapseVtx, edge);
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, iGraph, target));
      ObjectHandle nodeObj = nodeData->m_Object;
      if (auto layoutInfo = iLayoutData.Get(nodeObj))
      {
        if (layoutInfo->m_CollapseNode)
        {
          FindVtxToConnect(iLayoutData, iGraph, target, oVtxToConnect, oCollapsed);
        }
        else if (!layoutInfo->m_CollapseNode && !layoutInfo->m_RoomSizes.empty())
        {
          auto iter = std::find(oVtxToConnect.begin(), oVtxToConnect.end(), iCollapseVtx);
          if (iter == oVtxToConnect.end())
          {
            oVtxToConnect.push_back(target);
          }
        }
      }
    }
  }

  using VtxMapping = UnorderedMap<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx>;

  void MakeEdge(
    ES_RuleSystem::Graph& oGraph,
    VtxMapping const& iGraphMap,
    UnorderedSet<std::pair<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx>>& iHandledPairs,
    VtxMapping::iterator iVtx1, VtxMapping::iterator iVtx2
  )
  {
    auto pair = std::make_pair(iVtx1->first, iVtx2->first);
    if (pair.first > pair.second)
    {
      std::swap(pair.first, pair.second);
    }
    if (iHandledPairs.count(pair) == 0)
    {
      auto newEdge = boost::add_edge(iVtx1->second, iVtx2->second, oGraph).first;
      EdgeLayoutData* layoutData = new EdgeLayoutData;
      layoutData->m_Ignore = false;
      boost::put(boost::edge_name, oGraph, newEdge, layoutData);
      iHandledPairs.insert(pair);
    }
  }

  void FillTerrain(ES_RuleSystem::Graph const& iGraph,
    //Layout const& iRooms,
    Vector<AABB2Di>& iRooms,
    Vector<AABB2DPolygoni>& oRooms,
    Vector<AABB2DPolygoni>& oWalls)
  {
    //uint32_t scaleFactor = 4;
    for (auto const& room : iRooms)
    {
      if (iRooms.size() != 1)
      {
        //AABB2Di scaledRoom = room;
        //scaledRoom.m_Min *= scaleFactor;
        //scaledRoom.m_Max *= scaleFactor;
        AABB2DPolygoni tempRoom(room);
        Vector<AABB2DPolygoni> tempShrink;
        //tempRoom.Shrink(2, tempShrink);
        //oRooms.insert(oRooms.end(), tempShrink.begin(), tempShrink.end());
        // 
        oRooms.push_back(room);

      }
      else
      {
        oRooms.push_back(room);
        //oRooms[0].Scale(scaleFactor);
      }
    }

    AABB2Di enclosingBox = oRooms[0].GetAABB();
    for (uint32_t i = 1; i < oRooms.size(); ++i)
    {
      enclosingBox.Absorb(oRooms[i].GetAABB());
    }

    AABB2Di fullSpace = enclosingBox;
    fullSpace.m_Data[0] -= One<Vec2i>() * 5;
    fullSpace.m_Data[1] += One<Vec2i>() * 5;

    AABB2DPolygoni::Merge(oRooms);

    oWalls.push_back(AABB2DPolygoni(fullSpace));
    Vector<AABB2DPolygoni> wallsTemp;

    for (auto& poly : oRooms)
    {
      for (auto& wall : oWalls)
      {
        Vector<AABB2DPolygoni> wallsOut;
        wall.Difference(poly, wallsOut);
        wallsTemp.insert(wallsTemp.end(), wallsOut.begin(), wallsOut.end());
      }
      AABB2DPolygoni::Merge(wallsTemp);
      oWalls = std::move(wallsTemp);
    }
  }

  void GraphRunner::LayoutGraph(World& iWorld, Random& iRand, ES_RuleSystem::Graph const& iGraph, Vector<AABB2DPolygoni>& oRooms, Vector<AABB2DPolygoni>& oWalls, LuaEventHandler const* iLayoutScript)
  {
    GameDatabase& db = *iWorld.GetSystem<GameDatabase>();
    auto layoutInfoView = db.GetView<RoomLayoutInfo>("RoomLayoutInfo");

    ES_RuleSystem::Graph layoutGraph;
    UnorderedMap<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx> graphMap;
    for (auto vtx : VerticesIter(iGraph))
    {
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, iGraph, vtx));
      ObjectHandle nodeObj = nodeData->m_Object;
      if (auto layoutInfo = layoutInfoView->Get(nodeObj))
      {
        if (!layoutInfo->m_CollapseNode && !layoutInfo->m_RoomSizes.empty())
        {
          auto iter = graphMap.insert(std::make_pair(vtx, boost::add_vertex(layoutGraph))).first;
          NodeLayoutData* layoutData = new NodeLayoutData;
          layoutData->m_PossibleRoomSize = layoutInfo->m_RoomSizes;
          layoutData->m_Ignore = false;
          boost::put(boost::vertex_name, layoutGraph, iter->second, layoutData);
          boost::put(boost::vertex_index, layoutGraph, iter->second, graphMap.size() - 1);
        }
      }
    }

    UnorderedSet<ES_RuleSystem::GraphVtx> collapsed;
    UnorderedSet<std::pair<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx>> handledPairs;
    for (auto vtx : VerticesIter(iGraph))
    {
      auto iter = graphMap.find(vtx);
      if (iter != graphMap.end())
      {
        for (auto edge : OutEdgesIter(iGraph, vtx))
        {
          ES_RuleSystem::GraphVtx target = GetTarget(vtx, edge);
          auto iter2 = graphMap.find(target);
          if (iter2 != graphMap.end())
          {
            MakeEdge(layoutGraph, graphMap, handledPairs, iter, iter2);
          }
        }
        continue;
      }

      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, iGraph, vtx));
      ObjectHandle nodeObj = nodeData->m_Object;
      if (auto layoutInfo = layoutInfoView->Get(nodeObj))
      {
        if (layoutInfo->m_CollapseNode)
        {
          SmallVector<ES_RuleSystem::GraphVtx, 2> vtxToConnect;
          FindVtxToConnect(*layoutInfoView, iGraph, vtx, vtxToConnect, collapsed);
          for (uint32_t i = 0; i < vtxToConnect.size(); ++i)
          {
            auto iter = graphMap.find(vtxToConnect[i]);
            if (iter != graphMap.end())
            {
              for (uint32_t j = i + 1; j < vtxToConnect.size(); ++j)
              {
                auto iter2 = graphMap.find(vtxToConnect[j]);
                if (iter2 != graphMap.end())
                {
                  MakeEdge(layoutGraph, graphMap, handledPairs, iter, iter2);
                }
              }
            }
          }
        }
      }
    }

    UnorderedMap<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx> revMap;
    for (auto const& entry : graphMap)
    {
      revMap.insert(std::make_pair(entry.second, entry.first));
    }

    LayoutCollection col = ::eXl::LayoutGraph(layoutGraph, iRand);

    if (!col.empty())
    {
      Name ppFun("LayoutPostProcess::ProcessRoom");

      ObjectHandle scriptObj = iWorld.CreateObject();
      EventSystem& evtSys = *iWorld.GetSystem<EventSystem>();
      LuaScriptSystem& luaSys = *iWorld.GetSystem<LuaScriptSystem>();

      if (iLayoutScript != nullptr)
      {
        luaSys.AddHandler(scriptObj, *iLayoutScript);
      }

      Layout lay = col[0];
      Vector<AABB2Di> boxes;
      UnorderedMap<ES_RuleSystem::GraphVtx, uint32_t> roomMap;
      for (auto const& room : lay)
      {
        roomMap.insert(std::make_pair(room.m_Node, roomMap.size()));
      }

      for (auto room : lay)
      {
        auto iter = revMap.find(room.m_Node);
        if (iter != revMap.end())
        {
          LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, iGraph, iter->second));
          ObjectHandle nodeObj = nodeData->m_Object;
          if (auto layoutInfo = layoutInfoView->Get(nodeObj))
          {
            Vector<AABB2Di> doors;
            for (auto edge : OutEdgesIter(layoutGraph, iter->first))
            {
              auto vtx1 = edge.m_source;
              auto vtx2 = edge.m_target;

              AABB2Di doorPlace;
              doorPlace.SetCommonBox(lay[roomMap[vtx1]].m_Box, lay[roomMap[vtx2]].m_Box);
              doors.push_back(doorPlace);
            }

            layoutInfo->m_Layout = room.m_Box;
            if (Optional<Vector<AABB2Di>> newBoxes = evtSys.Dispatch<Vector<AABB2Di>>(scriptObj, ppFun, nodeObj, doors))
            {
              boxes.insert(boxes.end(), newBoxes->begin(), newBoxes->end());
            }
          }
        }
      }

      FillTerrain(layoutGraph, boxes, oRooms, oWalls);
    }
  }
}

