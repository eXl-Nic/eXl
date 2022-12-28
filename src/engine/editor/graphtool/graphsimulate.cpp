#include "graphsimulate.hpp"
#include "graphpainter.hpp"
#include "grapheditor.hpp"

#include <core/input.hpp>
#include <core/random.hpp>
#include <engine/common/app.hpp>
#include <engine/common/transforms.hpp>
#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <engine/map/map.hpp>

#include <editor/editorstate.hpp>
#include <editor/resourceselectionwidget.hpp>

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
#include <QKeyEvent>

namespace eXl
{
  struct GraphSimulateWidget::Impl : public QObject
  {
    Impl(GraphEditor* iEditor, GraphSimulateWidget* iWidget, RewriteSystem& iSys);

    bool eventFilter(QObject* object, QEvent* event) override;

    WorldConfig m_Conf;
    WorldState m_World;
    InputSystem m_Inputs;

    QListWidget* m_Rules;

    GraphPainter* m_GraphPainter;
    Vector<ObjectHandle> m_DisplayNodes;

    String m_CurSelectedRule;

    GraphEditor* m_Editor;
    GraphSimulateWidget* m_Widget;
    RewriteSystem& m_Sys;

    ResourceSelectionWidget* m_LayoutScriptSelection;

    ES_RuleSystem::Graph m_CurGraph;
    bool m_Init = false;

    Optional<DenseGameDataStorage<LevelNodeData>> m_NodeData;
    Optional<DenseGameDataStorage<LevelEdgeData>> m_EdgeData;

    void UdpdateResultGraph();
    void SaveWorld(MapResource& oMap);
    void AddRule(RewriteSystem::SeqItem iItem);
  };

  GraphSimulateWidget::GraphSimulateWidget(GraphEditor* iEditor, RewriteSystem& iSys)
    : QWidget(iEditor)
    , m_Impl(std::make_unique<Impl>(iEditor, this, iSys))
  {

  }

  GraphSimulateWidget::~GraphSimulateWidget() = default;

  void GraphSimulateWidget::SetSelectedRule(String const& iRule)
  {
    m_Impl->m_CurSelectedRule = iRule;
  }

