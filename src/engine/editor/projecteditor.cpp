#include "projecteditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include "objectmodel.hpp"
#include "objectdelegate.hpp"

#include "collectionmodel.hpp"

#include <core/type/typemanager.hpp>

#include <engine/common/project.hpp>
#include <engine/map/map.hpp>
#include <engine/game/archetype.hpp>
#include <editor/resourceselectionwidget.hpp>

#include <QTabWidget>
#include <QTableWidget>
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
  class PropertySheetDeclCollectionModel : public CollectionModel<TypeName, Project::Typedecl, Project>
  {
    //Q_OBJECT
  public:
    static PropertySheetDeclCollectionModel* Create(QObject* iParent, Project* iProject);

  protected:
    PropertySheetDeclCollectionModel(QObject* iParent, Project* iGroup);
    bool AddToResource(TypeName const& iName, Project::Typedecl const&) override;
    bool RemoveFromResource(TypeName const& iName) override;
    Project::Typedecl const* FindInResource(TypeName const& iName) const override;

  };

  PropertySheetDeclCollectionModel* PropertySheetDeclCollectionModel::Create(QObject* iParent, Project* iProject)
  {
    PropertySheetDeclCollectionModel* newModel = new PropertySheetDeclCollectionModel(iParent, iProject);
    newModel->BuildMap(iProject->m_Types);

    return newModel;
  }

  PropertySheetDeclCollectionModel::PropertySheetDeclCollectionModel(QObject* iParent, Project* iArchetype)
    : CollectionModel(iParent, iArchetype)
  {
  }

  bool PropertySheetDeclCollectionModel::AddToResource(TypeName const& iName, Project::Typedecl const& iValue)
  {
    auto insertRes = m_Resource->m_Types.insert(std::make_pair(iName, iValue));
    
    return insertRes.second;
  }

  bool PropertySheetDeclCollectionModel::RemoveFromResource(TypeName const& iName)
  {
    m_Resource->m_Types.erase(iName);
    return true;
  }

  Project::Typedecl const* PropertySheetDeclCollectionModel::FindInResource(TypeName const& iName) const
  {
    auto iter = m_Resource->m_Types.find(iName);
    if (iter != m_Resource->m_Types.end())
    {
      return &iter->second;
    }
    return nullptr;
  }

  class ProjectEditorHandler : public ResourceEditorHandler
  {
  public:

    ProjectEditorHandler()
      : ResourceEditorHandler(Project::StaticLoaderName())
    {}

    DocumentState* CreateNewDocument() override
    {
      return nullptr;
    }

    ResourceEditor* CreateEditor(QWidget* iParent, DocumentState* iDoc) override
    {
      return new ProjectEditor(iParent, iDoc);
    }
  };
  ResourceEditorHandler& ProjectEditor::GetEditorHandler()
  {
    static ProjectEditorHandler s_Handler;
    return s_Handler;
  }

	struct ProjectEditor::Impl
	{
		Impl()
		{
		}
		
    QModelIndex m_GroupSelection;

    PropertySheetDeclCollectionModel* m_PropsCollectionModel;

    QTableWidget* m_PropDataView;
    QMetaObject::Connection m_PropDataChangeCb;
    QListView* m_PropCollectionView;

    QModelIndex m_CurrentPropIdx;

    TypeName m_CurrentEditedSheetName;
    Project::Typedecl m_CurrentEditedType;
    
    QComboBox* m_PlayerSelector;
    ResourceSelectionWidget* m_MapSelector;

    ProjectEditor* m_Editor;
    Project* m_Project;
    QStringList m_TypeNames;

    
    void UpdateEditedProperty(QModelIndex);
	};

  void ProjectEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  ProjectEditor::ProjectEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl)
  {
    m_Impl->m_Editor = this;
    m_Impl->m_Project = Project::DynamicCast(iDoc->GetResource());

    m_Impl->m_PropsCollectionModel = PropertySheetDeclCollectionModel::Create(this, m_Impl->m_Project);

    QSplitter* rootSplitter = new QSplitter(Qt::Vertical, this);

    QWidget* projectSettings = new QWidget(this);
    QVBoxLayout* settingsLayout = new QVBoxLayout(projectSettings);
    projectSettings->setLayout(settingsLayout);

    QWidget* playerSelWidget = new QWidget(this);

    QHBoxLayout* playerSelLayout = new QHBoxLayout(playerSelWidget);
    playerSelWidget->setLayout(playerSelLayout);
    settingsLayout->addWidget(playerSelWidget);

    m_Impl->m_PlayerSelector = new QComboBox(playerSelWidget);
    playerSelLayout->addWidget(new QLabel("Player Archetype : "));
    playerSelLayout->addWidget(m_Impl->m_PlayerSelector);
    
    auto* archetypesModel = EditorState::GetState()->GetProjectResourcesModel()->MakeFilteredModel(m_Impl->m_PlayerSelector, Archetype::StaticLoaderName(), true);
    m_Impl->m_PlayerSelector->setModel(archetypesModel);

    {
      Resource::UUID const& archetypeUUID = m_Impl->m_Project->m_PlayerArchetype.GetUUID();
      if (archetypeUUID.IsValid())
      {
        QModelIndex index = archetypesModel->GetIndexFromUUID(archetypeUUID);
        if (index.isValid())
        {
          m_Impl->m_PlayerSelector->setCurrentIndex(index.row());
        }
      }
    }

    QObject::connect(m_Impl->m_PlayerSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, archetypesModel](int iIndex)
      {
        Resource::UUID const* resourceId = archetypesModel->GetResourceIDFromIndex(archetypesModel->index(iIndex, 0, QModelIndex()));

        if (resourceId != nullptr)
        {
          if (*resourceId != m_Impl->m_Project->m_PlayerArchetype.GetUUID())
          {
            m_Impl->m_Project->m_PlayerArchetype.SetUUID(*resourceId);
            ModifyResource();
          }
        }
        else if(m_Impl->m_Project->m_PlayerArchetype.GetUUID().IsValid())
        {
          m_Impl->m_Project->m_PlayerArchetype.SetUUID(Resource::UUID());
          ModifyResource();
        }
      });

    QWidget* mapSelWidget = new QWidget(this);
    QHBoxLayout* defaultMapSelLayout = new QHBoxLayout(mapSelWidget);
    mapSelWidget->setLayout(defaultMapSelLayout);
    settingsLayout->addWidget(mapSelWidget);

    m_Impl->m_MapSelector = new ResourceSelectionWidget(playerSelWidget, MapResource::StaticLoaderName(), ResourceSelectionWidget::Combo);
    defaultMapSelLayout->addWidget(new QLabel("Default map : "));
    defaultMapSelLayout->addWidget(m_Impl->m_MapSelector);
    {
      Resource::UUID const& mapUUID = m_Impl->m_Project->m_PlayerArchetype.GetUUID();
      m_Impl->m_MapSelector->ForceSelection(mapUUID);
    }

    QObject::connect(m_Impl->m_MapSelector, &ResourceSelectionWidget::onResourceChanged, [this, archetypesModel]()
      {
        Resource::UUID const& resourceId = m_Impl->m_MapSelector->GetSelectedResourceId();

        if (resourceId != m_Impl->m_Project->m_StartupMap.GetUUID())
        {
          m_Impl->m_Project->m_StartupMap.SetUUID(resourceId);
          ModifyResource();
        }
      });

    rootSplitter->addWidget(projectSettings);

    QSplitter* dataSplitter = new QSplitter(Qt::Horizontal, this);
    {
      QWidget* propCollection = new QWidget(this);
      QVBoxLayout* propCollectionLayout = new QVBoxLayout(this);
      propCollection->setLayout(propCollectionLayout);
      QWidget* propData = new QWidget(this);
      QVBoxLayout* propDataLayout = new QVBoxLayout(this);
      propData->setLayout(propDataLayout);
      QToolBar* propCollectionTool = new QToolBar(this);
      m_Impl->m_PropCollectionView = new QListView(this);
      m_Impl->m_PropCollectionView->setModel(m_Impl->m_PropsCollectionModel);
      m_Impl->m_PropCollectionView->setSelectionModel(new QItemSelectionModel(m_Impl->m_PropCollectionView->model()));

      QObject::connect(m_Impl->m_PropCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_Impl->m_PropDataView->clear();
          m_Impl->m_CurrentPropIdx = QModelIndex();
          m_Impl->m_CurrentEditedSheetName = TypeName();
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
        m_Impl->m_CurrentEditedSheetName = *m_Impl->m_PropsCollectionModel->GetNameFromIndex(iIndex);
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

      Vector<Type const*> types = TypeManager::GetCoreTypes();
      for (auto type : types)
      {
        m_Impl->m_TypeNames.append(QString::fromUtf8(type->GetName().c_str()));
      }

      propCollectionLayout->addWidget(propCollectionTool);
      propCollectionLayout->addWidget(m_Impl->m_PropCollectionView);

      propCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this]
      {
        uint32_t curRowCount = m_Impl->m_PropsCollectionModel->rowCount(QModelIndex());
        
        if (m_Impl->m_PropsCollectionModel->insertRow(curRowCount))
        {
          QModelIndex newIndex = m_Impl->m_PropsCollectionModel->index(curRowCount, 0, QModelIndex());
          m_Impl->m_PropsCollectionModel->setData(newIndex, QString("NewProperty"), Qt::EditRole);
          m_Impl->m_Editor->ModifyResource();
        }
      });

      propCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
      {
        QModelIndex currentSelection = m_Impl->m_PropCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_Impl->m_PropsCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
          m_Impl->m_Editor->ModifyResource();
        }
      });

      m_Impl->m_PropDataView = new QTableWidget(this);
      
      QToolBar* propDataTool = new QToolBar(this);

      propDataTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Field", [this]
      {
        Project::Field newField;
        newField.m_Name = "NewField";
        newField.m_TypeName = "uint32_t";
        newField.m_IsArray = false;
        m_Impl->m_CurrentEditedType.m_Fields.push_back(newField);
        m_Impl->m_Project->m_Types[m_Impl->m_CurrentEditedSheetName] = m_Impl->m_CurrentEditedType;
        m_Impl->UpdateEditedProperty(m_Impl->m_CurrentPropIdx);
        m_Impl->m_Editor->ModifyResource();
      });

      propDataTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Field", [this]
      {
        QTableWidgetItem* currentSelection = m_Impl->m_PropDataView->currentItem();
        if (currentSelection != nullptr)
        {
          int32_t row = currentSelection->row();
          m_Impl->m_CurrentEditedType.m_Fields.erase(m_Impl->m_CurrentEditedType.m_Fields.begin() + row);
          m_Impl->m_Project->m_Types[m_Impl->m_CurrentEditedSheetName] = m_Impl->m_CurrentEditedType;
          m_Impl->UpdateEditedProperty(m_Impl->m_CurrentPropIdx);
          m_Impl->m_Editor->ModifyResource();
        }
      });

      propDataLayout->addWidget(propDataTool);
      propDataLayout->addWidget(m_Impl->m_PropDataView);

      dataSplitter->addWidget(propCollection);

      dataSplitter->addWidget(propData);

      rootSplitter->addWidget(dataSplitter);
    }

		QVBoxLayout* layout = new QVBoxLayout(this);

		layout->addWidget(rootSplitter);

		setLayout(layout);
	}

  void ProjectEditor::Impl::UpdateEditedProperty(QModelIndex iIndex)
  {
    Project::Typedecl const* obj = m_PropsCollectionModel->GetObjectFromIndex(iIndex);
    eXl_ASSERT(obj != nullptr);

    TypeName const* name = m_PropsCollectionModel->GetNameFromIndex(iIndex);
    m_CurrentEditedSheetName = *name;
    m_CurrentEditedType = *obj;

    QObject::disconnect(m_PropDataChangeCb);
    m_PropDataView->clear();
    m_PropDataView->setRowCount(obj->m_Fields.size());
    m_PropDataView->setColumnCount(3);

    auto onSheetDataChanged = [this](QModelIndex const& iIndex)
    {
      QTableWidgetItem* item = m_PropDataView->item(iIndex.row(), iIndex.column());
      switch (iIndex.column())
      {
      case 0:
      {
        //TypeFieldName oldFieldName = item->data(Qt::UserRole).toString().toUtf8();
        TypeFieldName newFieldName = item->text().toUtf8();
        Project::Field& fieldData = m_CurrentEditedType.m_Fields[iIndex.row()];
        fieldData.m_Name = newFieldName;
      }
      break;
      case 1:
      {
        QComboBox* typeSelector = static_cast<QComboBox*>(m_PropDataView->cellWidget(iIndex.row(), iIndex.column()));
        m_CurrentEditedType.m_Fields[iIndex.row()].m_TypeName = m_TypeNames[typeSelector->currentIndex()].toUtf8();
      }
      break;
      case 2:
      {
        TypeFieldName fieldName = item->data(Qt::UserRole).toString().toUtf8();
        m_CurrentEditedType.m_Fields[iIndex.row()].m_IsArray = item->checkState() == Qt::Checked;
      }
      break;
      }

      m_Project->m_Types[m_CurrentEditedSheetName] = m_CurrentEditedType;
      m_Editor->ModifyResource();
    };
    
    
    for (uint32_t numRow = 0; numRow < obj->m_Fields.size(); ++numRow)
    {
      auto const& field = obj->m_Fields[numRow];
      QTableWidgetItem* nameItem = new QTableWidgetItem;
      QString fieldName = QString::fromUtf8(field.m_Name.c_str());
      nameItem->setData(Qt::UserRole, QVariant(fieldName));
      nameItem->setText(fieldName);
      m_PropDataView->setItem(numRow, 0, nameItem);
      
      QString typeNameStr = QString::fromUtf8(field.m_TypeName.c_str());

      bool foundType = false;
      int32_t selType = 0;
      QComboBox* typeCombo = new QComboBox(m_PropDataView);
      for (auto& type : m_TypeNames)
      {
        typeCombo->addItem(type);
        if (type == typeNameStr)
        {
          foundType = true;
        }
        if (!foundType)
        {
          ++selType;
        }
      }
      if (selType < m_TypeNames.size())
      {
        typeCombo->setCurrentIndex(selType);
      }
      m_PropDataView->setCellWidget(numRow, 1, typeCombo);
      QObject::connect(typeCombo, (void (QComboBox::*)(int ))&QComboBox::currentIndexChanged, 
        [onSheetDataChanged, index = m_PropDataView->model()->index(numRow, 1)](int)
        { onSheetDataChanged(index); });

      QTableWidgetItem* isArrayItem = new QTableWidgetItem;
      isArrayItem->setData(Qt::UserRole, QVariant(fieldName));
      isArrayItem->setCheckState(field.m_IsArray ? Qt::Checked : Qt::Unchecked);
      m_PropDataView->setItem(numRow, 2, isArrayItem);
    }
    
    m_PropDataChangeCb = QObject::connect(m_PropDataView->model(), &QAbstractItemModel::dataChanged, [onSheetDataChanged](QModelIndex const& iDataIndex)
    {
      onSheetDataChanged(iDataIndex);
    });

    //QObject::connect(m_Impl->m_PropDataView->model(), &QAbstractItemModel::rowsInserted, onSheetDataChanged);
    //QObject::connect(m_Impl->m_PropDataView->model(), &QAbstractItemModel::rowsRemoved, onSheetDataChanged);
  }
}