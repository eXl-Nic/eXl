#include "mcmclearntool.hpp"

#include "mapeditor.hpp"
#include "utils.hpp"

#include "mapitemdata.hpp"

#include "tilestool.hpp"
#include "terraintool.hpp"
#include "objectstool.hpp"

#include "../editordef.hpp"
#include "../eXl_Editor/resourcehandle_editor.h"

#include <core/random.hpp>
#include <core/resource/resourcemanager.hpp>
#include <math/mathtools.hpp>

#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <gen/mcmcsvmmodel.hpp>
#include <gen/mcmcdiskmodel.hpp>

#include <core/stream/jsonstreamer.hpp>
#include <fstream>

#include <QListWidget>
#include <QBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

namespace eXl
{
  const uint32_t s_DefaultDimScale = 1;
  const uint32_t s_DefaultShapeShrink = 2;
  const float s_DefaultInteractionRadius = 64.0;

  MCMCLearnTool::~MCMCLearnTool() = default;

  MCMCLearnTool::MCMCLearnTool(QWidget* iParent, World& iWorld, MapEditor* iEditor)
    : EditorTool(iParent, iWorld)
    , m_World(iWorld)
    , m_MCMCData(iWorld)
    , m_ParentEditor(iEditor)
  {
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_ElementsDisplay = new QListWidget(this);
    QObject::connect(m_ElementsDisplay->model(), &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex)
    {
        QListWidgetItem* item = m_ElementsDisplay->item(iIndex.row());
        if (item == nullptr)
        {
          return;
        }
        eXl_ASSERT_REPAIR_RET(iIndex.row() < m_Enabled.size(), );
        m_Enabled[iIndex.row()] = item->checkState() == Qt::Checked;
    });
    layout->addWidget(m_ElementsDisplay);

    m_ShapeScale = new QSpinBox(this);
    m_ShapeScale->setMinimum(1);
    m_ShapeScale->setMaximum(1000);
    m_ShapeScale->setValue(s_DefaultDimScale);
    m_ShapeShrink = new QSpinBox(this);
    m_ShapeShrink->setMinimum(0);
    m_ShapeShrink->setMaximum(1000);
    m_ShapeShrink->setValue(s_DefaultShapeShrink);
    m_InteractionRadius = new QSpinBox(this);
    m_InteractionRadius->setMinimum(1);
    m_InteractionRadius->setMaximum(Mathi::MAX_REAL);
    m_InteractionRadius->setValue(s_DefaultInteractionRadius);
    m_UseQuantileCull = new QCheckBox(this);

    auto makeRow = [this, layout](QWidget* iInputWidget, char const* iLabel)
    {
      QWidget* row = new QWidget(this);
      QHBoxLayout* rowLayout = new QHBoxLayout(row);

      rowLayout->addWidget(new QLabel(QString::fromUtf8(iLabel), row));
      rowLayout->addWidget(iInputWidget);
      row->setLayout(rowLayout);
      layout->addWidget(row);
    };

    makeRow(m_ShapeScale, "Shape Scale :");
    makeRow(m_ShapeShrink, "Shape Shrink :");
    makeRow(m_InteractionRadius, "Interaction Radius :");
    makeRow(m_UseQuantileCull, "Use Quantile Cull : ");

    m_CurModelSelector = new ResourceHandle_Editor(this);

    ResourceHandle<MCMCModelRsc> handle;
    ConstDynObject obj(TypeManager::GetType<ResourceHandle<MCMCModelRsc>>(), &handle);
    m_CurModelSelector->SetObject(&obj);
    layout->addWidget(m_CurModelSelector);

    QPushButton* learnButton = new QPushButton("Learn", this);
    QObject::connect(learnButton, &QPushButton::pressed, [this]
      {
        TrainModel();
      });

    layout->addWidget(learnButton);

    m_RunSpaceList = new QListWidget(this);
    layout->addWidget(m_RunSpaceList);

    m_RunIter = new QSpinBox(this);
    m_RunIter->setMinimum(1);
    m_RunIter->setMaximum(Mathi::MAX_REAL);
    m_RunIter->setValue(1000);
    makeRow(m_RunIter, "Run Iteration # :");
    