  bool GraphSimulateWidget::Impl::eventFilter(QObject* iObject, QEvent* iEvent)
  {
    if (iObject == m_Rules && iEvent->type() == QEvent::KeyPress)
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(iEvent);
      if (ke->key() == Qt::Key_Escape)
      {
        m_Rules->clearSelection();
        return true;
      }
    }
    return false;
  }

  void GraphSimulateWidget::Impl::AddRule(RewriteSystem::SeqItem iItem)
  {
    auto iterRule = m_Sys.m_Rules.find(iItem.m_Rule);
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
    m_Rules->item(insertionPoint)->setData(Qt::UserRole, (int)iItem.m_Appl);
    m_Rules->item(insertionPoint)->setData(Qt::CheckStateRole, iItem.m_Appl == RuleApplication::AllMatch);
    m_Sys.m_CurSequence.insert(m_Sys.m_CurSequence.begin() + insertionPoint, iItem);
    m_Editor->GetDocument()->Touch();
    UdpdateResultGraph();
  };

  GraphSimulateWidget::Impl::Impl(GraphEditor* iEditor, GraphSimulateWidget* iWidget, RewriteSystem& iSys)
    : QObject(iWidget)
    , m_Editor(iEditor)
    , m_Widget(iWidget)
    , m_Sys(iSys)
    , m_Conf(EditorState::BuildWorldConfig())
  {
    m_World.Init(m_Conf).WithGfx();

    World& world = m_World.GetWorld();
    m_NodeData.emplace(world);
    m_EdgeData.emplace(world);

    GfxSystem& gfx = *world.GetSystem<GfxSystem>();

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, m_Widget);

    QWidget* rulesPanel = new QWidget(m_Widget);
    QVBoxLayout* rulesLayout = new QVBoxLayout(rulesPanel);
    rulesPanel->setLayout(rulesLayout);

    m_LayoutScriptSelection = new ResourceSelectionWidget(m_Editor, LuaEventHandler::StaticLoaderName(), ResourceSelectionWidget::Combo);
    rulesLayout->addWidget(m_LayoutScriptSelection);

    QToolBar* rulesTool = new QToolBar(rulesPanel);
    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_MediaPlay), "Add Single Match Rule", [this]
      {
        RewriteSystem::SeqItem item;
        item.m_Rule = m_CurSelectedRule;
        item.m_Appl = RuleApplication::OneMatch;
        AddRule(item);
      });

    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_MediaSeekForward), "Add Parallel Match Rule", [this]
      {
        RewriteSystem::SeqItem item;
        item.m_Rule = m_CurSelectedRule;
        item.m_Appl = RuleApplication::AllMatch;
        AddRule(item);
      });

    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Rule", [this]
      {
        if (m_Rules->selectedItems().size() > 0)
        {
          int pos = m_Rules->row(m_Rules->selectedItems()[0]);
          QListWidgetItem* removedItem = m_Rules->takeItem(pos);
          m_Sys.m_CurSequence.erase(m_Sys.m_CurSequence.begin() + pos);
          delete removedItem;
          m_Editor->GetDocument()->Touch();
          UdpdateResultGraph();
        }
      });

    rulesTool->addAction(m_Widget->style()->standardIcon(QStyle::SP_DialogSaveButton), "Save Map", [this]
      {
        DocumentState* doc = EditorState::OpenSaveResource(MapResource::StaticLoaderName());
        if (doc != nullptr)
        {
          MapResource* map = MapResource::DynamicCast(doc->GetResource());
          if (map)
          {
            map->m_Objects.clear();
            SaveWorld(*map);
            doc->Save();
          }
        }
      });

    rulesLayout->addWidget(rulesTool);
    m_Rules = new QListWidget(m_Widget);
    for (auto& item : m_Sys.m_CurSequence) 
    {
      int insertionPoint = m_Rules->count();
      m_Rules->addItem(QString::fromUtf8(item.m_Rule.c_str()));
      m_Rules->item(insertionPoint)->setData(Qt::UserRole, (int)item.m_Appl);
      m_Rules->item(insertionPoint)->setData(Qt::CheckStateRole, item.m_Appl == RuleApplication::AllMatch);
    }
    m_Rules->installEventFilter(this);

    rulesLayout->addWidget(m_Rules);
    rootSplitter->addWidget(rulesPanel);

    {
      GameWidget* gameWidget = new GameWidget(m_Widget);
      gameWidget->SetInputSystem(&m_Inputs);
      gameWidget->SetGfxSystem(m_World.GetWorld().GetSystem<GfxSystem>());

      m_GraphPainter = new GraphPainter(gameWidget);
      gameWidget->SetPainterInterface(m_GraphPainter);

      GfxSystem::ViewInfo& view = gameWidget->GetViewInfo();
      view.pos = UnitZ<Vec3>() * 2;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = GraphPainter::s_NodeSize * 10;
      view.backgroundColor = One<Vec4>();

      m_World.GetCamera().view = view;

      gameWidget->SetTickCallback([this, gameWidget](float iDelta)
        {
          World& world = m_World.GetWorld();
          GfxSystem& gfx = *world.GetSystem<GfxSystem>();

          if (!m_Init )
          {
            UdpdateResultGraph();
            m_Init = true;
          }

          m_World.GetCamera().ProcessInputs(m_World.GetWorld(), m_Inputs, CameraState::WheelZoom | CameraState::RightClickPan);
          m_World.GetCamera().UpdateView(world);
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
    luaSys.Reload();

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
      ruleObjects.push_back(data->ruleObject);

      if (LuaEventHandler const* script = ruleEntry.second.m_RewriteScript.GetOrLoad())
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
        auto iter = data->rewriteSys->m_Tags.find(tag);
        if (iter != data->rewriteSys->m_Tags.end()
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
        auto iter = data->rewriteSys->m_Tags.find(tag);
        if (iter != data->rewriteSys->m_Tags.end()
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

    m_CurGraph.clear();
    m_NodeData->Clear();
    m_EdgeData->Clear();
    UniquePtr<Random> rand(Random::CreateDefaultRNG(0));

    Vector<ObjectHandle> oldNodes;

    for (uint32_t i = 0; i < m_Rules->count(); ++i)
    {
      oldNodes.clear();

      for (auto vtx : VerticesIter(m_CurGraph))
      {
        LevelNodeData const* data = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_CurGraph, vtx));
        oldNodes.push_back(data->m_Object);
      }
      for (auto edge : EdgesIter(m_CurGraph))
      {
        LevelEdgeData const* data = LevelEdgeData::DynamicCast(boost::get(boost::edge_name, m_CurGraph, edge));
        oldNodes.push_back(data->m_Object);
      }

      QListWidgetItem* item = m_Rules->item(i);
      String ruleName(item->text().toUtf8().data());
      RuleApplication application = (RuleApplication)item->data(Qt::UserRole).toInt();

      auto iter = rulesIdx.find(ruleName);
      if (iter != rulesIdx.end())
      {
        GraphWrapper srcGraphWrapper(world, m_CurGraph, *m_NodeData, *m_EdgeData);

        SimMatchCtx matchCtx(srcGraphWrapper);
        if (application == RuleApplication::OneMatch)
        {
          auto matchings = sys.FindRuleMatch(iter->second, m_CurGraph, &matchCtx);
          if (matchings.size() > 0)
          {
            uint32_t matchToConsider = rand->Generate() % matchings.size();

            ES_RuleSystem::Graph newGraph;
            GraphWrapper dstGraphWrapper(world, newGraph, *m_NodeData, *m_EdgeData);

            SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
            sys.ApplyRule(m_CurGraph, newGraph, iter->second, matchings[matchToConsider], &rewriteCtx);
            static Name const postRewrite("RewriteRule::PostRewrite");
            EventSystem& evtSys = *rewriteCtx.m_DestGraph.m_World.GetSystem<EventSystem>();
            ObjectHandle ruleObj = ruleObjects[iter->second];
            if (evtSys.GetEventHandlerInternal(ruleObj, postRewrite) != nullptr)
            {
              RewriteWrapper rwWrapper(srcGraphWrapper, dstGraphWrapper, matchings[matchToConsider]);
              GraphFactoryWrapper factory(dstGraphWrapper, m_Sys);
              evtSys.Dispatch<void>(ruleObj, postRewrite, rwWrapper, factory);
            }
            m_CurGraph = newGraph;
            for (auto vtx : VerticesIter(m_CurGraph))
            {
              ES_RuleSystem::NodeData const* data = boost::get(boost::vertex_name, m_CurGraph, vtx);
              data->CopyNode(vtx);
            }
            for (auto edge : EdgesIter(m_CurGraph))
            {
              ES_RuleSystem::EdgeData const* data = boost::get(boost::edge_name, m_CurGraph, edge);
              data->CopyEdge(edge);
            }
          }
        }
        else if (application == RuleApplication::AllMatch)
        {
          ES_RuleSystem::Graph newGraph;
          GraphWrapper dstGraphWrapper(world, newGraph, *m_NodeData, *m_EdgeData);

          SimRewriteCtx rewriteCtx(srcGraphWrapper, dstGraphWrapper);
          sys.ApplyRuleParallel(m_CurGraph, newGraph, iter->second, &matchCtx, &rewriteCtx);
          m_CurGraph = newGraph;
          for (auto vtx : VerticesIter(m_CurGraph))
          {
            ES_RuleSystem::NodeData const* data = boost::get(boost::vertex_name, m_CurGraph, vtx);
            data->CopyNode(vtx);
          }
          for (auto edge : EdgesIter(m_CurGraph))
          {
            ES_RuleSystem::EdgeData const* data = boost::get(boost::edge_name, m_CurGraph, edge);
            data->CopyEdge(edge);
          }
        }
        uint32_t nodeIdx = 0;
        for (auto vtx : VerticesIter(m_CurGraph))
        {
          boost::put(boost::vertex_index, m_CurGraph, vtx, nodeIdx++);
        }
      }
    }

    TGraphMap < ES_RuleSystem::Graph, boost::rectangle_topology<>::point_type> positionMap;
    TGraphMap < ES_RuleSystem::Graph, int> componentsMap;
    bool bIsConnected = boost::connected_components(m_CurGraph, componentsMap) == 1;

    boost::rectangle_topology<>::point_type defaultPos;
    defaultPos[0] = 0;
    defaultPos[1] = 0;

    GraphWrapper graphWrapper(world, m_CurGraph, *m_NodeData, *m_EdgeData);
    m_GraphPainter->nodes.resize(boost::num_vertices(m_CurGraph));
    m_GraphPainter->nodeDesc.resize(boost::num_vertices(m_CurGraph));
    for (auto vtx : VerticesIter(m_CurGraph))
    {
      ObjectHandle nodeObj = graphWrapper.GetNodeObject(vtx);
      LevelNodeData const* node = m_NodeData->Get(nodeObj);
      Name nodeTag = node->m_Tag;
      m_GraphPainter->nodesColor.push_back(qRgb(0, 0, 255));
      boost::put(positionMap, vtx, defaultPos);
      if (node->m_DebugString.empty())
      {
        m_GraphPainter->nodeDesc[boost::get(boost::vertex_index, m_CurGraph, vtx)] = QString::fromUtf8(nodeTag.c_str());
      }
      else
      {
        m_GraphPainter->nodeDesc[boost::get(boost::vertex_index, m_CurGraph, vtx)] = QString::fromUtf8((String(nodeTag.get()) + " : " + node->m_DebugString).c_str());
      }

    }

    for (auto edge : EdgesIter(m_CurGraph))
    {
      ObjectHandle edgeObj = graphWrapper.GetEdgeObject(edge);
      Name edgeTag = m_EdgeData->Get(edgeObj)->m_Tag;
      m_GraphPainter->edgesColor.push_back(qRgb(0, 0, 255));
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(edgeTag.c_str()));
    }

    float dist = Mathf::Max(Mathf::Sqrt(boost::num_vertices(m_CurGraph) + 2), 2) * 8 * GraphPainter::s_NodeSize;

    boost::rectangle_topology<> rectangle(-dist, -dist, dist, dist);
    if (!bIsConnected)
    {
      struct my_square_distance_attractive_force {

        double operator()(ES_RuleSystem::GraphEdge,
          double k,
          double d,
          const ES_RuleSystem::Graph&) const
        {
          return d * d / k + 1;
          //return d * d / k;
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
          //return k / d;
        }
      };
      boost::random_graph_layout(m_CurGraph, MakeRef(positionMap), rectangle);
      boost::fruchterman_reingold_force_directed_layout(m_CurGraph, MakeRef(positionMap), rectangle
        , boost::attractive_force(my_square_distance_attractive_force()).repulsive_force(my_square_distance_repulsive_force()));
    }
    else
    {
      boost::circle_graph_layout(m_CurGraph, MakeRef(positionMap), dist);
      TEdgeGraphMap < ES_RuleSystem::Graph, double> weightMap;
      for (auto edge : EdgesIter(m_CurGraph))
      {
        boost::put(weightMap, edge, 1.0);
      }
      for( int i = 0; i<10 ; ++i)
      {
        boost::kamada_kawai_spring_layout(m_CurGraph, MakeRef(positionMap), weightMap, rectangle, boost::side_length(dist));
      }
      
    }

    Transforms& trans = *world.GetSystem<Transforms>();
    GfxSystem& gfx = *world.GetSystem<GfxSystem>();
    GameDatabase& database = *world.GetSystem<GameDatabase>();

    GameDataView<GfxSpriteComponent::Desc> const* spriteDescView = GetSpriteComponentView(world);

    m_NodeData->Iterate([&](ObjectHandle iObj, LevelNodeData const& iData)
      {
        auto pos = boost::get(positionMap, iData.m_Vtx);
        m_GraphPainter->nodes[boost::get(boost::vertex_index, m_CurGraph, iData.m_Vtx)] = QPointF(pos[0], pos[1]);

        auto iter = m_Sys.m_Tags.find(iData.m_Tag);
        if (iter != m_Sys.m_Tags.end()
          && iter->second.m_Archetype.GetUUID().IsValid())
        {
          Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
          if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
          {
            trans.AddTransform(iObj, translate(Identity<Mat4>(), Vec3(pos[0], pos[1], 0.0)));
            gfx.CreateSpriteComponent(iObj);
            m_DisplayNodes.push_back(iObj);
          }
        }
      });

    for (auto edge : EdgesIter(m_CurGraph))
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vec2d& pos1V = reinterpret_cast<Vec2d&>(pos1);
      Vec2d& pos2V = reinterpret_cast<Vec2d&>(pos2);
      Vec2d dir = normalize(pos2V - pos1V);
      pos2V -= dir * double(GraphPainter::s_NodeSize);
      pos1V += dir * double(GraphPainter::s_NodeSize);
      m_GraphPainter->edges.push_back(qMakePair(QPointF(pos1[0], pos1[1]), QPointF(pos2[0], pos2[1])));
    }
  }
}

