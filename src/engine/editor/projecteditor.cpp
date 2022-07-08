#include "projecteditor.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include "objectmodel.hpp"
#include "objectdelegate.hpp"

#include "collectionmodel.hpp"
#include "fundecleditor.hpp"

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
#include <QLineEdit>

namespace eXl
{
  using PropertySheetDeclCollectionModel = CollectionModelMapAdaptor<TypeName, Project::TypeDecl, Project, &Project::m_Types>;
  using ClientCallbacksCollectionModel = CollectionModelMapAdaptor<String, Project::FunctionDecl, Project, &Project::m_ClientCommands>;
  using ServerCallbacksCollectionModel = CollectionModelMapAdaptor<String, Project::FunctionDecl, Project, &Project::m_ServerCommands>;
  using ItfCollectionModel = CollectionModelMapAdaptor<String, UnorderedMap<String, Project::FunctionDecl>, Project, &Project::m_Events>;

  class EventsCollectionModel : public CollectionModel<String, Project::FunctionDecl, Project>
  {
  public:

    static EventsCollectionModel* Create(QObject* iParent, String iInterfaceName, Project* iResource)
    {
      auto iter = iResource->m_Events.find(iInterfaceName);
      if (iter == iResource->m_Events.end())
      {
        return nullptr;
      }
      EventsCollectionModel* model = new EventsCollectionModel(iParent, iInterfaceName, iResource);

      model->BuildMap(iter->second);

      return model;
    }

  protected:
    String m_InterfaceName;

    EventsCollectionModel(QObject* iParent, String iInterfaceName, Project* iResource)
      : CollectionModel(iParent, iResource)
      , m_InterfaceName(iInterfaceName)
    {

    }

    bool AddToResource(String const& iName, Project::FunctionDecl const& iValue) override
    {
      auto iter = m_Resource->m_Events.find(m_InterfaceName);
      if (iter == m_Resource->m_Events.end())
      {
        return false;
      }

      auto insertRes = iter->second.insert(std::make_pair(iName, iValue));

      return insertRes.second;
    }

    bool RemoveFromResource(String const& iName) override
    {
      auto iter = m_Resource->m_Events.find(m_InterfaceName);
      if (iter == m_Resource->m_Events.end())
      {
        return false;
      }

      auto iterEvt = iter->second.find(iName);
      if (iterEvt == iter->second.end())
      {
        return false;
      }

      iter->second.erase(iterEvt);

      return true;
    }

    Project::FunctionDecl const* FindInResource(String const& iName) const override
    {
      auto iter = m_Resource->m_Events.find(m_InterfaceName);
      if (iter == m_Resource->m_Events.end())
      {
        return nullptr;
      }

      auto iterEvt = iter->second.find(iName);
      if (iterEvt == iter->second.end())
      {
        return nullptr;
      }

      return &iterEvt->second;
    }
  };

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
    Impl(ProjectEditor* iEditor, Project* iProject);

    void AddEventEdit(QTabWidget* iWidget);

    template<typename Collection, Collection* Impl::* iCol, typename Collection::key_type Impl::* iEdited>
    QListView* SetupTypeEdit(QTabWidget* iWidget, char const* iName, TypeDeclEditor* iEditor);

    template<typename Collection, Collection* Impl::* iCol, typename Collection::key_type Impl::* iEdited>
    void AddTypeEdit(QTabWidget* iWidget, char const* iName);

    template<typename Collection, Collection* Impl::* iCol, typename Collection::key_type Impl::* iEdited>
    void AddFunDeclEdit(QTabWidget* iWidget, char const* iName);
		
    QModelIndex m_GroupSelection;

    PropertySheetDeclCollectionModel* m_PropsCollectionModel;
    ClientCallbacksCollectionModel* m_ClientCallbacksModel;
    ServerCallbacksCollectionModel* m_ServerCallbacksModel;
    ItfCollectionModel* m_ItfCollection;
    EventsCollectionModel* m_EventsCollection = nullptr;

