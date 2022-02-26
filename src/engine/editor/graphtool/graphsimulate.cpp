#include "graphsimulate.hpp"
#include "graphpainter.hpp"

#include <core/input.hpp>
#include <core/random.hpp>
#include <engine/common/app.hpp>
#include <engine/common/transforms.hpp>
#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/script/luascriptsystem.hpp>

#include <editor/editorstate.hpp>

#include <gen/graphutils.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/circle_layout.hpp>
#include <boost/graph/kamada_kawai_spring_layout.hpp>

#include <QListWidget>
#include <QSplitter>
#include <QBoxLayout>
#include <QToolBar>

namespace eXl
{
  struct GraphSimulateWidget::Impl
  {
    Impl(GraphSimulateWidget* iWidget, RewriteSystem& iSys);

    enum RuleApplication
    {
      OneMatch,
      AllMatches,
      AllSystem
    };

    WorldState m_World;
    InputSystem m_Inputs;

    QListWidget* m_Rules;

    GraphPainter* m_GraphPainter;
    Vector<ObjectHandle> m_DisplayNodes;

    String m_CurSelectedRule;

    GraphSimulateWidget* m_Widget;
    RewriteSystem& m_Sys;

    void UdpdateResultGraph();
  };

  GraphSimulateWidget::GraphSimulateWidget(QWidget* iParent, RewriteSystem& iSys)
    : QWidget(iParent)
    , m_Impl(std::make_unique<Impl>(this, iSys))
  {
    
  }

  GraphSimulateWidget::~GraphSimulateWidget() = default;

  void GraphSimulateWidget::SetSelectedRule(String const& iRule)
  {
    m_Impl->m_CurSelectedRule = iRule;
  }

  GraphSimulateWidget::Impl::Impl(GraphSimulateWidget* iWidget, RewriteSystem& iSys)
    : m_Widget(iWidget)
    , m_Sys(iSys)
  {
    m_World.Init(EditorState::GetProjectProperties()).WithGfx();

    World& world = m_World.GetWorld();
    GfxSystem& gfx = *world.GetSystem<GfxSystem>();

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, m_Widget);