    m_CleanRun = new QCheckBox(this);
    m_CleanRun->setChecked(false);
    makeRow(m_CleanRun, "Clean run :");

    QPushButton* runButton = new QPushButton("Run", this);
    QObject::connect(runButton, &QPushButton::pressed, [this]
      {
        RunModel();
      });
    layout->addWidget(runButton);

    setLayout(layout);
  }

  struct TempElementInfo
  {
    Set<uint32_t> layers;
    Vector2i size;
    bool placeable;
    QPixmap thumbnail;
    String desc;
  };

  static const TerrainTypeName GetWallTypeName()
  {
    static TerrainTypeName s_WallTypeName("Wall");
    return s_WallTypeName;
  }

  void MCMCLearnTool::EnableTool()
  {
    m_RunSpaceList->clear();
    m_RunSpaces.clear();
    m_RunSpacesHandle.clear();

    ReadElements();

    GameDatabase* database = m_World.GetSystem<GameDatabase>();
    if (auto const* terrain = database->GetView<TerrainIslandItemData>(TerrainTool::ToolDataName()))
    {
      terrain->Iterate([&](ObjectHandle iHandle, TerrainIslandItemData const& iIsland)
        {
          //TempElementInfo info;
          //TilingGroup const* group = iIsland.m_TilingGroup.GetOrLoad();
          //if (group)
          //{
          //  GetTileInfo(group->GetTileset(), group->m_DefaultTile, info);
          //}

          if (iIsland.m_Terrain == GetWallTypeName())
          {
            for (auto const& hole : iIsland.m_IslandPoly.Holes())
            {
              Polygoni runSpace(hole);
              //runSpace.Scale(info.size.X());
              m_RunSpaces.push_back(std::move(runSpace));
              m_RunSpacesHandle.push_back(iHandle);
            }
          };
        });
    }

    for (auto const& space : m_RunSpaces)
    {
      AABB2Di box = space.GetAABB();

      String descStr("Box at : (");
      descStr += StringUtil::FromInt(box.m_Data[0].X());
      descStr += ", ";
      descStr += StringUtil::FromInt(box.m_Data[0].Y());
      descStr += ") -> (";
      descStr += StringUtil::FromInt(box.m_Data[1].X());
      descStr += ", ";
      descStr += StringUtil::FromInt(box.m_Data[1].Y());
      descStr += ")";

      m_RunSpaceList->addItem(descStr.c_str());
    }

  }

  void MCMCLearnTool::DisableTool()
  {
    m_MCMCData.Clear();
  }

  void MCMCLearnTool::GetTileInfo(ResourceHandle<Tileset> const& iHandle, TileName iName, TempElementInfo& oInfo)
  {
    oInfo.size = Vector2i::ONE;
    oInfo.thumbnail = m_Cache.GetMiniature(iHandle, iName);
    if (!oInfo.thumbnail.isNull())
    {
      oInfo.size = Vector2i(oInfo.thumbnail.size().width(), oInfo.thumbnail.size().height());
    }

    Resource::Header const* header = ResourceManager::GetHeader(iHandle.GetUUID());
    eXl_ASSERT_REPAIR_RET(header != nullptr, );
    oInfo.desc = String("Tile : ") + header->m_ResourceName;
  }

  void MCMCLearnTool::GetTileInfo(ResourceHandle<Archetype> const& iArchetype, TempElementInfo& oInfo)
  {
    Archetype const* archetype = iArchetype.GetOrLoad();
    eXl_ASSERT_REPAIR_RET(archetype != nullptr, );

    oInfo.size = Vector2i::ONE;

    auto const& components = archetype->GetComponents();
    auto iterGfx = components.find(EngineCommon::GfxSpriteComponentName());
    if (iterGfx == components.end())
    {
      return;
    }
    auto const* gfxDesc = iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>();

    GetTileInfo(gfxDesc->m_Tileset, gfxDesc->m_TileName, oInfo);

    oInfo.size.X() = Mathi::Max(1, Mathf::Round(oInfo.size.X() * gfxDesc->m_Size.X() * EngineCommon::s_WorldToPixel));
    oInfo.size.Y() = Mathi::Max(1, Mathf::Round(oInfo.size.Y() * gfxDesc->m_Size.Y() * EngineCommon::s_WorldToPixel));

    oInfo.desc = String("Object : ") + archetype->GetName();
  }

  void MCMCLearnTool::ReadElements()
  {
    m_Elements.clear();
    m_Ref.clear();
    m_Enabled.clear();
    m_ElementsDisplay->clear();

    UnorderedMap<std::pair<Resource::UUID, Name>, TempElementInfo> elements;

    GameDatabase* database = m_World.GetSystem<GameDatabase>();
    if (auto const* tiles = database->GetView<TileItemData>(TilesTool::ToolDataName()))
    {
      tiles->Iterate([&](ObjectHandle, TileItemData const& iTile)
        {

          auto tileKey = std::make_pair(iTile.m_Tileset.GetUUID(), Name(iTile.m_Tile));
          auto insertRes = elements.insert(std::make_pair(tileKey, TempElementInfo()));
          TempElementInfo& info = insertRes.first->second;
          info.placeable = true;
          info.layers.insert(iTile.m_Layer);
          GetTileInfo(iTile.m_Tileset, iTile.m_Tile, info);
          info.desc = String("Tile : ") + iTile.m_Tile.get();
        });
    }

    if (auto const* terrain = database->GetView<TerrainIslandItemData>(TerrainTool::ToolDataName()))
    {
      terrain->Iterate([&](ObjectHandle, TerrainIslandItemData const& iIsland)
        {
          if (iIsland.m_Terrain == GetWallTypeName())
          {
            return;
          }

          auto tileKey = std::make_pair(iIsland.m_TilingGroup.GetUUID(), Name());
          auto insertRes = elements.insert(std::make_pair(tileKey, TempElementInfo()));
          TempElementInfo& info = insertRes.first->second;
          info.placeable = false;
          info.layers.insert(iIsland.m_Layer);
          TilingGroup const* group = iIsland.m_TilingGroup.GetOrLoad();
          if (group)
          {
            GetTileInfo(group->GetTileset(), group->m_DefaultTile, info);
          }
          info.desc = "Terrain : " + iIsland.m_Terrain.get();
        });
    }

    if (auto const* objects = database->GetView<MapResource::ObjectHeader>(ObjectsTool::ToolDataName()))
    {
      objects->Iterate([&](ObjectHandle, MapResource::ObjectHeader const& iObject)
        {
          auto tileKey = std::make_pair(iObject.m_Archetype.GetUUID(), Name());
          auto insertRes = elements.insert(std::make_pair(tileKey, TempElementInfo()));
          TempElementInfo& info = insertRes.first->second;
          info.placeable = true;
          info.layers.insert(0);
          GetTileInfo(iObject.m_Archetype, info);
        });
    }

    for (auto const& element : elements)
    {
      for(auto layer : element.second.layers)
      {
        MCMC2D::Element newElement;
        newElement.m_Layer = layer;
        if (element.second.placeable)
        {
          newElement.m_RelDensity = 1.0;
          AABB2Di shapeBox(element.second.size / -2, element.second.size);
          newElement.m_Shapes.push_back(Polygoni(shapeBox));
          newElement.m_GridX = element.second.size.X();
          newElement.m_GridY = element.second.size.Y();
        }
        else
        {
          newElement.m_RelDensity = 0.0;
          newElement.m_DirMethod = MCMC2D::eNearestPlane;
        }

        m_Elements.push_back(newElement);
        m_Ref.push_back(MCMCModelRsc::ElementRef {element.first.first, element.first.second});
        m_Enabled.push_back(true);

        QListWidgetItem* newItem = new QListWidgetItem(m_ElementsDisplay);
        newItem->setData(Qt::DecorationRole, element.second.thumbnail);
        newItem->setCheckState(Qt::CheckState::Checked);
        newItem->setText(QString::fromUtf8(element.second.desc.c_str()));
        

        m_ElementsDisplay->addItem(newItem);
      }
    }
  }

  void MCMCLearnTool::TrainModel()
  {
    uint32_t dimScale = m_ShapeScale->value();
    MCMC2D::LearnExample example;

    UnorderedMap<std::pair<Resource::UUID, Name>, uint32_t> selectedElements;

    Vector<MCMC2D::Element> elements;

    for (uint32_t i = 0; i < m_Elements.size(); ++i)
    {
      if (!m_Enabled[i])
      {
        continue;
      }
      selectedElements.insert(std::make_pair(std::make_pair(m_Ref[i].m_Resource, m_Ref[i].m_Subobject), elements.size()));

      elements.push_back(m_Elements[i]);
      MCMC2D::Element& curElem = elements.back();
      for (auto& shape : curElem.m_Shapes)
      {
        shape.Scale(dimScale);
        Vector<Polygoni> shrinkRes;
        shape.Shrink(m_ShapeShrink->value(), shrinkRes);

        if (shrinkRes.size() == 1)
        {
          shape = std::move(shrinkRes[0]);
        }
      }
    }

    Vector<Polygoni> carvers;

    GameDatabase* database = m_World.GetSystem<GameDatabase>();

    if (auto const* terrain = database->GetView<TerrainIslandItemData>(TerrainTool::ToolDataName()))
    {
      terrain->Iterate([&](ObjectHandle, TerrainIslandItemData const& iIsland)
        {
          //TempElementInfo info;
          //TilingGroup const* group = iIsland.m_TilingGroup.GetOrLoad();
          //if (group)
          //{
          //  GetTileInfo(group->GetTileset(), group->m_DefaultTile, info);
          //}

          if (iIsland.m_Terrain == GetWallTypeName())
          {
            if (example.m_Shape.Empty() && !iIsland.m_IslandPoly.Holes().empty())
            {
              example.m_Shape = Polygoni(iIsland.m_IslandPoly.Holes()[0]);
              example.m_Shape.Scale(dimScale);
            }
            else
            {
              carvers.push_back(Polygoni(iIsland.m_IslandPoly.Border()));
              carvers.back().Scale(dimScale);
            }
          }
          else
          {
            auto tileKey = std::make_pair(iIsland.m_TilingGroup.GetUUID(), Name());
            auto iter = selectedElements.find(tileKey);
            if (iter != selectedElements.end())
            {
              MCMC2D::Element& elem = elements[iter->second];
              uint32_t curShape = elem.m_Shapes.size();
              elem.m_Shapes.push_back(Polygoni(iIsland.m_IslandPoly.Border()));
              for (auto const& hole : iIsland.m_IslandPoly.Holes())
              {
                elem.m_Shapes.back().Holes().push_back(hole);
              }
              elem.m_Shapes.back().Scale(dimScale);

              MCMC2D::PlacedElement placedElem;
              placedElem.m_Angle = 0.0;
              placedElem.m_Element = iter->second + 1;
              placedElem.m_ShapeNum = curShape;
              placedElem.m_Pos = Vector2i::ZERO;
              example.m_Elements.push_back(placedElem);
            }
          }
        });
    }

    for (auto const& carver : carvers)
    {
      Vector<Polygoni> polys;
      example.m_Shape.Difference(carver, polys);
      if (polys.size() > 0)
      {
        example.m_Shape = std::move(polys[0]);
      }
    }

    if (example.m_Shape.Empty())
    {
      LOG_ERROR << "MCMC learn : missing walls";
      return;
    }

    ResourceHandle<MCMCModelRsc> handle = *m_CurModelSelector->Object()->CastBuffer<ResourceHandle<MCMCModelRsc>>();
    MCMCModelRsc* model = const_cast<MCMCModelRsc*>(handle.GetOrLoad());
    DocumentState* rscDoc = nullptr;
    if (model == nullptr)
    {
      rscDoc = EditorState::CreateResource(MCMCModelRsc::StaticLoaderName());
      if (rscDoc == nullptr)
      {
        return;
      }
      model = MCMCModelRsc::DynamicCast(rscDoc->GetResource());
      handle.Set(model);
      ConstDynObject handleRef(TypeManager::GetType<ResourceHandle<MCMCModelRsc>>(), &handle);
      m_CurModelSelector->SetObject(&handleRef);
    }
    else
    {
      rscDoc = EditorState::OpenDocument(handle.GetUUID());
    }

    if (model == nullptr)
    {
      return;
    }

    model->m_DimScaling = m_ShapeScale->value();
    model->m_ElementsVector.clear();
    model->m_Model.reset();

    std::unique_ptr<Random> randGen(Random::CreateDefaultRNG(Clock::GetTimestamp()));
    MCMC2D::SVMLearnParams params(*randGen);
    //MCMC2D::DiskLearnParams params(*randGen);
    params.m_QuantileCull = m_UseQuantileCull->checkState() == Qt::Checked;
    params.m_MaxDist_ = m_InteractionRadius->value() * dimScale;
    params.m_Toroidal = false;

    params.m_Resample = false;
    params.m_Oversampling = elements.size() * 4;
    params.m_Dispersion = 2.0 / m_InteractionRadius->value();

    MCMC2D::SVMLearner learner(elements, params);
    //MCMC2D::DiskLearner learner(elements, params);

    if (auto const* tiles = database->GetView<TileItemData>(TilesTool::ToolDataName()))
    {
      tiles->Iterate([&](ObjectHandle, TileItemData const& iTile)
        {
          auto tileKey = std::make_pair(iTile.m_Tileset.GetUUID(), Name(iTile.m_Tile));
          auto iter = selectedElements.find(tileKey);
          if (iter != selectedElements.end())
          {
            MCMC2D::PlacedElement elem;
            elem.m_Angle = 0.0;
            elem.m_Element = iter->second + 1;
            elem.m_ShapeNum = 0;
            elem.m_Pos = iTile.m_Position * dimScale;
            example.m_Elements.push_back(elem);
          }
        });
    }

    if (auto const* objects = database->GetView<MapResource::ObjectHeader>(ObjectsTool::ToolDataName()))
    {
      objects->Iterate([&](ObjectHandle, MapResource::ObjectHeader const& iObject)
        {
          auto tileKey = std::make_pair(iObject.m_Archetype.GetUUID(), Name());
          auto iter = selectedElements.find(tileKey);
          if (iter != selectedElements.end())
          {
            MCMC2D::PlacedElement elem;
            elem.m_Angle = 0.0;
            elem.m_Element = iter->second + 1;
            elem.m_ShapeNum = 0;
            elem.m_Pos = MathTools::ToIVec(MathTools::As2DVec(iObject.m_Position * EngineCommon::s_WorldToPixel * dimScale));
            example.m_Elements.push_back(elem);
          }
        });
    }

    //{
    //  std::ofstream outStr("D:\\TestEx.json");
    //  JSONStreamer streamer(&outStr);
    //
    //  streamer.Begin();
    //  example.Stream(streamer);
    //  streamer.End();
    //}

    MCMC2D::LearnedModel* modelImpl = nullptr;
    learner.Learn(modelImpl, example, 1000, nullptr);
    model->m_ElementsVector.resize(selectedElements.size());
    for (auto const& elemEntry : selectedElements)
    {
      model->m_ElementsVector[elemEntry.second] = MCMCModelRsc::ElementRef{ elemEntry.first.first, elemEntry.first.second };
    }
    model->m_Model.reset(modelImpl);
    rscDoc->Save();
  }

  void MCMCLearnTool::RunModel()
  {
    ResourceHandle<MCMCModelRsc> handle = *m_CurModelSelector->Object()->CastBuffer<ResourceHandle<MCMCModelRsc>>();
    MCMCModelRsc* model = const_cast<MCMCModelRsc*>(handle.GetOrLoad());
    if (model == nullptr || model->m_Model == nullptr)
    {
      return;
    }

    QModelIndexList selected = m_RunSpaceList->selectionModel()->selectedIndexes();
    if (selected.size() != 1)
    {
      return;
    }
    uint32_t selSpace = selected[0].row();

    Polygoni runSpace = m_RunSpaces[selSpace];
    ObjectHandle runSpaceHandle = m_RunSpacesHandle[selSpace];
    runSpace.Scale(model->m_DimScaling);

    Vector<Polygoni> carvers;

    GameDatabase* database = m_World.GetSystem<GameDatabase>();

    if (auto const* terrain = database->GetView<TerrainIslandItemData>(TerrainTool::ToolDataName()))
    {
      terrain->Iterate([&](ObjectHandle iHandle, TerrainIslandItemData const& iIsland)
        {
          if (iHandle == runSpaceHandle)
          {
            return;
          }
          if (iIsland.m_Terrain == GetWallTypeName()
            && iIsland.m_IslandPoly.GetAABB().IsInside(runSpace.GetAABB()))
          {
              carvers.push_back(Polygoni(iIsland.m_IslandPoly.Border()));
              carvers.back().Scale(model->m_DimScaling);
          }
        });
    }

    for (auto const& carver : carvers)
    {
      Vector<Polygoni> polys;
      runSpace.Difference(carver, polys);
      if (polys.size() > 0)
      {
        runSpace = std::move(polys[0]);
      }
    }

    UnorderedMap<std::pair<Resource::UUID, Name>, uint32_t> selectedElements;
    for (uint32_t i = 0; i < model->m_ElementsVector.size(); ++i)
    {
      auto const& elemRef = model->m_ElementsVector[i];
      auto elemKey = std::make_pair(elemRef.m_Resource, elemRef.m_Subobject);
      selectedElements.insert(std::make_pair(elemKey, i));
    }

    MCMC2D::RunParams params;
    params.m_NumIter = m_RunIter->value();
    params.m_Shape = runSpace;

    bool const cleanRun = m_CleanRun->isChecked();

    if (auto const* tiles = database->GetView<TileItemData>(TilesTool::ToolDataName()))
    {
      tiles->Iterate([&](ObjectHandle iHandle, TileItemData const& iTile)
        {
          const bool isMCMCPlaced = m_MCMCData.Get(iHandle) != nullptr;
          if (cleanRun && isMCMCPlaced)
          {
            return;
          }
          auto tileKey = std::make_pair(iTile.m_Tileset.GetUUID(), Name(iTile.m_Tile));
          auto iter = selectedElements.find(tileKey);
          if (iter != selectedElements.end())
          {
            MCMC2D::PlacedElement elem;
            elem.m_Angle = 0.0;
            elem.m_Element = iter->second + 1;
            elem.m_ShapeNum = 0;
            elem.m_Pos = iTile.m_Position * model->m_DimScaling;

            if (isMCMCPlaced)
            {
              params.m_Placed.push_back(elem);
            }
            else
            {
              params.m_Static.push_back(elem);
            }
          }
        });
    }

    if (auto const* objects = database->GetView<MapResource::ObjectHeader>(ObjectsTool::ToolDataName()))
    {
      objects->Iterate([&](ObjectHandle iHandle, MapResource::ObjectHeader const& iObject)
        {
          const bool isMCMCPlaced = m_MCMCData.Get(iHandle) != nullptr;
          if (cleanRun && isMCMCPlaced)
          {
            return;
          }

          auto tileKey = std::make_pair(iObject.m_Archetype.GetUUID(), Name());
          auto iter = selectedElements.find(tileKey);
          if (iter != selectedElements.end())
          {
            MCMC2D::PlacedElement elem;
            elem.m_Angle = 0.0;
            elem.m_Element = iter->second + 1;
            elem.m_ShapeNum = 0;
            elem.m_Pos = MathTools::ToIVec(MathTools::As2DVec(iObject.m_Position * EngineCommon::s_WorldToPixel * model->m_DimScaling));
            if (isMCMCPlaced)
            {
              params.m_Placed.push_back(elem);
            }
            else
            {
              params.m_Static.push_back(elem);
            }
          }
        });
    }

    m_MCMCData.Iterate([&](ObjectHandle iHandle, MCMCTag&)
      {
        m_World.DeleteObject(iHandle);
      });

    m_MCMCData.Clear();

    if (m_Rand == nullptr || cleanRun)
    {
      m_Rand.reset(Random::CreateDefaultRNG(0));
    }

    MCMC2D::Run(*m_Rand, params, model->m_Model.get());

    for (auto const& placed : params.m_Placed)
    {
      Vector2i pixelPos(Mathf::Round(placed.m_Pos.X() / model->m_DimScaling), Mathf::Round(placed.m_Pos.Y() / model->m_DimScaling));
      uint32_t refIdx = placed.m_Element - 1;
      auto const& elemRef = model->m_ElementsVector[refIdx];
      ObjectHandle newObject = m_ParentEditor->Place(elemRef.m_Resource, elemRef.m_Subobject, pixelPos);
      m_MCMCData.GetOrCreate(newObject);
    }
  }
}