    TypeName m_CurrentEditedSheetName;
    String m_CurrentEditedClientCBName;
    String m_CurrentEditedServerCBName;
    String m_CurrentEditedEventItfName;
    String m_CurrentEditedEventFunName;
    
    QComboBox* m_PlayerSelector;
    ResourceSelectionWidget* m_MapSelector;

    ProjectEditor* m_Editor;
    Project* m_Project;
	};

  void ProjectEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  ProjectEditor::ProjectEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl(this, Project::DynamicCast(iDoc->GetResource())))
  {
    
	}

  void ProjectEditor::Impl::AddEventEdit(QTabWidget* iTabs)
  {
    QSplitter* dataSplitter = new QSplitter(Qt::Horizontal, m_Editor);

    QWidget* itfCollection = new QWidget(m_Editor);

    QVBoxLayout* itfCollectionLayout = new QVBoxLayout(itfCollection);

    itfCollection->setLayout(itfCollectionLayout);

    QToolBar* itfCollectionTool = new QToolBar(m_Editor);
    QToolBar* evtCollectionTool = new QToolBar(m_Editor);

    QWidget* evtData = new QWidget(m_Editor);
    QVBoxLayout* evtDataLayout = new QVBoxLayout(evtData);
    evtData->setLayout(evtDataLayout);

    QListView* itfCollectionView = new QListView(m_Editor);
    QListView* evtCollectionView = new QListView(m_Editor);
    
    itfCollectionView->setModel(m_ItfCollection);
    itfCollectionView->setSelectionModel(new QItemSelectionModel(itfCollectionView->model()));

    FunDeclEditor* declEditor = FunDeclEditor::Create(m_Editor, "");

    QObject::connect(itfCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this, declEditor, evtCollectionView, itfCollectionTool, evtCollectionTool](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          declEditor->Clear();
          m_CurrentEditedEventFunName.clear();
          m_CurrentEditedEventItfName.clear();
          m_EventsCollection = nullptr;
          itfCollectionTool->setDisabled(true);
          evtCollectionTool->setDisabled(true);
          declEditor->setDisabled(true);
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            QModelIndex currentPropIdx = *iSelected.indexes().begin();
            if (auto const* decl = m_ItfCollection->GetObjectFromIndex(currentPropIdx))
            {
              m_CurrentEditedEventItfName = *m_ItfCollection->GetNameFromIndex(currentPropIdx);
              m_EventsCollection = EventsCollectionModel::Create(evtCollectionView, m_CurrentEditedEventItfName, m_Project);
              itfCollectionTool->setDisabled(false);
              evtCollectionView->setModel(m_EventsCollection);
              evtCollectionView->setSelectionModel(new QItemSelectionModel(evtCollectionView->model()));
              
              QObject::connect(evtCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this, declEditor, evtCollectionTool](const QItemSelection& iSelected, const QItemSelection& iDeselected)
                {
                  if (iSelected.isEmpty())
                  {
                    declEditor->Clear();
                    declEditor->setDisabled(true);
                    m_CurrentEditedEventFunName.clear();
                    evtCollectionTool->setDisabled(true);
                  }
                  else
                  {
                    if (iSelected.indexes().size() == 1)
                    {
                      QModelIndex currentPropIdx = *iSelected.indexes().begin();
                      if (auto const* decl = m_EventsCollection->GetObjectFromIndex(currentPropIdx))
                      {
                        m_CurrentEditedEventFunName = *m_EventsCollection->GetNameFromIndex(currentPropIdx);
                        evtCollectionTool->setDisabled(false);
                        declEditor->SetDecl(*decl);
                        declEditor->setDisabled(false);
                      }
                    }
                  }
                });

              QObject::connect(m_EventsCollection, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
                {
                  m_CurrentEditedEventFunName = *m_EventsCollection->GetNameFromIndex(iIndex);
                  m_Editor->ModifyResource();
                });

              QObject::connect(m_EventsCollection, &QAbstractItemModel::rowsInserted, [this]()
                {
                  m_Editor->ModifyResource();
                });
              QObject::connect(m_EventsCollection, &QAbstractItemModel::rowsRemoved, [this]()
                {
                  m_Editor->ModifyResource();
                });

            }
          }
        }
      });

    QObject::connect(declEditor, &TypeDeclEditor::OnDeclChange, [this, declEditor]()
      {
        auto iter = m_Project->m_Events.find(m_CurrentEditedEventItfName);
        if (iter == m_Project->m_Events.end())
        {
          return;
        }

        auto iterEvt = iter->second.find(m_CurrentEditedEventFunName);
        if (iterEvt == iter->second.end())
        {
          return;
        }

        iterEvt->second.m_Fields = declEditor->GetDecl().m_Fields;
        m_Editor->ModifyResource();
      });

    QObject::connect(declEditor, &FunDeclEditor::OnRetTypeChanged, [this, declEditor]()
      {
        auto iter = m_Project->m_Events.find(m_CurrentEditedEventItfName);
        if (iter == m_Project->m_Events.end())
        {
          return;
        }

        auto iterEvt = iter->second.find(m_CurrentEditedEventFunName);
        if (iterEvt == iter->second.end())
        {
          return;
        }

        iterEvt->second.m_Ret = declEditor->GetRetType();
        m_Editor->ModifyResource();
      });

    QObject::connect(m_ItfCollection, &QAbstractItemModel::dataChanged, [this, itfCollectionView, evtCollectionView](QModelIndex const& iIndex, QModelIndex const&)
      {
        m_CurrentEditedEventItfName = *m_ItfCollection->GetNameFromIndex(iIndex);
        m_EventsCollection = EventsCollectionModel::Create(evtCollectionView, m_CurrentEditedEventItfName, m_Project);
        evtCollectionView->setModel(m_EventsCollection);
        evtCollectionView->setSelectionModel(new QItemSelectionModel(evtCollectionView->model()));

        m_Editor->ModifyResource();
      });

    QObject::connect(m_ItfCollection, &QAbstractItemModel::rowsInserted, [this]()
      {
        m_Editor->ModifyResource();
      });
    QObject::connect(m_ItfCollection, &QAbstractItemModel::rowsRemoved, [this]()
      {
        m_Editor->ModifyResource();
      });

    itfCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, itfCollectionView]
      {
        uint32_t curRowCount = m_ItfCollection->rowCount(QModelIndex());

        if (m_ItfCollection->insertRow(curRowCount))
        {
          QModelIndex newIndex = m_ItfCollection->index(curRowCount, 0, QModelIndex());
          m_ItfCollection->setData(newIndex, QString("NewItf"), Qt::EditRole);
          m_Editor->ModifyResource();
        }
      });

    itfCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this, itfCollectionView]
      {
        QModelIndex currentSelection = itfCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_ItfCollection->removeRow(currentSelection.row(), currentSelection.parent());
          m_Editor->ModifyResource();
        }
      });

    evtCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, evtCollectionView]
      {
        if (m_EventsCollection == nullptr)
        {
          return;
        }
        uint32_t curRowCount = m_EventsCollection->rowCount(QModelIndex());

        if (m_EventsCollection->insertRow(curRowCount))
        {
          QModelIndex newIndex = m_EventsCollection->index(curRowCount, 0, QModelIndex());
          m_EventsCollection->setData(newIndex, QString("NewEvt"), Qt::EditRole);
          m_Editor->ModifyResource();
        }
      });

    evtCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this, evtCollectionView]
      {
        if (m_EventsCollection == nullptr)
        {
          return;
        }
        QModelIndex currentSelection = evtCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_EventsCollection->removeRow(currentSelection.row(), currentSelection.parent());
          m_Editor->ModifyResource();
        }
      });


    itfCollectionLayout->addWidget(itfCollectionTool);
    itfCollectionLayout->addWidget(itfCollectionView);
    itfCollectionLayout->addWidget(evtCollectionTool);
    itfCollectionLayout->addWidget(evtCollectionView);

    evtDataLayout->addWidget(declEditor);

    dataSplitter->addWidget(itfCollection);

    dataSplitter->addWidget(evtData);

    iTabs->addTab(dataSplitter, "Events");
  }

  template<typename Collection, Collection* ProjectEditor::Impl::* iColModel, typename Collection::key_type ProjectEditor::Impl::* iEdited>
  QListView* ProjectEditor::Impl::SetupTypeEdit(QTabWidget* iTabs, char const* iName, TypeDeclEditor* iEditor)
  {
    using KeyType = typename Collection::key_type;
    using ValueType = typename Collection::value_type;

    QSplitter* dataSplitter = new QSplitter(Qt::Horizontal, m_Editor);

    QWidget* propCollection = new QWidget(m_Editor);
    QVBoxLayout* propCollectionLayout = new QVBoxLayout(propCollection);
    propCollection->setLayout(propCollectionLayout);
    QWidget* propData = new QWidget(m_Editor);
    QVBoxLayout* propDataLayout = new QVBoxLayout(propData);
    propData->setLayout(propDataLayout);
    QToolBar* propCollectionTool = new QToolBar(m_Editor);
    QListView* propCollectionView = new QListView(m_Editor);
    Collection* col = (this->*iColModel);
    propCollectionView->setModel(col);
    propCollectionView->setSelectionModel(new QItemSelectionModel(propCollectionView->model()));

    TypeDeclEditor* declEditor = iEditor;

    QObject::connect(propCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this, col, declEditor](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          declEditor->Clear();
          (this->*iEdited) = KeyType();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            QModelIndex currentPropIdx = *iSelected.indexes().begin();
            if (ValueType const* decl = col->GetObjectFromIndex(currentPropIdx))
            {
              (this->*iEdited) = *col->GetNameFromIndex(currentPropIdx);
              declEditor->SetDecl(*decl);
            }
          }
        }
      });

    QObject::connect(col, &QAbstractItemModel::dataChanged, [this, col](QModelIndex const& iIndex, QModelIndex const&)
      {
        (this->*iEdited) = *col->GetNameFromIndex(iIndex);
        m_Editor->ModifyResource();
      });

    QObject::connect(col, &QAbstractItemModel::rowsInserted, [this]()
      {
        m_Editor->ModifyResource();
      });
    QObject::connect(col, &QAbstractItemModel::rowsRemoved, [this]()
      {
        m_Editor->ModifyResource();
      });

    propCollectionLayout->addWidget(propCollectionTool);
    propCollectionLayout->addWidget(propCollectionView);

    propCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, col, propCollectionView]
      {
        uint32_t curRowCount = col->rowCount(QModelIndex());

        if (col->insertRow(curRowCount))
        {
          QModelIndex newIndex = col->index(curRowCount, 0, QModelIndex());
          col->setData(newIndex, QString("NewDecl"), Qt::EditRole);
          m_Editor->ModifyResource();
        }
      });

    propCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this, col, propCollectionView]
      {
        QModelIndex currentSelection = propCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          col->removeRow(currentSelection.row(), currentSelection.parent());
          m_Editor->ModifyResource();
        }
      });

    propDataLayout->addWidget(declEditor);

    dataSplitter->addWidget(propCollection);

    dataSplitter->addWidget(propData);

    iTabs->addTab(dataSplitter, iName);

    return propCollectionView;
  }

  template<typename Collection, Collection* ProjectEditor::Impl::* iCol, typename Collection::key_type ProjectEditor::Impl::* iEdited>
  void ProjectEditor::Impl::AddTypeEdit(QTabWidget* iWidget, char const* iName)
  {
    TypeDeclEditor* declEditor = TypeDeclEditor::Create(m_Editor);
    Collection* col = (this->*iCol);

    SetupTypeEdit<Collection, iCol, iEdited>(iWidget, iName, declEditor);

    QObject::connect(declEditor, &TypeDeclEditor::OnDeclChange, [this, col, declEditor]()
      {
        col->SetOnResource((this->*iEdited), declEditor->GetDecl());
        m_Editor->ModifyResource();
      });

  }

  template<typename Collection, Collection* ProjectEditor::Impl::* iCol, typename Collection::key_type ProjectEditor::Impl::* iEdited>
  void ProjectEditor::Impl::AddFunDeclEdit(QTabWidget* iWidget, char const* iName)
  {
    FunDeclEditor* declEditor = FunDeclEditor::Create(m_Editor, "");
    Collection* col = (this->*iCol);

    QListView* propCollectionView = SetupTypeEdit<Collection, iCol, iEdited>(iWidget, iName, declEditor);

    QObject::connect(declEditor, &TypeDeclEditor::OnDeclChange, [this, col, declEditor]()
      {
        Project::FunctionDecl decl;
        decl.m_Fields = declEditor->GetDecl().m_Fields;
        decl.m_Ret = declEditor->GetRetType();
        col->SetOnResource((this->*iEdited), std::move(decl));
        m_Editor->ModifyResource();
      });

    QObject::connect(declEditor, &FunDeclEditor::OnRetTypeChanged, [this, col, declEditor]()
      {
        Project::FunctionDecl decl;
        decl.m_Fields = declEditor->GetDecl().m_Fields;
        decl.m_Ret = declEditor->GetRetType();
        col->SetOnResource((this->*iEdited), std::move(decl));
        m_Editor->ModifyResource();
      });

    QObject::connect(propCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this, col, declEditor](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          declEditor->Clear();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            QModelIndex currentPropIdx = *iSelected.indexes().begin();
            if (Project::FunctionDecl const* decl = col->GetObjectFromIndex(currentPropIdx))
            {
              (this->*iEdited) = *col->GetNameFromIndex(currentPropIdx);
              declEditor->SetRetType(decl->m_Ret);
            }
          }
        }
      });
  }

  ProjectEditor::Impl::Impl(ProjectEditor* iEditor, Project* iProject)
    : m_Editor(iEditor)
    , m_Project(iProject)
  {
    m_PropsCollectionModel = PropertySheetDeclCollectionModel::Create(m_Editor, m_Project);
    m_ClientCallbacksModel = ClientCallbacksCollectionModel::Create(iEditor, m_Project);
    m_ServerCallbacksModel = ServerCallbacksCollectionModel::Create(iEditor, m_Project);
    m_ItfCollection = ItfCollectionModel::Create(iEditor, m_Project);

    QTabWidget* tabs = new QTabWidget(m_Editor);

    AddTypeEdit<PropertySheetDeclCollectionModel, &Impl::m_PropsCollectionModel, &Impl::m_CurrentEditedSheetName>(tabs, "Types");
    AddFunDeclEdit<ClientCallbacksCollectionModel, &Impl::m_ClientCallbacksModel, &Impl::m_CurrentEditedClientCBName>(tabs, "Client RPC");
    AddFunDeclEdit<ServerCallbacksCollectionModel, &Impl::m_ServerCallbacksModel, &Impl::m_CurrentEditedServerCBName>(tabs, "Server RPC");
    AddEventEdit(tabs);

    QWidget* projectSettings = new QWidget(m_Editor);
    QVBoxLayout* settingsLayout = new QVBoxLayout(projectSettings);
    projectSettings->setLayout(settingsLayout);

    {
      QWidget* gameDllWidget = new QWidget(projectSettings);
      QHBoxLayout* gameDllLayout = new QHBoxLayout(gameDllWidget);
      gameDllWidget->setLayout(gameDllLayout);
      settingsLayout->addWidget(gameDllWidget);

      gameDllLayout->addWidget(new QLabel("Game Dll (restart if changed) : "));
      QLineEdit* gameDllInput = new QLineEdit(projectSettings);
      gameDllInput->setText(m_Project->m_GameDll.c_str());

      QObject::connect(gameDllInput, &QLineEdit::editingFinished, [this, gameDllInput]()
        {
          m_Project->m_GameDll = gameDllInput->text().toStdString();
          m_Editor->ModifyResource();
        });
      gameDllLayout->addWidget(gameDllInput);
    }

    
    {
      QWidget* playerParamsWidget = new QWidget(projectSettings);
      QHBoxLayout* playerParamsLayout = new QHBoxLayout(playerParamsWidget);
      playerParamsWidget->setLayout(playerParamsLayout);
      settingsLayout->addWidget(playerParamsWidget);

      playerParamsLayout->addWidget(new QLabel("Player additional parameters "));

      QLineEdit* playerParams = new QLineEdit(projectSettings);
      playerParams->setText(m_Project->m_PlayerAdditionalParameters.c_str());

      QObject::connect(playerParams, &QLineEdit::editingFinished, [this, playerParams]()
        {
          m_Project->m_PlayerAdditionalParameters = playerParams->text().toStdString();
          m_Editor->ModifyResource();
        });
      playerParamsLayout->addWidget(playerParams);
    }

    QWidget* playerSelWidget = new QWidget(m_Editor);

    QHBoxLayout* playerSelLayout = new QHBoxLayout(playerSelWidget);
    playerSelWidget->setLayout(playerSelLayout);
    settingsLayout->addWidget(playerSelWidget);

    m_PlayerSelector = new QComboBox(playerSelWidget);
    playerSelLayout->addWidget(new QLabel("Player Archetype : "));
    playerSelLayout->addWidget(m_PlayerSelector);

    auto* archetypesModel = EditorState::GetState()->GetProjectResourcesModel()->MakeFilteredModel(m_PlayerSelector, Archetype::StaticLoaderName(), true);
    m_PlayerSelector->setModel(archetypesModel);

    {
      Resource::UUID const& archetypeUUID = m_Project->m_PlayerArchetype.GetUUID();
      if (archetypeUUID.IsValid())
      {
        QModelIndex index = archetypesModel->GetIndexFromUUID(archetypeUUID);
        if (index.isValid())
        {
          m_PlayerSelector->setCurrentIndex(index.row());
        }
      }
    }

    QObject::connect(m_PlayerSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, archetypesModel](int iIndex)
      {
        Resource::UUID const* resourceId = archetypesModel->GetResourceIDFromIndex(archetypesModel->index(iIndex, 0, QModelIndex()));

        if (resourceId != nullptr)
        {
          if (*resourceId != m_Project->m_PlayerArchetype.GetUUID())
          {
            m_Project->m_PlayerArchetype.SetUUID(*resourceId);
            m_Editor->ModifyResource();
          }
        }
        else if (m_Project->m_PlayerArchetype.GetUUID().IsValid())
        {
          m_Project->m_PlayerArchetype.SetUUID(Resource::UUID());
          m_Editor->ModifyResource();
        }
      });

    QWidget* mapSelWidget = new QWidget(m_Editor);
    QHBoxLayout* defaultMapSelLayout = new QHBoxLayout(mapSelWidget);
    mapSelWidget->setLayout(defaultMapSelLayout);
    settingsLayout->addWidget(mapSelWidget);

    m_MapSelector = new ResourceSelectionWidget(playerSelWidget, MapResource::StaticLoaderName(), ResourceSelectionWidget::Combo);
    defaultMapSelLayout->addWidget(new QLabel("Default map : "));
    defaultMapSelLayout->addWidget(m_MapSelector);
    {
      Resource::UUID const& mapUUID = m_Project->m_PlayerArchetype.GetUUID();
      m_MapSelector->ForceSelection(mapUUID);
    }

    QObject::connect(m_MapSelector, &ResourceSelectionWidget::onResourceChanged, [this, archetypesModel]()
      {
        Resource::UUID const& resourceId = m_MapSelector->GetSelectedResourceId();

        if (resourceId != m_Project->m_StartupMap.GetUUID())
        {
          m_Project->m_StartupMap.SetUUID(resourceId);
          m_Editor->ModifyResource();
        }
      });

    tabs->addTab(projectSettings, "Settings");

    QVBoxLayout* layout = new QVBoxLayout(m_Editor);
    layout->addWidget(tabs);
    m_Editor->setLayout(layout);
  }
}