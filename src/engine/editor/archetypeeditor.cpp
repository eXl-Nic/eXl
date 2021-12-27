#include "archetypeeditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include "objectmodel.hpp"
#include "customizationmodel.hpp"
#include "objectdelegate.hpp"
#include "gamewidget.hpp"
#include "aspectratiowidget.hpp"
#include <core/input.hpp>

#include "collectionmodel.hpp"
#include <editor/tileset/tilecollectionmodel.hpp>
#include <editor/tileset/tileitemdelegate.hpp>

#include <engine/game/archetype.hpp>

#include <engine/gfx/gfxcomponent.hpp>
#include <engine/physics/physicsys.hpp>

#include <engine/common/app.hpp>

#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QLabel>
#include <QFileDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QComboBox>

namespace eXl
{
  class PropertySheetCollectionModel : public CollectionModel<PropertySheetName, Archetype::PropertyEntry, Archetype>
  {
    //Q_OBJECT
  public:
    static PropertySheetCollectionModel* Create(QObject* iParent, Archetype* iArchetype);

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &iIndex, const QVariant &value, int role);

    int columnCount(const QModelIndex &parent) const override
    {
      return 2;
    }

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

  protected:
    PropertySheetCollectionModel(QObject* iParent, Archetype* iGroup);
    bool AddToResource(PropertySheetName const& iName, Archetype::PropertyEntry const&) override;
    bool RemoveFromResource(PropertySheetName const& iName) override;
    Archetype::PropertyEntry const* FindInResource(PropertySheetName const& iName) const override;

  };

  PropertySheetCollectionModel* PropertySheetCollectionModel::Create(QObject* iParent, Archetype* iArchetype)
  {
    PropertySheetCollectionModel* newModel = new PropertySheetCollectionModel(iParent, iArchetype);
    newModel->BuildMap(iArchetype->GetProperties());

    return newModel;
  }

  Qt::ItemFlags PropertySheetCollectionModel::flags(const QModelIndex &index) const
  {
    if (index.isValid())
    {
      Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

      if (index.column() == 1)
      {
        flags |= Qt::ItemIsUserCheckable;
      }

      return flags;
    }
    return Qt::ItemFlag();
  }

  bool PropertySheetCollectionModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if (!iIndex.isValid())
    {
      return false;
    }

    PropertySheetName name = m_IndexToName[iIndex.row()];

    if (role == Qt::CheckStateRole && iIndex.column() == 1)
    {
      auto iter = m_Resource->GetProperties().find(name);
      eXl_ASSERT_REPAIR_RET(iter != m_Resource->GetProperties().end(), false);

      auto checkState = value.value<Qt::CheckState>();

      DynObject dataCopy(&iter->second.m_Data);
      m_Resource->SetProperty(name, dataCopy, checkState == Qt::Checked);

      emit dataChanged(iIndex, iIndex);
      return true;
    }

    if (role == Qt::EditRole && iIndex.column() == 0)
    {
      PropertySheetName newName(value.toString().toUtf8());
      Type const* type = EditorState::GetProjectProperties().GetTypeFromName(newName);
      eXl_ASSERT_REPAIR_RET(type != nullptr, false);
      if (m_NameToIndex.find(newName) == m_NameToIndex.end())
      {
        DynObject newData;
        newData.SetType(type, type->Build(), true);
        m_Resource->SetProperty(newName, newData, false);
      
        m_IndexToName[iIndex.row()] = newName;
        m_NameToIndex.erase(name);
        m_NameToIndex.insert(std::make_pair(newName, iIndex.row()));
      
        emit dataChanged(iIndex, iIndex);
        return true;
      }
    }
    return false;
  }

  QVariant PropertySheetCollectionModel::data(const QModelIndex &index, int role) const
  {
    PropertySheetName const* name;
    Archetype::PropertyEntry const* value;
    if (index.isValid() && index.row() < m_IndexToName.size())
    {
      name = &m_IndexToName[index.row()];
      value = GetObjectFromIndex(index);
    }

    eXl_ASSERT_REPAIR_RET(name != nullptr && value != nullptr, QVariant());

    if (role == Qt::CheckStateRole)
    {
      if (index.column() == 1)
      {
        return value->m_Instanced ? Qt::Checked : Qt::Unchecked;
      }
    }

    if (role == Qt::DisplayRole
      || role == Qt::EditRole)
    {
      if (index.column() == 0)
      {
        return QString::fromUtf8(name->get().c_str());
      }
      if (index.column() == 1)
      {
        return value->m_Instanced;
      }
    }
    return QVariant();
  }

  QVariant PropertySheetCollectionModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
    if (role == Qt::DisplayRole)
    {
      if (orientation == Qt::Horizontal)
      {
        if(section == 0)
        {
          return QString("Property Sheet");
        }
        else if(section == 1)
        {
          return QString("Instanced");
        }
      }
    }
    return QVariant();
  }

  PropertySheetCollectionModel::PropertySheetCollectionModel(QObject* iParent, Archetype* iArchetype)
    : CollectionModel(iParent, iArchetype)
  {
  }

  bool PropertySheetCollectionModel::AddToResource(PropertySheetName const& iName, Archetype::PropertyEntry const& iValue)
  {
    Type const* propType = EditorState::GetProjectProperties().GetTypeFromName(iName);
    if (propType != nullptr)
    {
      DynObject obj = iValue.m_Data.Ref();
      if (!obj.IsValid())
      {
        obj.SetType(propType, propType->Build());
      }
      m_Resource->SetProperty(iName, obj, iValue.m_Instanced);
    }
    return true;
  }

  bool PropertySheetCollectionModel::RemoveFromResource(PropertySheetName const& iName)
  {
    m_Resource->RemoveProperty(iName);
    return true;
  }

  Archetype::PropertyEntry const* PropertySheetCollectionModel::FindInResource(PropertySheetName const& iName) const
  {
    auto iter = m_Resource->GetProperties().find(iName);
    if (iter != m_Resource->GetProperties().end())
    {
      return &iter->second;
    }
    return nullptr;
  }

  class ComponentCollectionModel : public CollectionModel<ComponentName, ConstDynObject, Archetype>
  {
    //Q_OBJECT
  public:
    static ComponentCollectionModel* Create(QObject* iParent, Archetype* iArchetype);

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool setData(const QModelIndex &iIndex, const QVariant &value, int role);

  protected:
    ComponentCollectionModel(QObject* iParent, Archetype* iGroup);
    bool AddToResource(ComponentName const& iName, ConstDynObject const& iObject) override;
    bool RemoveFromResource(ComponentName const& iName) override;
    ConstDynObject const* FindInResource(ComponentName const& iName) const override;

  };

  ComponentCollectionModel* ComponentCollectionModel::Create(QObject* iParent, Archetype* iArchetype)
  {
    ComponentCollectionModel* newModel = new ComponentCollectionModel(iParent, iArchetype);
    newModel->BuildMap(iArchetype->GetComponents());

    return newModel;
  }

  Qt::ItemFlags ComponentCollectionModel::flags(const QModelIndex &index) const
  {
    if (index.isValid())
    {
      Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      return flags;
    }
    return Qt::ItemFlag();
  }

  bool ComponentCollectionModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if (iIndex.isValid() && role == Qt::EditRole && iIndex.column() == 0)
    {
      ComponentName name(value.toString().toUtf8().data());
      if (m_NameToIndex.find(name) == m_NameToIndex.end())
      {
        ComponentName oldName = m_IndexToName[iIndex.row()];

        Type const* type = EngineCommon::GetComponents().GetComponentTypeFromName(name);
        DynObject newData;
        newData.SetType(type, type->Build(), true);
        m_Resource->SetComponent(name, newData);

        m_IndexToName[iIndex.row()] = name;
        m_NameToIndex.erase(oldName);
        m_NameToIndex.insert(std::make_pair(name, iIndex.row()));

        emit dataChanged(iIndex, iIndex);
        return true;
      }
    }
    return false;
  }

  ComponentCollectionModel::ComponentCollectionModel(QObject* iParent, Archetype* iArchetype)
    : CollectionModel(iParent, iArchetype)
  {
  }

  bool ComponentCollectionModel::AddToResource(ComponentName const& iName, ConstDynObject const& iObject)
  {
    if (EngineCommon::GetComponents().GetComponentTypeFromName(iName))
    {
      m_Resource->SetComponent(iName, iObject);
    }
    return true;
  }

  bool ComponentCollectionModel::RemoveFromResource(ComponentName const& iName)
  {
    m_Resource->RemoveComponent(iName);
    return true;
  }

  ConstDynObject const* ComponentCollectionModel::FindInResource(ComponentName const& iName) const
  {
    ConstDynObject const& prop = m_Resource->GetComponent(iName);
    if (prop.IsValid())
    {
      return &prop;
    }
    return nullptr;
  }

  ResourceEditorHandler& ArchetypeEditor::GetEditorHandler()
  {
    static EditorHandler_T<Archetype, ArchetypeEditor> s_Handler;
    return s_Handler;
  }

	struct ArchetypeEditor::Impl
	{
		Impl()
		{
      m_World.Init(EditorState::GetProjectProperties()).WithGfx();

      World& world = m_World.GetWorld();
      GfxSystem& gfx = *world.GetSystem<GfxSystem>();
      PhysicsSystem& ph = *world.GetSystem<PhysicsSystem>();
      
      ph.EnableDebugDraw(*gfx.GetDebugDrawer());
		}

    ObjectHandle m_Handle;
    WorldState m_World;
    InputSystem m_Inputs;
    ObjectHandle m_ArchetypeObject;
		
    QModelIndex m_GroupSelection;

    PropertySheetCollectionModel* m_PropsCollectionModel;
    ComponentCollectionModel* m_CompCollectionModel;

    QTreeView* m_PropDataView;
    QTableView* m_PropCollectionView;

    QTreeView* m_ComponentDataView;
    QListView* m_ComponentCollectionView;

    QModelIndex m_CurrentPropIdx;
    QModelIndex m_CurrentCompIdx;

    PropertySheetName m_CurrentEditedSheetName;
    DynObject m_CurrentEditedSheet;
    //ObjectModel* m_CurrentPropertyModel = nullptr;
    
    ArchetypeEditor* m_Editor;
    Archetype* m_Archetype;

    ComponentName m_CurrentEditedComponentName;
    DynObject m_CurrentEditedComponent;
    //ObjectModel* m_CurrentComponentModel = nullptr;

    Vector<PropertySheetName> m_AvailablePropNames;
    Vector<ComponentName> m_AvailableComponentNames;


    TupleType const* m_GfxSpriteDescType = TupleType::DynamicCast(GfxSpriteComponent::Desc::GetType());
    uint32_t m_TilesetField;
    uint32_t m_TilenameFieldIdx;
    
    void UpdateEditedProperty(QModelIndex);
    void UpdateEditedComponent(QModelIndex);
    void UpdateTileItemDelegate();
    void RecreateObject();
	};

  void ArchetypeEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  ArchetypeEditor::ArchetypeEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Archetype = Archetype::DynamicCast(iDoc->GetResource());

    m_Impl->m_PropsCollectionModel = PropertySheetCollectionModel::Create(this, m_Impl->m_Archetype);
    m_Impl->m_CompCollectionModel = ComponentCollectionModel::Create(this, m_Impl->m_Archetype);

    m_Impl->m_GfxSpriteDescType->GetFieldDetails("m_Tileset", m_Impl->m_TilesetField);
    m_Impl->m_GfxSpriteDescType->GetFieldDetails("m_TileName", m_Impl->m_TilenameFieldIdx);

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter* dataSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter* componentsSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter* gameViewSplitter = new QSplitter(Qt::Vertical, this);
    {
      QWidget* propCollection = new QWidget(this);
      QVBoxLayout* propCollectionLayout = new QVBoxLayout(this);
      propCollection->setLayout(propCollectionLayout);
      QToolBar* propCollectionTool = new QToolBar(this);
      m_Impl->m_PropCollectionView = new QTableView(this);
      m_Impl->m_PropCollectionView->setShowGrid(false);
      m_Impl->m_PropCollectionView->setModel(m_Impl->m_PropsCollectionModel);
      m_Impl->m_PropCollectionView->setSelectionModel(new QItemSelectionModel(m_Impl->m_PropCollectionView->model()));

      QObject::connect(m_Impl->m_PropCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          ObjectModel::ClearModelFromView(m_Impl->m_PropDataView);
          m_Impl->m_CurrentEditedSheetName = PropertySheetName();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            m_Impl->m_CurrentPropIdx = *iSelected.indexes().begin();
            m_Impl->UpdateEditedProperty(m_Impl->m_CurrentPropIdx);
          }
        }
      });

      QObject::connect(m_Impl->m_PropsCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
      {
        m_Impl->m_Editor->ModifyResource();
      });

      QObject::connect(m_Impl->m_PropsCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
      {
        m_Impl->m_Editor->ModifyResource();
      });
      QObject::connect(m_Impl->m_PropsCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
      {
        m_Impl->m_Editor->ModifyResource();
      });

      propCollectionLayout->addWidget(new QLabel(QString::fromUtf8("Property"), propCollection));
      QComboBox* propertySelector = new QComboBox(propCollection);
      propCollectionLayout->addWidget(propertySelector);
      m_Impl->m_AvailablePropNames = EditorState::GetProjectProperties().GetArchetypeProperties();
      for (auto name : m_Impl->m_AvailablePropNames)
      {
        propertySelector->addItem(name.get().c_str());
      }

      propCollectionLayout->addWidget(propCollectionTool);
      propCollectionLayout->addWidget(m_Impl->m_PropCollectionView);

      propCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, propertySelector]
      {
        uint32_t curRowCount = m_Impl->m_PropsCollectionModel->rowCount(QModelIndex());
        int selName = propertySelector->currentIndex();
        if (!m_Impl->m_PropsCollectionModel->GetIndexFromName(m_Impl->m_AvailablePropNames[selName]).isValid())
        {
          if (m_Impl->m_PropsCollectionModel->insertRow(curRowCount))
          {
            QModelIndex newIndex = m_Impl->m_PropsCollectionModel->index(curRowCount, 0, QModelIndex());
            m_Impl->m_PropsCollectionModel->setData(newIndex, QString(m_Impl->m_AvailablePropNames[selName].get().c_str()), Qt::EditRole);
          }
        }
      });

      propCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
      {
        QModelIndex currentSelection = m_Impl->m_PropCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_Impl->m_PropsCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
        }
      });

      m_Impl->m_PropDataView = new QTreeView(this);
      m_Impl->m_PropDataView->setItemDelegate(new ObjectDelegate);

      dataSplitter->addWidget(propCollection);

      dataSplitter->addWidget(m_Impl->m_PropDataView);

      rootSplitter->addWidget(dataSplitter);
    }

    QWidget* componentCollection = new QWidget(this);
    QVBoxLayout* componentCollectionLayout = new QVBoxLayout(this);
    componentCollection->setLayout(componentCollectionLayout);
    QToolBar* compCollectionTool = new QToolBar(this);
    m_Impl->m_ComponentCollectionView = new QListView(this);
    m_Impl->m_ComponentCollectionView->setModel(m_Impl->m_CompCollectionModel);
    m_Impl->m_ComponentCollectionView->setSelectionModel(new QItemSelectionModel(m_Impl->m_ComponentCollectionView->model()));

    QObject::connect(m_Impl->m_ComponentCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
    {
      if (iSelected.isEmpty())
      {
        ObjectModel::ClearModelFromView(m_Impl->m_ComponentDataView);
        m_Impl->m_CurrentEditedComponentName = ComponentName();
      }
      else
      {
        if (iSelected.indexes().size() == 1)
        {
          m_Impl->m_CurrentCompIdx = *iSelected.indexes().begin();
          m_Impl->UpdateEditedComponent(m_Impl->m_CurrentCompIdx);
        }
      }
    });

    QObject::connect(m_Impl->m_CompCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
    {
      m_Impl->m_Editor->ModifyResource();
    });

    QObject::connect(m_Impl->m_CompCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });
    QObject::connect(m_Impl->m_CompCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });

    componentCollectionLayout->addWidget(new QLabel(QString::fromUtf8("Component"), componentCollection));
    QComboBox* componentSelector = new QComboBox(componentCollection);
    componentCollectionLayout->addWidget(componentSelector);
    m_Impl->m_AvailableComponentNames = EngineCommon::GetComponents().GetComponents();
    for (auto name : m_Impl->m_AvailableComponentNames)
    {
      componentSelector->addItem(name.get().c_str());
    }

    componentCollectionLayout->addWidget(compCollectionTool);
    componentCollectionLayout->addWidget(m_Impl->m_ComponentCollectionView);

    compCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, componentSelector]
    {
      uint32_t curRowCount = m_Impl->m_CompCollectionModel->rowCount(QModelIndex());
      int selName = componentSelector->currentIndex();
      if (!m_Impl->m_CompCollectionModel->GetIndexFromName(m_Impl->m_AvailableComponentNames[selName]).isValid())
      {
        if (m_Impl->m_CompCollectionModel->insertRow(curRowCount))
        {
          QModelIndex newIndex = m_Impl->m_CompCollectionModel->index(curRowCount, 0, QModelIndex());
          m_Impl->m_CompCollectionModel->setData(newIndex, QString(m_Impl->m_AvailableComponentNames[selName].get().c_str()), Qt::EditRole);
        }
      }
      m_Impl->RecreateObject();
    });

    compCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
    {
      QModelIndex currentSelection = m_Impl->m_ComponentCollectionView->selectionModel()->currentIndex();
      if (currentSelection.isValid())
      {
        m_Impl->m_CompCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
      }
      m_Impl->RecreateObject();
    });

    m_Impl->m_ComponentDataView = new QTreeView(this);
    //m_Impl->m_ComponentDataView->setItemDelegate(new ObjectDelegate);
    
    componentsSplitter->addWidget(componentCollection);
    componentsSplitter->addWidget(m_Impl->m_ComponentDataView);
		rootSplitter->addWidget(componentsSplitter);

    {
      GameWidget* gameWidget = new GameWidget(this);
      gameWidget->SetInputSystem(&m_Impl->m_Inputs);
      gameWidget->SetGfxSystem(m_Impl->m_World.GetWorld().GetSystem<GfxSystem>());

      GfxSystem::ViewInfo& view = gameWidget->GetViewInfo();
      view.pos = Vector3f::UNIT_Z * 2;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = 2.0;
      view.backgroundColor = Vector4f::ONE;

      m_Impl->m_World.GetCamera().view = view;

      auto ratioWidget = new AspectRatioWidget(gameWidget, 1.0, 1.0, this);

      gameWidget->SetTickCallback([this, gameWidget](float iDelta)
      {
        m_Impl->m_World.GetCamera().ProcessInputs(m_Impl->m_World.GetWorld(), m_Impl->m_Inputs, CameraState::WheelZoom | CameraState::RightClickPan);
        gameWidget->GetViewInfo().pos = m_Impl->m_World.GetCamera().view.pos;
        gameWidget->GetViewInfo().basis[0] = m_Impl->m_World.GetCamera().view.basis[0];
        gameWidget->GetViewInfo().basis[1] = m_Impl->m_World.GetCamera().view.basis[1];
        gameWidget->GetViewInfo().basis[2] = m_Impl->m_World.GetCamera().view.basis[2];
        gameWidget->GetViewInfo().displayedSize = m_Impl->m_World.GetCamera().view.displayedSize;

        m_Impl->m_Inputs.Clear();

        m_Impl->m_World.Tick();
      });

      gameViewSplitter->addWidget(ratioWidget);
      gameWidget->SetAnimated(true);

      rootSplitter->addWidget(gameViewSplitter);
    }

    World& world = m_Impl->m_World.GetWorld();

    m_Impl->m_ArchetypeObject = world.CreateObject();
    m_Impl->m_Archetype->Instantiate(m_Impl->m_ArchetypeObject, world, nullptr);

		QVBoxLayout* layout = new QVBoxLayout(this);

		layout->addWidget(rootSplitter);

		setLayout(layout);
	}

  void ArchetypeEditor::Impl::UpdateEditedProperty(QModelIndex iIndex)
  {
    Archetype::PropertyEntry const* obj = m_PropsCollectionModel->GetObjectFromIndex(iIndex);
    eXl_ASSERT(obj != nullptr);

    PropertySheetName const* name = m_PropsCollectionModel->GetNameFromIndex(iIndex);
    m_CurrentEditedSheetName = *name;

    m_CurrentEditedSheet = DynObject(obj->m_Data);
    bool needToCreateModel = m_ComponentDataView->model() == nullptr;
    ObjectModel::CreateOrUpdateModel(m_PropDataView, false, m_CurrentEditedSheet);
    if (needToCreateModel)
    {
      auto onSheetDataChanged = [this, iIndex]()
      {
        Archetype::PropertyEntry const* obj = m_PropsCollectionModel->GetObjectFromIndex(iIndex);
        eXl_ASSERT(obj != nullptr);

        m_Editor->ModifyResource();
        m_Archetype->SetProperty(m_CurrentEditedSheetName, m_CurrentEditedSheet, obj->m_Instanced);
      };

      QObject::connect(m_PropDataView->model(), &QAbstractItemModel::dataChanged, [onSheetDataChanged](QModelIndex const& iDataIndex)
      {
        onSheetDataChanged();
      });

      QObject::connect(m_PropDataView->model(), &QAbstractItemModel::rowsInserted, onSheetDataChanged);
      QObject::connect(m_PropDataView->model(), &QAbstractItemModel::rowsRemoved, onSheetDataChanged);
    }

  }

  void ArchetypeEditor::Impl::UpdateTileItemDelegate()
  {
    TileItemDelegate* itemDelegate = static_cast<TileItemDelegate*>(m_ComponentDataView->itemDelegate());
    if (m_CurrentEditedComponentName != EngineCommon::GfxSpriteComponentName())
    {
      itemDelegate->Clear();
    }
    else
    {
      GfxSpriteComponent::Desc* desc = m_CurrentEditedComponent.CastBuffer<GfxSpriteComponent::Desc>();

      Tileset* tileset = nullptr;
      if (desc->m_Tileset.GetUUID().IsValid())
      {
        desc->m_Tileset.Load();
        tileset = const_cast<Tileset*>(desc->m_Tileset.Get());
      }

      if (tileset)
      {
        TileCollectionModel* tiles = TileCollectionModel::Create(itemDelegate, tileset);
        itemDelegate->Setup(qobject_cast<ObjectModel*>(m_ComponentDataView->model()), tiles, m_GfxSpriteDescType, m_TilenameFieldIdx);
      }
    }
  };

  void ArchetypeEditor::Impl::UpdateEditedComponent(QModelIndex iIndex)
  {
    ComponentName const* name = m_CompCollectionModel->GetNameFromIndex(iIndex);
    eXl_ASSERT(name != nullptr);
    ConstDynObject const* obj = m_CompCollectionModel->GetObjectFromIndex(iIndex);
    eXl_ASSERT(obj != nullptr);

    m_CurrentEditedComponentName = *name;
    m_CurrentEditedComponent = DynObject(obj);
    bool needToCreateModel = m_ComponentDataView->model() == nullptr;
    ObjectModel* objectModel = ObjectModel::CreateOrUpdateModel(m_ComponentDataView, false, m_CurrentEditedComponent);
    if (needToCreateModel)
    {
      auto onComponentDataChanged = [this, iIndex]()
      {
        m_Editor->ModifyResource();

        m_Archetype->SetComponent(m_CurrentEditedComponentName, m_CurrentEditedComponent);
      };
      QObject::connect(objectModel, &QAbstractItemModel::dataChanged, [this, onComponentDataChanged](QModelIndex const& iDataIndex)
      {
        onComponentDataChanged();
        RecreateObject();
      });

      QObject::connect(objectModel, &ObjectModel::dataChanged, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &)
      {
        if (!topLeft.parent().isValid() && topLeft.row() == m_TilesetField)
        {
          UpdateTileItemDelegate();
        }
      });

      QObject::connect(objectModel, &QAbstractItemModel::rowsInserted, onComponentDataChanged);
      QObject::connect(objectModel, &QAbstractItemModel::rowsRemoved, onComponentDataChanged);
      m_ComponentDataView->setItemDelegate(new TileItemDelegate(m_ComponentDataView));
    }

    QAbstractItemDelegate* delegate = m_ComponentDataView->itemDelegate();

    if (*name == EngineCommon::GfxSpriteComponentName())
    {  
      UpdateTileItemDelegate();
    }
  }

  void ArchetypeEditor::Impl::RecreateObject()
  {
    if (m_ArchetypeObject.IsAssigned())
    {
      m_World.GetWorld().DeleteObject(m_ArchetypeObject);
    }
    
    World& world = m_World.GetWorld();
    m_ArchetypeObject = world.CreateObject();
    m_Archetype->Instantiate(m_ArchetypeObject, world, nullptr);
    world.GetSystem<Transforms>()->AddTransform(m_ArchetypeObject, nullptr);

    m_World.Tick();
  }
}