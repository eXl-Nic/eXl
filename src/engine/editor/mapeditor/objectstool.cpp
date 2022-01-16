#include "objectstool.hpp"
#include "pentoolfilter.hpp"
#include "archetypecustomization.hpp"
#include "utils.hpp"

#include <core/type/tagtype.hpp>

#include <editor/editordef.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/tileset/tilecollectionmodel.hpp>
#include <editor/editoricons.hpp>
#include <editor/resourceselectionwidget.hpp>
#include <editor/objectdelegate.hpp>

#include <math/mathtools.hpp>
#include <math/aabb2dpolygon.hpp>
#include <math/geometrytraits.hpp>

#include <engine/map/map.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/common/app.hpp>

#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QActionGroup>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>
#include <QTreeView>

namespace eXl
{
	IMPLEMENT_TAG_TYPE_EX(ObjectsTool::PlacedObject, ObjectsTool__PlacedObject);

  Vector2i GetSizeFromArchetype(Archetype const* iArchetype)
  {
    auto const& data = iArchetype->GetProperties();
    auto iterGfx = data.find(EngineCommon::GfxSpriteDescName());
    if (iterGfx != data.end())
    {
      auto const* gfxDesc = iterGfx->second.m_Data.CastBuffer<GfxSpriteComponent::Desc>();

      Vector2i tileSize = SafeGetTileSize(gfxDesc->m_Tileset, gfxDesc->m_TileName);
      tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * gfxDesc->m_Size.X() * EngineCommon::s_WorldToPixel));
      tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * gfxDesc->m_Size.Y() * EngineCommon::s_WorldToPixel));
      return tileSize;
    }

    return Vector2i::ONE;
  }

  Vector2i SafeGetSizeFromArchetype(ResourceHandle<Archetype> const& iArchetype)
  {
    Archetype const* rsc = iArchetype.GetOrLoad();
    if (rsc == nullptr)
    {
      return Vector2i::ONE;
    }

    return GetSizeFromArchetype(rsc);
  }

  void ObjectsTool::UpdateObjectBoxAndTile(ObjectHandle iHandle, MapResource::ObjectHeader& iObject)
  {
    PlacedObject& custoData = m_ObjectsEditorData.GetOrCreate(iHandle);
    BoxIndexEntry oldBoxEntry(custoData.m_BoxCache, iHandle);

    Vector2i pixelPos = MathTools::ToIVec(MathTools::As2DVec(iObject.m_Position) * EngineCommon::s_WorldToPixel);

    Archetype const* archetype = iObject.m_Archetype.GetOrLoad();
    auto const& data = archetype->GetProperties();
    auto const iterGfx = data.find(EngineCommon::GfxSpriteDescName());
    if (iterGfx != data.end())
    {
      DynObject customizedData;
      GfxSpriteComponent::Desc const* spriteData;
      auto iterCusto = custoData.m_CustoData.m_PropertyCustomization.find(EngineCommon::GfxSpriteDescName());
      if (iterCusto == custoData.m_CustoData.m_PropertyCustomization.end())
      {
        spriteData = iterGfx->second.m_Data.CastBuffer<GfxSpriteComponent::Desc>();
      }
      else
      {
        customizedData = DynObject(&iterGfx->second.m_Data);
        CustomizationData::ApplyCustomization(customizedData, iterCusto->second);
        spriteData = customizedData.CastBuffer<GfxSpriteComponent::Desc>();
      }

      Vector2i tileSize = SafeGetTileSize(spriteData->m_Tileset, spriteData->m_TileName);
      tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * spriteData->m_Size.X() * EngineCommon::s_WorldToPixel));
      tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * spriteData->m_Size.Y() * EngineCommon::s_WorldToPixel));
      
      custoData.m_BoxCache = AABB2Di(pixelPos - tileSize / 2, tileSize);

      
      GfxSpriteComponent* spriteComp = m_World.GetSystem<GfxSystem>()->GetSpriteComponent(iHandle);
      if (spriteComp)
      {
        spriteComp->SetDesc(*spriteData);
      }
    }
    else
    {
      custoData.m_BoxCache = AABB2Di(pixelPos, Vector2i::ONE);
    }

    Transforms& trans = *m_World.GetSystem<Transforms>();
    Matrix4f mat = trans.GetLocalTransform(iHandle);
    MathTools::GetPosition(mat) = iObject.m_Position;

    trans.UpdateTransform(iHandle, mat);

    m_TilesIdx.remove(oldBoxEntry);
    m_TilesIdx.insert(BoxIndexEntry(custoData.m_BoxCache, iHandle));
  }

  PropertySheetName ObjectsTool::ToolDataName()
  {
    static PropertySheetName s_Name("PlacedObject");
    return s_Name;
  }

	ObjectsTool::ObjectsTool(QWidget* parent, Tools iTools, World& iWorld)
		: EditorTool(parent, iWorld)
		, m_World(iWorld)
    , m_ObjectsView(*iWorld.GetSystem<GameDatabase>()->GetView<MapResource::ObjectHeader>(ToolDataName()))
    , m_ObjectsEditorData(iWorld)
		, m_Tools(iTools)
	{
		m_Icons[None] = EditorIcons::GetMoveToolIcon();
		m_Icons[PenTool] = EditorIcons::GetDrawToolIcon();
		m_Icons[EraserTool] = EditorIcons::GetEraseToolIcon();
		m_HIcons[None] = EditorIcons::GetHighlightedMoveToolIcon();
		m_HIcons[PenTool] = EditorIcons::GetHighlightedDrawToolIcon();
		m_HIcons[EraserTool] = EditorIcons::GetHighlightedEraseToolIcon();

		QVBoxLayout* toolLayout = new QVBoxLayout(this);
		setLayout(toolLayout);
		QSplitter* toolSplitter = new QSplitter(Qt::Vertical);

		{
			QToolBar* tilesTool = new QToolBar(this);

			m_Actions[None] = tilesTool->addAction(m_Icons[None], "Move", [this]
			{
				ChangeTool(None);
			});

			m_Actions[PenTool] = tilesTool->addAction(m_Icons[PenTool], "Draw", [this]
			{
				ChangeTool(PenTool);
			});

			m_Actions[EraserTool] = tilesTool->addAction(m_Icons[EraserTool], "Erase", [this]
			{
				ChangeTool(EraserTool);
			});

			QActionGroup* grp = new QActionGroup(this);
			grp->addAction(m_Actions[None]);
			grp->addAction(m_Actions[PenTool]);
			grp->addAction(m_Actions[EraserTool]);

			toolSplitter->addWidget(tilesTool);
			ChangeTool(None);
		}

		{
			QWidget* penSettings = new QWidget(this);
			QVBoxLayout* layout = new QVBoxLayout(penSettings);
			penSettings->setLayout(layout);

      TResourceSelectionWidget<Archetype>* archetypeSelection = new TResourceSelectionWidget<Archetype>(penSettings, ResourceSelectionWidget::List);

			layout->addWidget(archetypeSelection);
      QObject::connect(archetypeSelection, &TResourceSelectionWidget<Archetype>::onResourceChanged, [this, archetypeSelection]()
      {
        m_SelectedArchetype = archetypeSelection->GetSelectedResource().GetOrLoad();

        if (m_Tool == PenTool)
        {
        }

      });

      toolSplitter->addWidget(penSettings);
		}

		{
			m_SelectionSettings = new QWidget(this);
			QVBoxLayout* selectionLayout = new QVBoxLayout(m_SelectionSettings);
			m_SelectionSettings->setLayout(selectionLayout);
			m_SelectionSettings->setEnabled(false);

      m_SelectionX = new QSpinBox(m_SelectionSettings);
      m_SelectionX->setMinimum(INT32_MIN);
      m_SelectionX->setMaximum(INT32_MAX);

      m_SelectionY = new QSpinBox(m_SelectionSettings);
      m_SelectionY->setMinimum(INT32_MIN);
      m_SelectionY->setMaximum(INT32_MAX);

      ConnectToValueChanged(m_SelectionX, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.X() = iNewValue;
          UpdateObjectBoxAndTile(m_SelectionHandle, *m_Selection);
        }
      });

      ConnectToValueChanged(m_SelectionY, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.Y() = iNewValue;
          UpdateObjectBoxAndTile(m_SelectionHandle, *m_Selection);
        }
      });

      QWidget* posWidget = new QWidget(m_SelectionSettings);
      QHBoxLayout* posWidgetLayout = new QHBoxLayout(posWidget);
      posWidget->setLayout(posWidgetLayout);

      posWidgetLayout->addWidget(new QLabel("X : ", this));
      posWidgetLayout->addWidget(m_SelectionX);
      posWidgetLayout->addWidget(new QLabel("Y : ", this));
      posWidgetLayout->addWidget(m_SelectionY);

      selectionLayout->addWidget(posWidget);

      m_CustoView = new QTreeView(m_SelectionSettings);
      m_CustoView->setItemDelegate(new ObjectDelegate(m_CustoView));

      selectionLayout->addWidget(m_CustoView);

			toolSplitter->addWidget(m_SelectionSettings);
		}
		toolLayout->addWidget(toolSplitter);
	}

	void ObjectsTool::Initialize(MapResource const& iMap)
	{
		for (auto const& object : iMap.m_Objects)
		{
      ObjectHandle newObjectHandle = m_World.CreateObject();
			MapResource::ObjectHeader& newObject = m_ObjectsView.GetOrCreate(newObjectHandle);
      newObject = object.m_Header;
      PlacedObject& newEditorData = m_ObjectsEditorData.GetOrCreate(newObjectHandle);
      newEditorData.m_CustoData = object.m_Data;
      m_EditorMgr.CreateComponent(newObjectHandle);
      AddToWorld(newObjectHandle, object.m_Header);

			m_TilesIdx.insert(std::make_pair(newEditorData.m_BoxCache, newObjectHandle));
			m_IDs.insert(std::make_pair(object.m_Header.m_ObjectId, newObjectHandle));
			
      UpdateObjectBoxAndTile(newObjectHandle, newObject);
		}
	}

	void ObjectsTool::ClearPenToolTile()
	{
		ObjectHandle prevObject = m_Tools.m_Pen->GetPenObject();
		if (prevObject.IsAssigned())
		{
			m_World.DeleteObject(prevObject);
			m_Tools.m_Pen->SetPenObject(ObjectHandle());
		}
	}

	void ObjectsTool::SetPenToolTile()
	{
		ClearPenToolTile();
    if (m_SelectedArchetype != nullptr)
    {
      Vector2i tileSize = GetSizeFromArchetype(m_SelectedArchetype);
      auto const& data = m_SelectedArchetype->GetProperties();
      auto iterGfx = data.find(EngineCommon::GfxSpriteDescName());
      if (iterGfx != data.end())
      {
        ObjectHandle penObj = m_World.CreateObject();

        auto const* gfxDesc = iterGfx->second.m_Data.CastBuffer<GfxSpriteComponent::Desc>();
        AddTileToWorld(penObj, gfxDesc);

        m_Tools.m_Pen->SetPenObject(penObj);
        SafeGetTileSize(gfxDesc->m_Tileset, gfxDesc->m_TileName);
        tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * gfxDesc->m_Size.X() * EngineCommon::s_WorldToPixel));
        tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * gfxDesc->m_Size.Y() * EngineCommon::s_WorldToPixel));
        m_Tools.m_Pen->SetSnapSize(tileSize);
      }
    }
	}

	void ObjectsTool::SetupPenTool()
	{
		SetPenToolTile();

		auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
		{
			ObjectHandle obj = GetAt(iPos);
			if (obj.IsAssigned())
			{
				return;
			}
			if (m_SelectedArchetype != nullptr)
			{
				ObjectHandle object = AddAt(iPos);
			}
		});

		m_ToolConnections.push_back(newConnection);
	}

  void ObjectsTool::SetupEraserTool()
  {
    m_Tools.m_Pen->SetSnapSize(Vector2i::ONE);
    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
    {
      ObjectHandle obj = GetAt(iPos);
      if (!obj.IsAssigned())
      {
        return;
      }
      Remove(obj);
    });

    m_ToolConnections.push_back(newConnection);
  }

  void ObjectsTool::Remove(ObjectHandle iHandle)
  {
    MapResource::ObjectHeader* object = m_ObjectsView.Get(iHandle);
    if (object != nullptr)
    {
      PlacedObject* custoData = m_ObjectsEditorData.Get(iHandle);
      AABB2Di objBox = custoData->m_BoxCache;
      
      m_IDs.erase(object->m_ObjectId);
      m_TilesIdx.remove(std::make_pair(objBox, iHandle));
      object->m_Archetype = ResourceHandle<Archetype>();
      custoData->m_CustoData.m_ComponentCustomization.clear();
      custoData->m_CustoData.m_PropertyCustomization.clear();

      m_World.DeleteObject(iHandle);

      emit onEditDone();
    }
  }

  void ObjectsTool::Cleanup(ObjectHandle iHandle)
  {
    MapResource::ObjectHeader const* object = m_ObjectsView.GetDataForDeletion(iHandle);
    if (object == nullptr)
    {
      return;
    }
    PlacedObject const* custoData = m_ObjectsEditorData.GetDataForDeletion(iHandle);
    if (custoData)
    {
      AABB2Di objBox = custoData->m_BoxCache;
      m_TilesIdx.remove(std::make_pair(objBox, iHandle));
    }
    
    m_IDs.erase(object->m_ObjectId);
  }

	void ObjectsTool::EnableTool()
	{
		ChangeTool(m_Tool, true);
	}

	void ObjectsTool::DisableTool()
	{
		ClearPenToolTile();
		CleanupToolConnections();
	}

	void ObjectsTool::CleanupToolConnections()
	{
		for (auto connection : m_ToolConnections)
		{
			QObject::disconnect(connection);
		}
		m_ToolConnections.clear();
	}

	void ObjectsTool::ChangeTool(CurTool iTool, bool iForceRefresh)
	{
		if (m_Tool != iTool || iForceRefresh)
		{
			m_Actions[m_Tool]->setIcon(m_Icons[m_Tool]);
			m_Actions[iTool]->setIcon(m_HIcons[iTool]);
			m_Tool = iTool;

			if (m_Tool != PenTool)
			{
				ClearPenToolTile();
			}

			CleanupToolConnections();

			QObject* newFilter = nullptr;
			switch (iTool)
			{
			case None:
				SetupSelectionTool();
				newFilter = m_Tools.m_Selection;
				break;
			case PenTool:
				SetupPenTool();
				newFilter = m_Tools.m_Pen;

				break;
			case EraserTool:
        SetupEraserTool(); 
				newFilter = m_Tools.m_Pen;

				break;
			}

			emit onToolChanged(newFilter, nullptr);
		}
	}

	void ObjectsTool::SetupSelectionTool()
	{
		auto newConnection = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionEnded, [this](AABB2Di const& iSelBox)
		{
			GfxSystem& gfxSys = *m_World.GetSystem<GfxSystem>();

			AABB2Di queryBox;
			Vector3f worldPos;
			Vector3f viewDir;
			gfxSys.ScreenToWorld(iSelBox.m_Data[0], worldPos, viewDir);
			queryBox.m_Data[0] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;
			gfxSys.ScreenToWorld(iSelBox.m_Data[1], worldPos, viewDir);
			queryBox.m_Data[1] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;

			// World and Screen space have opposite Y directions
			std::swap(queryBox.m_Data[0].Y(), queryBox.m_Data[1].Y());

			queryBox.m_Data[1].X() = Mathi::Max(queryBox.m_Data[0].X() + 1, queryBox.m_Data[1].X());
			queryBox.m_Data[1].Y() = Mathi::Max(queryBox.m_Data[0].Y() + 1, queryBox.m_Data[1].Y());

			QueryResult results(m_ResultsCache);

			m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

			if (results.empty())
			{
				return;
			}
			if (results.size() > 1)
			{
				LOG_WARNING << "Multiple selection not yet supported" << "\n";
				return;
			}

      SelectObject(results.m_Result[0].second);
		});

		m_ToolConnections.push_back(newConnection);
	}

	void ObjectsTool::SelectObject(ObjectHandle iHandle)
	{
		if (iHandle == m_SelectionHandle)
		{
			return;
		}

		if (!m_World.IsObjectValid(iHandle))
		{
			m_SelectionSettings->setEnabled(false);
      ArchetypeCustomizationModel::ClearModelFromView(m_CustoView);
		}

		m_Selection = nullptr;
    m_SelectionHandle = ObjectHandle();
		if (m_World.IsObjectValid(iHandle))
		{
      m_SelectionHandle = iHandle;
			m_SelectionSettings->setEnabled(true);
      MapResource::ObjectHeader* object = m_ObjectsView.Get(iHandle);
      eXl_ASSERT(object != nullptr);
      PlacedObject* custoData = m_ObjectsEditorData.Get(iHandle);

      auto* model = ArchetypeCustomizationModel::CreateOrUpdateModel(m_CustoView, object->m_Archetype.GetOrLoad(), custoData->m_CustoData);

			m_SelectionX->setValue(object->m_Position.X());
			m_SelectionY->setValue(object->m_Position.Y());

			m_Selection = object;

      QObject::connect(model, &ArchetypeCustomizationModel::dataChanged, [this, model](QModelIndex)
      {
        if (m_Selection)
        {
          PlacedObject* custoData = m_ObjectsEditorData.Get(m_SelectionHandle);
          custoData->m_CustoData = model->GetCustomization();
          UpdateObjectBoxAndTile(m_SelectionHandle, *m_Selection);

          emit onEditDone();
        }
      });
		}
	}

	ObjectHandle ObjectsTool::GetAt(Vector2i iWorldPos)
	{
		AABB2Di queryBox(iWorldPos, Vector2i::ONE);

		QueryResult results(m_ResultsCache);
		m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

		if (results.empty())
		{
			return ObjectHandle();
		}

		return m_ResultsCache[0].second;
	}

  ObjectHandle ObjectsTool::AddAt(Vector2i iPixelPos)
  {
    return AddAt(m_SelectedArchetype, iPixelPos);
  }

  ObjectHandle ObjectsTool::AddAt(Archetype const* iArchetype, Vector2i iPixelPos)
	{
    ObjectHandle newObjectHandle = m_World.CreateObject();
    MapResource::ObjectHeader newObject = m_ObjectsView.GetOrCreate(newObjectHandle);
    PlacedObject& custoData = m_ObjectsEditorData.GetOrCreate(newObjectHandle);
		newObject.m_Position = MathTools::To3DVec(MathTools::ToFVec(iPixelPos / EngineCommon::s_WorldToPixel));
    newObject.m_Archetype.Set(m_SelectedArchetype);
    do
    {
      newObject.m_ObjectId = MapResource::ObjectHeader::AllocObjectID();
    } while (m_IDs.count(newObject.m_ObjectId) > 0);

    m_EditorMgr.CreateComponent(newObjectHandle);

    AddToWorld(newObjectHandle, newObject);

    m_IDs.insert(std::make_pair(newObject.m_ObjectId, newObjectHandle));
		m_TilesIdx.insert(std::make_pair(custoData.m_BoxCache, newObjectHandle));

		emit onEditDone();

    UpdateObjectBoxAndTile(newObjectHandle, newObject);
    SelectObject(newObjectHandle);

		return newObjectHandle;
	}

	void ObjectsTool::AddToWorld(ObjectHandle iHandle, MapResource::ObjectHeader const& iObject)
	{
		Transforms& trans = *m_World.GetSystem<Transforms>();
		GfxSystem& gfx = *m_World.GetSystem<GfxSystem>();

		trans.AddTransform(iHandle, Matrix4f::FromPosition(iObject.m_Position));

    auto const& data = iObject.m_Archetype.GetOrLoad()->GetProperties();
    auto iterGfx = data.find(EngineCommon::GfxSpriteDescName());
    if (iterGfx != data.end())
    {
      AddTileToWorld(iHandle, iterGfx->second.m_Data.CastBuffer<GfxSpriteComponent::Desc>());
    }
	}

  void ObjectsTool::AddTileToWorld(ObjectHandle iObj, GfxSpriteComponent::Desc const* iDesc)
  {
    GfxSpriteComponent& sprite = m_World.GetSystem<GfxSystem>()->CreateSpriteComponent(iObj);
    sprite.SetDesc(*iDesc);
  }
}
