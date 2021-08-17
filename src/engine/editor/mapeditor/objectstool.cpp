#include "objectstool.hpp"
#include "pentoolfilter.hpp"
#include "archetypecustomization.hpp"
#include "utils.hpp"

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
	IMPLEMENT_RefC(ObjectsTool::PlacedObject);

  Vector2i GetSizeFromArchetype(Archetype const* iArchetype)
  {
    auto const& components = iArchetype->GetComponents();
    auto iterGfx = components.find(DunAtk::GfxSpriteComponentName());
    if (iterGfx != components.end())
    {
      auto const* gfxDesc = iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>();

      Vector2i tileSize = SafeGetTileSize(gfxDesc->m_Tileset, gfxDesc->m_TileName);
      tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * gfxDesc->m_Size.X() * DunAtk::s_WorldToPixel));
      tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * gfxDesc->m_Size.Y() * DunAtk::s_WorldToPixel));
      return tileSize;
    }

    return Vector2i::ONE;
  }

  void ObjectsTool::UpdateObjectBoxAndTile(PlacedObject& iObject)
  {
    BoxIndexEntry oldBoxEntry(iObject.m_BoxCache, iObject.m_Index);

    Archetype const* archetype = iObject.m_Archetype.GetOrLoad();
    auto const& components = archetype->GetComponents();
    auto const iterGfx = components.find(DunAtk::GfxSpriteComponentName());
    if (iterGfx != components.end())
    {
      DynObject customizedData;
      GfxSpriteComponent::Desc const* spriteData;
      auto iterCusto = iObject.m_CustoData.m_ComponentCustomization.find(DunAtk::GfxSpriteComponentName());
      if (iterCusto == iObject.m_CustoData.m_ComponentCustomization.end())
      {
        spriteData = iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>();
      }
      else
      {
        customizedData = DynObject(&iterGfx->second);
        CustomizationData::ApplyCustomization(customizedData, iterCusto->second);
        spriteData = customizedData.CastBuffer<GfxSpriteComponent::Desc>();
      }

      Vector2i tileSize = SafeGetTileSize(spriteData->m_Tileset, spriteData->m_TileName);
      tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * spriteData->m_Size.X() * DunAtk::s_WorldToPixel));
      tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * spriteData->m_Size.Y() * DunAtk::s_WorldToPixel));
      
      iObject.m_BoxCache = AABB2Di(iObject.m_Position - tileSize / 2, tileSize);

      if (iObject.m_WorldObject.IsAssigned())
      {
        GfxSpriteComponent* spriteComp = m_World.GetSystem<GfxSystem>()->GetSpriteComponent(iObject.m_WorldObject);
        if (spriteComp)
        {
          spriteComp->SetDesc(*spriteData);
        }
      }
    }
    else
    {
      iObject.m_BoxCache = AABB2Di(iObject.m_Position, Vector2i::ONE);
    }

    if (iObject.m_WorldObject.IsAssigned())
    {
      Transforms& trans = *m_World.GetSystem<Transforms>();
      Matrix4f mat = trans.GetLocalTransform(iObject.m_WorldObject);
      MathTools::GetPosition2D(mat) = Vector2f(iObject.m_Position.X(), iObject.m_Position.Y()) / DunAtk::s_WorldToPixel;

      trans.UpdateTransform(iObject.m_WorldObject, mat);
    }

    m_TilesIdx.remove(oldBoxEntry);
    m_TilesIdx.insert(BoxIndexEntry(iObject.m_BoxCache, iObject.m_Index));
  }

	ObjectsTool::ObjectsTool(QWidget* parent, Tools iTools, World& iWorld)
		: EditorTool(parent)
		, m_World(iWorld)
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
          UpdateObjectBoxAndTile(*m_Selection);
        }
      });

      ConnectToValueChanged(m_SelectionY, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.Y() = iNewValue;
          UpdateObjectBoxAndTile(*m_Selection);
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
			IntrusivePtr<PlacedObject> newObject = eXl_NEW PlacedObject;

			newObject->m_Position = MathTools::ToIVec(MathTools::As2DVec(object.m_Header.m_Position)) *  DunAtk::s_WorldToPixel;
			newObject->m_UUID = object.m_Header.m_ObjectId;
      newObject->m_Archetype = object.m_Header.m_Archetype;
			newObject->m_Index = m_Counter++;
      newObject->m_CustoData = object.m_Data;
      AddToWorld(*newObject);

			m_Objects.insert(std::make_pair(newObject->m_Index, newObject));
			m_TilesIdx.insert(std::make_pair(newObject->m_BoxCache, newObject->m_Index));
			m_IDs.insert(std::make_pair(object.m_Header.m_ObjectId, newObject->m_Index));
			
      UpdateObjectBoxAndTile(*newObject);
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
      auto const& components = m_SelectedArchetype->GetComponents();
      auto iterGfx = components.find(ComponentName("GfxSprite"));
      if (iterGfx != components.end())
      {
        GfxSpriteComponent::Desc const* gfxDesc = iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>();

        ObjectHandle penObj = m_World.CreateObject();
        
        AddTileToWorld(penObj, gfxDesc);

        m_Tools.m_Pen->SetPenObject(penObj);
        Vector2i tileSize = SafeGetTileSize(gfxDesc->m_Tileset, gfxDesc->m_TileName);
        tileSize.X() = Mathi::Max(1, Mathf::Round(tileSize.X() * gfxDesc->m_Size.X() * DunAtk::s_WorldToPixel));
        tileSize.Y() = Mathi::Max(1, Mathf::Round(tileSize.Y() * gfxDesc->m_Size.Y() * DunAtk::s_WorldToPixel));
        m_Tools.m_Pen->SetSnapSize(tileSize);
      }
    }
	}

	void ObjectsTool::SetupPenTool()
	{
		SetPenToolTile();

		auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
		{
			ObjectsTool::PlacedObject* obj = GetAt(iPos);
			if (obj)
			{
				return;
			}
			if (m_SelectedArchetype != nullptr)
			{
				PlacedObject* newObj = AddAt(iPos, iWasDrawing);
        AddToWorld(*newObj);
			}
		});

		m_ToolConnections.push_back(newConnection);
	}

  void ObjectsTool::SetupEraserTool()
  {
    m_Tools.m_Pen->SetSnapSize(Vector2i::ONE);
    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
    {
      ObjectsTool::PlacedObject* obj = GetAt(iPos);
      if (obj == nullptr)
      {
        return;
      }
      Remove(*obj);
    });

    m_ToolConnections.push_back(newConnection);
  }

  void ObjectsTool::Remove(PlacedObject& iObject)
  {
    AABB2Di objBox = iObject.m_BoxCache;
    RemoveFromWorld(iObject);

    m_IDs.erase(iObject.m_UUID);
    m_TilesIdx.remove(std::make_pair(objBox, iObject.m_Index));
    m_Objects.erase(iObject.m_Index);

    emit onEditDone();
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
			queryBox.m_Data[0] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * DunAtk::s_WorldToPixel;
			gfxSys.ScreenToWorld(iSelBox.m_Data[1], worldPos, viewDir);
			queryBox.m_Data[1] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * DunAtk::s_WorldToPixel;

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

			auto iter = m_Objects.find(m_ResultsCache[0].second);
			eXl_ASSERT(iter != m_Objects.end());
      SelectObject(iter->second.get());

		});

		m_ToolConnections.push_back(newConnection);
	}

	void ObjectsTool::SelectObject(PlacedObject* iObject)
	{
		if (iObject == m_Selection)
		{
			return;
		}

		if (iObject == nullptr)
		{
			m_SelectionSettings->setEnabled(false);
      ArchetypeCustomizationModel::ClearModelFromView(m_CustoView);
		}

		m_Selection = nullptr;
		if (iObject)
		{
			m_SelectionSettings->setEnabled(true);

      auto* model = ArchetypeCustomizationModel::CreateOrUpdateModel(m_CustoView, iObject->m_Archetype.GetOrLoad(), iObject->m_CustoData);

			m_SelectionX->setValue(iObject->m_Position.X());
			m_SelectionY->setValue(iObject->m_Position.Y());

			m_Selection = iObject;

      QObject::connect(model, &ArchetypeCustomizationModel::dataChanged, [this, model](QModelIndex)
      {
        if (m_Selection)
        {
          m_Selection->m_CustoData = model->GetCustomization();
          UpdateObjectBoxAndTile(*m_Selection);

          emit onEditDone();
        }
      });
		}
	}

	ObjectsTool::PlacedObject* ObjectsTool::GetAt(Vector2i iWorldPos)
	{
		AABB2Di queryBox(iWorldPos, Vector2i::ONE);

		QueryResult results(m_ResultsCache);
		m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

		if (results.empty())
		{
			return nullptr;
		}

		auto iter = m_Objects.find(m_ResultsCache[0].second);
		eXl_ASSERT(iter != m_Objects.end());

		return iter->second.get();
	}

	ObjectsTool::PlacedObject* ObjectsTool::AddAt(Vector2i iPixelPos, bool iAppend)
	{
		IntrusivePtr<PlacedObject> newObject = eXl_NEW PlacedObject;
		newObject->m_Position = iPixelPos;
    newObject->m_Archetype.Set(m_SelectedArchetype);
		newObject->m_Index = m_Counter++;
    do
    {
      newObject->m_UUID = MapResource::ObjectHeader::AllocObjectID();
    } while (m_IDs.count(newObject->m_UUID) > 0);
    AddToWorld(*newObject);

		m_Objects.insert(std::make_pair(newObject->m_Index, newObject));
    m_IDs.insert(std::make_pair(newObject->m_UUID, newObject->m_Index));
		m_TilesIdx.insert(std::make_pair(newObject->m_BoxCache, newObject->m_Index));

		emit onEditDone();

    UpdateObjectBoxAndTile(*newObject);
    SelectObject(newObject.get());

		return newObject.get();
	}

	void ObjectsTool::AddToWorld(PlacedObject& iObject)
	{
		if (iObject.m_WorldObject.IsAssigned())
		{
			return;
		}

		iObject.m_WorldObject = m_World.CreateObject();
		Transforms& trans = *m_World.GetSystem<Transforms>();
		GfxSystem& gfx = *m_World.GetSystem<GfxSystem>();

		Matrix4f worldTrans;
		worldTrans.MakeIdentity();
		MathTools::GetPosition2D(worldTrans) = Vector2f(iObject.m_Position.X(), iObject.m_Position.Y()) / DunAtk::s_WorldToPixel;
		trans.AddTransform(iObject.m_WorldObject, &worldTrans);

    auto const& components = iObject.m_Archetype.GetOrLoad()->GetComponents();
    auto iterGfx = components.find(DunAtk::GfxSpriteComponentName());
    if (iterGfx != components.end())
    {
      AddTileToWorld(iObject.m_WorldObject, iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>());
    }
	}

  void ObjectsTool::AddTileToWorld(ObjectHandle iObj, GfxSpriteComponent::Desc const* iDesc)
  {
    GfxSpriteComponent& sprite = m_World.GetSystem<GfxSystem>()->CreateSpriteComponent(iObj);
    sprite.SetDesc(*iDesc);
  }

	void ObjectsTool::RemoveFromWorld(PlacedObject& iTile)
	{
		if (!iTile.m_WorldObject.IsAssigned())
		{
			return;
		}
		m_World.DeleteObject(iTile.m_WorldObject);
		iTile.m_WorldObject = ObjectHandle();
	}
}
