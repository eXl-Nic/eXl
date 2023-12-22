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
#include <engine/map/graphrunner.hpp>

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
    Impl(GraphEditor* iEditor, GraphSimulateWidget* iWidget, RewriteSystemRsc& iSys);

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
    RewriteSystemRsc& m_SysRsc;
    RewriteSystem& m_Sys;

    ResourceSelectionWidget* m_LayoutScriptSelection;

    ES_RuleSystem::Graph m_CurGraph;
    Optional<GraphRunner::NodeData> m_NodeData;
    Optional<GraphRunner::EdgeData> m_EdgeData;
    bool m_Init = false;

    void UdpdateResultGraph();
    void SaveWorld(MapResource& oMap);
    void AddRule(RewriteSystem::SeqItem iItem);
  };

  GraphSimulateWidget::GraphSimulateWidget(GraphEditor* iEditor, RewriteSystemRsc& iSys)
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

  GraphSimulateWidget::Impl::Impl(GraphEditor* iEditor, GraphSimulateWidget* iWidget, RewriteSystemRsc& iSys)
    : QObject(iWidget)
    , m_Editor(iEditor)
    , m_Widget(iWidget)
    , m_SysRsc(iSys)
    , m_Sys(iSys.m_Sys)
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

    GraphRunner runner(m_SysRsc, world, *m_NodeData, *m_EdgeData);
    std::unique_ptr<Random> rand(Random::CreateDefaultRNG(0));
    
    Vector<GraphRunner::RuleItem> rules;
    for (int i = 0; i < m_Rules->count(); ++i) 
    {
      if (QListWidgetItem* item = m_Rules->item(i))
      {
        GraphRunner::RuleItem ruleItem;
        ruleItem.ruleName = item->text().toUtf8().data();
        int data = item->data(Qt::UserRole).toInt();
        ruleItem.application = (RuleApplication)data == RuleApplication::OneMatch ? 1 : 0;
        rules.push_back(ruleItem);
      }
    }

    runner.RunRules(*rand, rules, m_CurGraph);

    TGraphMap < ES_RuleSystem::Graph, boost::rectangle_topology<>::point_type> positionMap;
    TGraphMap < ES_RuleSystem::Graph, int> componentsMap;
    bool bIsConnected = boost::connected_components(m_CurGraph, componentsMap) == 1;

    boost::rectangle_topology<>::point_type defaultPos;
    defaultPos[0] = 0;
    defaultPos[1] = 0;

    GraphWrapper graphWrapper(world, m_CurGraph, runner.m_NodeData, runner.m_EdgeData);
    m_GraphPainter->nodes.resize(boost::num_vertices(m_CurGraph));
    m_GraphPainter->nodeDesc.resize(boost::num_vertices(m_CurGraph));
    for (auto vtx : VerticesIter(m_CurGraph))
    {
      ObjectHandle nodeObj = graphWrapper.GetNodeObject(vtx);
      LevelNodeData const* node = runner.m_NodeData.Get(nodeObj);
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
      Name edgeTag = runner.m_EdgeData.Get(edgeObj)->m_Tag;
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

    runner.m_NodeData.Iterate([&](ObjectHandle iObj, LevelNodeData const& iData)
      {
        auto pos = boost::get(positionMap, iData.m_Vtx);
        m_GraphPainter->nodes[boost::get(boost::vertex_index, m_CurGraph, iData.m_Vtx)] = QPointF(pos[0], pos[1]);

        auto iter = m_SysRsc.m_Tags.find(iData.m_Tag);
        if (iter != m_SysRsc.m_Tags.end()
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

namespace eXl
{

#if 0
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
#endif

  void GraphSimulateWidget::Impl::SaveWorld(MapResource& oMap)
  {
    World& world = m_World.GetWorld();
    GameDatabase& db = *world.GetSystem<GameDatabase>();
    
    UniquePtr<Random> rand(Random::CreateDefaultRNG(0));

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

    Resource::UUID scriptId = m_LayoutScriptSelection->GetSelectedResourceId();
    ResourceHandle<LuaEventHandler> layoutScript;
    layoutScript.SetUUID(scriptId);
    
    Vector<AABB2DPolygoni> oRooms;
    Vector<AABB2DPolygoni> oWalls;
    GraphRunner::LayoutGraph(world, *rand, m_CurGraph, oRooms, oWalls, layoutScript.GetOrLoad());

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