    QWidget* rulesPanel = new QWidget(m_Widget);
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesPanel);
    rulesPanel->setLayout(rulesLayout);

    QToolBar* rulesTool = new QToolBar(rulesPanel);
    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_MediaPlay), "Add Single Match Rule", [this]
      {
        auto iterRule = m_Sys.m_Rules.find(m_CurSelectedRule);
        if (iterRule == m_Sys.m_Rules.end())
        {
          return;
        }
        int insertionPoint = m_Rules->count();
        if (m_Rules->selectedItems().size() > 0)
        {
          insertionPoint = m_Rules->row(m_Rules->selectedItems()[0]);
        }
        
        m_Rules->insertItem(insertionPoint, QString::fromUtf8(m_CurSelectedRule.c_str()));
        m_Rules->item(insertionPoint)->setData(Qt::UserRole, (int)OneMatch);
        UdpdateResultGraph();
      });

    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_MediaSeekForward), "Add Parallel Match Rule", [this]
      {
        auto iterRule = m_Sys.m_Rules.find(m_CurSelectedRule);
        if (iterRule == m_Sys.m_Rules.end())
        {
          return;
        }
        int insertionPoint = m_Rules->count();
        if (m_Rules->selectedItems().size() > 0)
        {
          insertionPoint = m_Rules->row(m_Rules->selectedItems()[0]);
        }

        m_Rules->insertItem(insertionPoint, QString::fromUtf8(m_CurSelectedRule.c_str()));
        m_Rules->item(insertionPoint)->setData(Qt::UserRole, (int)AllMatches);
        UdpdateResultGraph();
      });

    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Rule", [this]
      {
        if (m_Rules->selectedItems().size() > 0)
        {
          QListWidgetItem* removedItem = m_Rules->takeItem(m_Rules->row(m_Rules->selectedItems()[0]));
          delete removedItem;
          UdpdateResultGraph();
        }
      });

    rulesLayout->addWidget(rulesTool);
    m_Rules = new QListWidget(m_Widget);

    rulesLayout->addWidget(m_Rules);
    rootSplitter->addWidget(rulesPanel);

    {
      GameWidget* gameWidget = new GameWidget(m_Widget);
      gameWidget->SetInputSystem(&m_Inputs);
      gameWidget->SetGfxSystem(m_World.GetWorld().GetSystem<GfxSystem>());

      m_GraphPainter = new GraphPainter(gameWidget);
      gameWidget->SetPainterInterface(m_GraphPainter);

      GfxSystem::ViewInfo& view = gameWidget->GetViewInfo();
      view.pos = Vector3f::UNIT_Z * 2;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = GraphPainter::s_NodeSize * 10;
      view.backgroundColor = Vector4f::ONE;

      m_World.GetCamera().view = view;

      gameWidget->SetTickCallback([this, gameWidget](float iDelta)
        {
          World& world = m_World.GetWorld();
          GfxSystem& gfx = *world.GetSystem<GfxSystem>();

          m_World.GetCamera().ProcessInputs(m_World.GetWorld(), m_Inputs, CameraState::WheelZoom | CameraState::RightClickPan);
          gameWidget->GetViewInfo().pos = m_World.GetCamera().view.pos;
          gameWidget->GetViewInfo().basis[0] = m_World.GetCamera().view.basis[0];
          gameWidget->GetViewInfo().basis[1] = m_World.GetCamera().view.basis[1];
          gameWidget->GetViewInfo().basis[2] = m_World.GetCamera().view.basis[2];
          gameWidget->GetViewInfo().displayedSize = m_World.GetCamera().view.displayedSize;
          gameWidget->ViewInfoUpdated();

          m_Inputs.Clear();

          m_World.Tick();
        });

      gameWidget->SetAnimated(true);

      rootSplitter->addWidget(gameWidget);
    }

    rootSplitter->setSizes({ 1000, 6000 });

    QVBoxLayout* layout = new QVBoxLayout(m_Widget);
    layout->addWidget(rootSplitter);
    m_Widget->setLayout(layout);
  }

  class RuleData : public HeapObject
  {
    DECLARE_RefC;
  public:
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

  void GraphSimulateWidget::Impl::UdpdateResultGraph()
  {
    World& world = m_World.GetWorld();
    for (auto obj : m_DisplayNodes)
    {
      world.DeleteObject(obj);
    }
    m_DisplayNodes.clear();
    m_GraphPainter->Clear();

    LuaScriptSystem& luaSys = *world.GetSystem<LuaScriptSystem>();

    DenseGameDataStorage<LevelNodeData> nodeData(world);
    DenseGameDataStorage<LevelEdgeData> edgeData(world);

    ES_RuleSystem sys;
    Vector<String> rules;
    UnorderedMap<String, uint32_t> rulesIdx;

    Vector<Name> tags;
    UnorderedMap<Name, uint32_t> tagsIdx;
    tagsIdx.insert(std::make_pair(RewriteSystem::GetAnyTag(), UINT32_MAX));

    Vector<ObjectHandle> ruleObjects;

    for (auto const& tag : m_Sys.m_Tags)
    {
      tags.push_back(tag.first);
      tagsIdx.insert(std::make_pair(tag.first, tags.size() - 1));
    }

    for (auto const& ruleEntry : m_Sys.m_Rules)
    {
      IntrusivePtr<RuleData> data = MakeRefCounted<RuleData>();
      data->rewriteSys = &m_Sys;
      data->ruleObject = world.CreateObject();

      if (LuaScriptBehaviour const* script = ruleEntry.second.m_RewriteScript.GetOrLoad())
      {
        if (script->m_BehaviourName == "RewriteRule")
        {
          luaSys.AddBehaviour(data->ruleObject, *script);
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
          LuaScriptSystem& luaSys = *ctx.m_SourceGraph.m_World.GetSystem<LuaScriptSystem>();
          if (!luaSys.HasBehaviour(data->ruleObject, "RewriteRule"))
          {
            return true;
          }

          MatchWrapper wrapper(ctx.m_SourceGraph);
          Optional<bool> ret = luaSys.CallBehaviour<bool>(data->ruleObject, "RewriteRule", "CheckNode"
            , wrapper, iIdx, nodeObj);

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
          LuaScriptSystem& luaSys = *ctx.m_SourceGraph.m_World.GetSystem<LuaScriptSystem>();
          if (!luaSys.HasBehaviour(data->ruleObject, "RewriteRule"))
          {
            return true;
          }

          MatchWrapper wrapper(ctx.m_SourceGraph);
          Optional<bool> ret = luaSys.CallBehaviour<bool>(data->ruleObject, "RewriteRule", "CheckEdge"
            , wrapper, iIdx, edgeObj);

          eXl_ASSERT_MSG_REPAIR_RET(ret, "Invalid return type for CheckEdge function", false);

          return *ret;
        }

        return false;
      };

      auto checkMatch = [data](ES_RuleSystem::MatchCtx& iCtx, Vector<ES_RuleSystem::GraphVtx> const& iMatch)
      {
        SimMatchCtx const& ctx = *SimMatchCtx::DynamicCast(iCtx.userCtx);

        LuaScriptSystem& luaSys = *ctx.m_SourceGraph.m_World.GetSystem<LuaScriptSystem>();
        if (!luaSys.HasBehaviour(data->ruleObject, "RewriteRule"))
        {
          return true;
        }

        Vector<ObjectHandle> nodeObjects;

        for (auto vtx : iMatch)
        {
          nodeObjects.push_back(ctx.m_SourceGraph.GetNodeObject(vtx));
        }

        MatchWrapper wrapper(ctx.m_SourceGraph);
        Optional<bool> ret = luaSys.CallBehaviour<bool>(data->ruleObject, "RewriteRule", "CheckMatch"
          , wrapper, nodeObjects);

        eXl_ASSERT_MSG_REPAIR_RET(ret, "Invalid return type for CheckMatch function", false);

        return *ret;
      };

      auto createNode = [data](ES_RuleSystem::RewriteCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphVtx iVtx)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle nodeObj = ctx.m_DestGraph.AddNode(iVtx);
        Name tag = data->newNodeTags[iIdx];
        ctx.m_DestGraph.m_NodeData.Get(nodeObj)->m_Tag = tag;
        auto iter = data->rewriteSys->m_Tags.find(tag);
        if (iter != data->rewriteSys->m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          ctx.m_DestGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(nodeObj, arch, nullptr);
        }
      };

      auto createEdge = [data](ES_RuleSystem::RewriteCtx& iCtx, uint32_t iIdx, ES_RuleSystem::GraphEdge iEdge)
      {
        SimRewriteCtx& ctx = *SimRewriteCtx::DynamicCast(iCtx.userCtx);
        ObjectHandle edgeObj = ctx.m_DestGraph.AddEdge(iEdge);
        Name tag = data->newEdgeTags[iIdx];
        ctx.m_DestGraph.m_EdgeData.Get(edgeObj)->m_Tag = tag;
        auto iter = data->rewriteSys->m_Tags.find(tag);
        if (iter != data->rewriteSys->m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          ctx.m_DestGraph.m_World.GetSystem<GameDatabase>()->InstantiateArchetype(edgeObj, arch, nullptr);
        }
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

    ES_RuleSystem::Graph curGraph;


    UniquePtr<Random> rand(Random::CreateDefaultRNG(0));

    for (uint32_t i = 0; i < m_Rules->count(); ++i)
    {
      QListWidgetItem* item = m_Rules->item(i);
      String ruleName(item->text().toUtf8().data());
      RuleApplication application = (RuleApplication)item->data(Qt::UserRole).toInt();

      auto iter = rulesIdx.find(ruleName);
      if (iter != rulesIdx.end())
      {
        GraphWrapper srcGraphWrapper(world, curGraph, nodeData, edgeData);

        SimMatchCtx matchCtx(srcGraphWrapper);
        if (application == OneMatch)
        {
          auto matchings = sys.FindRuleMatch(iter->second, curGraph, &matchCtx);
          if (matchings.size() > 0)
          {
            uint32_t matchToConsider = rand->Generate() % matchings.size();

            ES_RuleSystem::Graph newGraph;
            GraphWrapper dstGraphWrapper(world, newGraph, nodeData, edgeData);

            SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
            sys.ApplyRule(curGraph, newGraph, iter->second, matchings[matchToConsider], &rewriteCtx);
            curGraph = newGraph;
          }
        }
        else if (application == AllMatches)
        {
          ES_RuleSystem::Graph newGraph;
          GraphWrapper dstGraphWrapper(world, newGraph, nodeData, edgeData);

          SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
          sys.ApplyRuleParallel(curGraph, newGraph, iter->second, &matchCtx, &rewriteCtx);
          curGraph = newGraph;
        }
        uint32_t nodeIdx = 0;
        for (auto vtx : VerticesIter(curGraph))
        {
          boost::put(boost::vertex_index, curGraph, vtx, nodeIdx++);
        }
      }
    }

    Vector<ES_RuleSystem::GraphVtx> nodes;
    Vector<Name> nodeTags;
    TGraphMap < ES_RuleSystem::Graph, boost::rectangle_topology<>::point_type> positionMap;
    TGraphMap < ES_RuleSystem::Graph, int> componentsMap;
    bool bIsConnected = boost::connected_components(curGraph, componentsMap) == 1;

    boost::rectangle_topology<>::point_type defaultPos;
    defaultPos[0] = 0;
    defaultPos[1] = 0;

    GraphWrapper graphWrapper(world, curGraph, nodeData, edgeData);

    for (auto vtx : VerticesIter(curGraph))
    {
      ObjectHandle nodeObj = graphWrapper.GetNodeObject(vtx);
      Name nodeTag = nodeData.Get(nodeObj)->m_Tag;
      nodeTags.push_back(nodeTag);
      m_GraphPainter->nodesColor.push_back(qRgb(0, 0, 255));
      nodes.push_back(vtx);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_GraphPainter->nodeDesc.push_back(QString::fromUtf8(nodeTag.c_str()));
    }

    for (auto edge : EdgesIter(curGraph))
    {
      ObjectHandle edgeObj = graphWrapper.GetEdgeObject(edge);
      Name edgeTag = edgeData.Get(edgeObj)->m_Tag;
      m_GraphPainter->edgesColor.push_back(qRgb(0, 0, 255));
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(edgeTag.c_str()));
    }

    float dist = Mathf::Max((nodes.size() + 2) / Mathf::Sqrt(2.0), 2) * GraphPainter::s_NodeSize;

    boost::rectangle_topology<> rectangle(-dist, -dist, dist, dist);
    if (1 || !bIsConnected)
    {
      struct my_square_distance_attractive_force {
        
        double operator()(ES_RuleSystem::GraphEdge,
            double k,
            double d,
            const ES_RuleSystem::Graph&) const
        {
          return d * d / k;
        }
      };
      struct my_square_distance_repulsive_force {

        double operator()(ES_RuleSystem::GraphVtx,
          ES_RuleSystem::GraphVtx,
          double k,
          double d,
          const ES_RuleSystem::Graph&) const
        {
          return k / d;
        }
      };
      boost::random_graph_layout(curGraph, MakeRef(positionMap), rectangle);
      boost::fruchterman_reingold_force_directed_layout(curGraph, MakeRef(positionMap), rectangle
        , boost::attractive_force(my_square_distance_attractive_force()).repulsive_force(my_square_distance_repulsive_force()));
    }
    else
    {
      boost::circle_graph_layout(curGraph, MakeRef(positionMap), dist);
      TEdgeGraphMap < ES_RuleSystem::Graph, double> weightMap;
      for (auto edge : EdgesIter(curGraph))
      {
        boost::put(weightMap, edge, 1.0);
      }
      // Infinite loop
      //boost::kamada_kawai_spring_layout(curGraph, MakeRef(positionMap), weightMap, rectangle, boost::side_length(dist * 2));
    }

    Transforms& trans = *world.GetSystem<Transforms>();
    GfxSystem& gfx = *world.GetSystem<GfxSystem>();
    GameDatabase& database = *world.GetSystem<GameDatabase>();

    GameDataView<GfxSpriteComponent::Desc> const* spriteDescView = GetSpriteComponentView(world);
    //for (uint32_t i = 0; i < nodes.size(); ++i)
    nodeData.Iterate([&](ObjectHandle iObj, LevelNodeData const& iData)
      {
        auto pos = boost::get(positionMap, iData.m_Vtx);
        m_GraphPainter->nodes.push_back(QPointF(pos[0], pos[1]));

        auto iter = m_Sys.m_Tags.find(iData.m_Tag);
        if (iter != m_Sys.m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
          {
            //ObjectHandle obj = world.CreateObject();
            database.InstantiateArchetype(iObj, arch, nullptr);
            trans.AddTransform(iObj, Matrix4f::FromPosition(Vector3f(pos[0], pos[1], 0.0)));
            gfx.CreateSpriteComponent(iObj);
            m_DisplayNodes.push_back(iObj);
          }
        }
      });

    for (auto edge : EdgesIter(curGraph))
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vector2d& pos1V = reinterpret_cast<Vector2d&>(pos1);
      Vector2d& pos2V = reinterpret_cast<Vector2d&>(pos2);
      Vector2d dir = pos2V - pos1V;
      dir.Normalize();
      pos2V -= dir * GraphPainter::s_NodeSize;
      pos1V += dir * GraphPainter::s_NodeSize;
      m_GraphPainter->edges.push_back(qMakePair(QPointF(pos1[0], pos1[1]), QPointF(pos2[0], pos2[1])));
    }
  }
}