#include <engine/map/dungeonlayout.hpp>

namespace eXl
{

  void FindVtxToConnect(GameDataView<RoomLayoutInfo>& iLayoutData,
    ES_RuleSystem::Graph const & iGraph, 
    ES_RuleSystem::GraphVtx iCollapseVtx, 
    SmallVector<ES_RuleSystem::GraphVtx, 2>& oVtxToConnect, 
    UnorderedSet<ES_RuleSystem::GraphVtx>& oCollapsed)
  {
    if(oCollapsed.count(iCollapseVtx) != 0)
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
        else if(!layoutInfo->m_CollapseNode && !layoutInfo->m_RoomSizes.empty())
        {
          auto iter = std::find(oVtxToConnect.begin(), oVtxToConnect.end(), iCollapseVtx);
          if( iter == oVtxToConnect.end())
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

  void FillTerrain_Old(ES_RuleSystem::Graph const& iGraph, 
    Layout const& iRooms, 
    Vector<AABB2DPolygoni>& oRooms,
    Vector<AABB2DPolygoni>& oWalls)
  {
    Vector<AABB2Di> boxes;
    {
      uint32_t scaleFactor = 2;
      Map<ES_RuleSystem::GraphVtx, uint32_t> roomMap;
      for (auto const& room : iRooms)
      {
        if (iRooms.size() != 1)
        {
          AABB2Di box;
          box.m_Data[0] = (room.m_Box.m_Data[0] * scaleFactor) + One<Vec2i>() * scaleFactor;
          box.m_Data[1] = (room.m_Box.m_Data[1] * scaleFactor) - One<Vec2i>() * scaleFactor;
          roomMap.insert(std::make_pair(room.m_Node, oRooms.size()));
          oRooms.push_back(AABB2DPolygoni(box));
          boxes.push_back(box);
        }
        else
        {
          oRooms.push_back(room.m_Box);
          oRooms[0].Scale(scaleFactor);
          oRooms[0].GetBoxes(boxes);

          roomMap.insert(std::make_pair(room.m_Node, oRooms.size()));
        }
      }

      for (auto edge = boost::edges(iGraph); edge.first != edge.second; ++edge.first)
      {
        auto vtx1 = edge.first->m_source;
        auto vtx2 = edge.first->m_target;

        AABB2Di doorPlace;
        doorPlace.SetCommonBox(iRooms[roomMap[vtx1]].m_Box, iRooms[roomMap[vtx2]].m_Box);
        doorPlace.m_Data[0] *= scaleFactor;
        doorPlace.m_Data[1] *= scaleFactor;

        int32_t doorDir = doorPlace.m_Data[0].x == doorPlace.m_Data[1].x ? 0 : 1;

        Vec2i doorOrig = doorPlace.GetCenter() - One<Vec2i>() * int(scaleFactor / 2);
        Vec2i doorSize = One<Vec2i>() * scaleFactor;

        doorOrig[doorDir] -= scaleFactor / 2;
        doorSize[doorDir] += scaleFactor;

        doorPlace = AABB2Di::FromMinAndSize(doorOrig, doorSize);

        oRooms.push_back(AABB2DPolygoni(doorPlace));
        boxes.push_back(doorPlace);
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
  }

  void GraphSimulateWidget::Impl::SaveWorld(MapResource& oMap)
  {
    World& world = m_World.GetWorld();
    GameDatabase& db = *world.GetSystem<GameDatabase>();

    auto layoutInfoView = db.GetView<RoomLayoutInfo>();

    ES_RuleSystem::Graph layoutGraph;
    UnorderedMap<ES_RuleSystem::GraphVtx, ES_RuleSystem::GraphVtx> graphMap;
    for (auto vtx : VerticesIter(m_CurGraph)) 
    {
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_CurGraph, vtx));
      ObjectHandle nodeObj = nodeData->m_Object;
      if (auto layoutInfo = layoutInfoView->Get(nodeObj)) 
      {
        if(!layoutInfo->m_CollapseNode && !layoutInfo->m_RoomSizes.empty())
        {
          auto iter = graphMap.insert(std::make_pair( vtx, boost::add_vertex(layoutGraph))).first;
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
    for (auto vtx : VerticesIter(m_CurGraph))
    {
      auto iter = graphMap.find(vtx);
      if (iter != graphMap.end())
      {
        for (auto edge : OutEdgesIter(m_CurGraph, vtx))
        {
          ES_RuleSystem::GraphVtx target = GetTarget(vtx, edge);
          auto iter2 = graphMap.find(target);
          if(iter2 != graphMap.end())
          {
            MakeEdge(layoutGraph, graphMap, handledPairs, iter, iter2);
          }
        }
        continue;
      }
      
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_CurGraph, vtx));
      ObjectHandle nodeObj = nodeData->m_Object;
      if (auto layoutInfo = layoutInfoView->Get(nodeObj))
      {
        if(layoutInfo->m_CollapseNode)
        {
          SmallVector<ES_RuleSystem::GraphVtx, 2> vtxToConnect;
          FindVtxToConnect(*layoutInfoView, m_CurGraph, vtx, vtxToConnect, collapsed);
          for(uint32_t i = 0; i<vtxToConnect.size(); ++i)
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

    UniquePtr<Random> rand(Random::CreateDefaultRNG(0));
    LayoutCollection col = LayoutGraph(layoutGraph, *rand);

    int terrainFloorIdx = -1;
    int terrainWallIdx = -1;
    for (uint32_t i = 0; i < oMap.m_Terrains.size(); ++i) 
    {
      if (oMap.m_Terrains[i].m_Type == "Floor") 
      {
        terrainFloorIdx = i;
      }
      if (oMap.m_Terrains[i].m_Type == "Wall") 
      {
        terrainWallIdx = i;
      }
    }

    if (terrainFloorIdx == -1) 
    {
      terrainFloorIdx = oMap.m_Terrains.size();
      oMap.m_Terrains.push_back(MapResource::Terrain());
      oMap.m_Terrains.back().m_Type == "Floor";
    }
    if (terrainWallIdx == -1)
    {
      terrainWallIdx = oMap.m_Terrains.size();
      oMap.m_Terrains.push_back(MapResource::Terrain());
      oMap.m_Terrains.back().m_Type == "Wall";
    }

    MapResource::Terrain& terrainRoom = oMap.m_Terrains[terrainFloorIdx];
    MapResource::Terrain& terrainWall = oMap.m_Terrains[terrainWallIdx];
    terrainRoom.m_Blocks.clear();
    terrainWall.m_Blocks.clear();
    if (!col.empty())
    {
      Name ppFun("LayoutPostProcess::ProcessRoom");

      ObjectHandle scriptObj = world.CreateObject();
      EventSystem& evtSys = *world.GetSystem<EventSystem>();
      LuaScriptSystem& luaSys = *world.GetSystem<LuaScriptSystem>();
      Resource::UUID id = m_LayoutScriptSelection->GetSelectedResourceId();
      Resource const * rsc = ResourceManager::LoadExpectedType(id, LuaEventHandler::StaticLoaderName());
      if( rsc != nullptr)
      {
        luaSys.AddHandler(scriptObj, *static_cast<LuaEventHandler const *>(rsc));
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
          LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_CurGraph, iter->second));
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
            if(Optional<Vector<AABB2Di>> newBoxes = evtSys.Dispatch<Vector<AABB2Di>>(scriptObj, ppFun, nodeObj, doors))
            {
              boxes.insert(boxes.end(), newBoxes->begin(), newBoxes->end());
            }
          }
        }
      }

      Vector<AABB2DPolygoni> oRooms;
      Vector<AABB2DPolygoni> oWalls;
      FillTerrain(layoutGraph, boxes, oRooms, oWalls);

      terrainRoom.m_Blocks.resize(oRooms.size());
      for (uint32_t i = 0; i < oRooms.size(); ++i)
      {
        terrainRoom.m_Blocks[i].m_Shape = oRooms[i];
        terrainRoom.m_Blocks[i].m_Layer = 0;
      }
      terrainWall.m_Blocks.resize(oWalls.size());
      for (uint32_t i = 0; i < oWalls.size(); ++i)
      {
        terrainWall.m_Blocks[i].m_Shape = oWalls[i];
        terrainWall.m_Blocks[i].m_Layer = 0;
      }
    }

    for (auto vtx : VerticesIter(m_CurGraph))
    {
      LevelNodeData const* nodeData = LevelNodeData::DynamicCast(boost::get(boost::vertex_name, m_CurGraph, vtx));
      ObjectHandle nodeObj = nodeData->m_Object;
      ObjectInfo const& info = world.GetObjectInfo(nodeObj);

      if (info.m_Archetype == nullptr || info.m_PendingDeletion) 
      {
        return;
      }

      MapResource::Object objDesc;
      objDesc.m_Header.m_Position = Zero<Vec3>();
      objDesc.m_Header.m_ObjectId = MapResource::ObjectHeader::AllocObjectID();
      objDesc.m_Header.m_Archetype.Set(info.m_Archetype);

      const GameDatabase::ArchetypeData* archData = db.TryGetArchetypeData(*info.m_Archetype);
      eXl_ASSERT_REPAIR_RET(archData != nullptr, void());

      for (auto const& entry : archData->m_Data)
      {
        ObjectTableHandle_Base currentDataHandle = db.GetDataHandle(nodeObj, entry.first);
        if (currentDataHandle.IsAssigned())
        {
          if (entry.second.handle != currentDataHandle)
          {
            CustomizationData::FieldsMap propCustom;
            Type const* baseType = world.GetConfig().m_Properties.GetTypeFromName(entry.first);
            TupleType const* propType = baseType != nullptr ? baseType->IsTuple() : nullptr;
            for (uint32_t i = 0; i < propType->GetNumField(); ++i)
            {
              const ConstDynObject& archetypeData = info.m_Archetype->GetProperty(entry.first);
              ConstDynObject objData = db.GetData(nodeObj, entry.first);

              eXl_ASSERT_REPAIR_BEGIN(archetypeData.IsValid() && objData.IsValid())
              {
                continue;
              }

              Type const* fieldType;
              void const* archFieldData = propType->GetField(archetypeData.GetBuffer(), i, fieldType);
              void const* objFieldData = propType->GetField(objData.GetBuffer(), i, fieldType);

              CompRes comp = CompDifferent;
              Err res = fieldType->Compare(archData, objFieldData, comp);
              if (!res || comp != CompEqual)
              {
                TypeFieldName fieldName;
                propType->GetFieldDetails(i, fieldName);
                ConstDynObject objField(fieldType, objFieldData);
                propCustom.insert(std::make_pair(fieldName, DynObject(&objField)));
              }
            }
            if (!propCustom.empty())
            {
              objDesc.m_Data.m_PropertyCustomization.insert(std::make_pair(entry.first, std::move(propCustom)));
            }
          }
        }
        else
        {
          //??
        }
      }

      //objDesc.m_Header.m_Position
      oMap.m_Objects.push_back(std::move(objDesc));
    }
  }
}