#include "editorstate.hpp"

#include <core/application.hpp>
#include <core/resource/resource.hpp>
#include <core/resource/resourcemanager.hpp>

#include <engine/common/project.hpp>
#include <engine/game/commondef.hpp>

#include "mainwindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QMdiSubWindow>

namespace eXl
{
  IMPLEMENT_RefC(DocumentState)

  DocumentState::DocumentState(Resource& iRsc, bool iSaved)
    : m_Resource(iRsc)
    , m_Saved(iSaved)
  {
    m_Handle.Set(&iRsc);
  }
  
  DocumentState::~DocumentState()
  {
    
  }
  
  String const& DocumentState::GetName() const
  {
    return m_Resource.GetName();
  }
  
  bool DocumentState::IsSaved()
  {
    return m_Saved;
  }
  
  void DocumentState::Touch()
  {
    m_Saved = false;
  }
  
  namespace
  {
    EditorState*            s_State = nullptr;
    MainWindow*             s_MainWindow = nullptr;
    DocumentState*          s_CurrentProject = nullptr;
    ProjectResourcesModel*  s_ProjectResources = nullptr;
    Path                    s_ProjectPath;
    Project::ProjectTypes   s_Types;
    PropertiesManifest      s_Properties;

    UnorderedMap<Resource::UUID, IntrusivePtr<DocumentState>> s_OpenedDocuments;
    UnorderedMap<ResourceLoaderName, ResourceEditorHandler*> s_ResourHandlers;
    ResourceEditor* s_CurrentActiveEditor = nullptr;
  }

  bool DocumentState::Save()
  {
    if (ResourceEditor* editor = GetEditor())
    {
      editor->CommitDocument();
    }

    if (this == s_CurrentProject)
    {
      m_Saved = EditorState::SaveProject();
    }
    else
    {
      if (ResourceManager::Save(&m_Resource))
      {
        m_Saved = true;
      }
    }
    return m_Saved;
  }

  EditorState* EditorState::GetState()
  {
    return s_State;
  }

  PropertiesManifest const& EditorState::GetProjectProperties()
  {
    return s_Properties;
  }

  void EditorState::BuildState(MainWindow* iWin)
  {
    s_State = new EditorState;
    s_MainWindow = iWin;
  }

  QSettings& EditorState::GetSettings()
  {
    static QSettings s_Settings("Dunatk_Editor", "eXl Corp");

    return s_Settings;
  }

  void EditorState::RestoreProject()
  {
    QVariant pathObject = GetSettings().value("projectPath");
    if (pathObject.isValid())
    {
      OpenProject(Path(pathObject.toString().toStdString()));
    }
  }

  DocumentState* EditorState::GetCurrentProject()
  {
    return s_CurrentProject;
  }

  void UpdateProjectTypes()
  {
    s_Properties = DunAtk::GetBaseProperties();
    static_cast<Project*>(s_CurrentProject->GetResource())->FillProperties(s_Types, s_Properties);

    ResourceManager::RemoveManifest(PropertiesManifest::StaticRtti());
    ResourceManager::RemoveManifest(ComponentManifest::StaticRtti());
    ResourceManager::AddManifest(DunAtk::GetComponents());
    ResourceManager::AddManifest(s_Properties);
  }

  void OnProjectOpened(Project& iProject)
  {
    DocumentState* newState = new DocumentState(iProject, true);

    s_CurrentProject = newState;

    Path sanitizedPath = ResourceManager::GetPath(iProject.GetHeader().m_ResourceId);
    s_ProjectPath = sanitizedPath.parent_path();
    ResourceManager::BootstrapDirectory(s_ProjectPath, true);
    s_ProjectResources = new ProjectResourcesModel;
    s_OpenedDocuments.insert(std::make_pair(iProject.GetHeader().m_ResourceId, s_CurrentProject));

    emit EditorState::GetState()->projectOpened();

    EditorState::GetSettings().setValue("projectPath", sanitizedPath.string().c_str());
    UpdateProjectTypes();
  }
  
  DocumentState* EditorState::CreateProject(Path const& iDir, String const& iName)
  {
    if(s_CurrentProject == nullptr 
      && ! iName.empty() 
      && Filesystem::exists(iDir) 
      && Filesystem::is_directory(iDir))
    {
      Path filePath = iDir / iName.c_str();
      if (Filesystem::exists(filePath))
      {
        return nullptr;
      }

      Project* newProject = Project::Create(iName);

      if (ResourceManager::SaveTo(newProject, filePath))
      {
        OnProjectOpened(*newProject);
        return s_CurrentProject;
      }
    }
    return nullptr;
  }

  DocumentState* EditorState::OpenProject(Path const& iPath)
  {
    if (s_CurrentProject == nullptr)
    {
      Resource* project = ResourceManager::LoadExpectedType(iPath, Project::StaticLoaderName());
      if (project)
      {
        Project* openedProject = static_cast<Project*>(project);
        OnProjectOpened(*openedProject);

        return s_CurrentProject;
      }
    }
    return nullptr;
  }

  ResourceEditor* DocumentState::OpenEditor(QWidget* iParent)
  {
    EditorState::OpenResourceEditor(this, iParent);
    return m_Editor;
  }

  bool ResourceEditorHandler::IsInProjectFolder(Path const& iPath)
  {
    bool containedInProject = false;
    Path directory = Filesystem::canonical(iPath.parent_path());
    while (directory.has_parent_path())
    {
      if (directory == s_ProjectPath)
      {
        return true;
      }
      directory = directory.parent_path();
    }
    return false;
  }

  DocumentState* ResourceEditorHandler::CreateNewDocument()
  {
    ResourceLoader* loader = ResourceManager::GetLoader(m_Loader);
    eXl_ASSERT(loader != nullptr);
    eXl_ASSERT(loader->CanCreateDefaultResource());

    QString resourceLoaderName = m_Loader.get().c_str();
    QString title("New ");
    title.append(resourceLoaderName);

    QString file = QFileDialog::getSaveFileName(nullptr, title,
      QString::fromStdString(EditorState::GetProjectDirectory().string()),
      "Resource file (*.eXlAsset)");

    Path newResourcePath(file.toStdString());
    if (Filesystem::exists(newResourcePath))
    {
      LOG_ERROR << "Cannot create new resource over already exising one" << "\n";
      return nullptr;
    }

    if (!IsInProjectFolder(newResourcePath))
    {
      LOG_ERROR << "Path " << newResourcePath.string() << " is not contained in the project folder" << "\n";
      return nullptr;
    }

    Path fileName = newResourcePath.filename();
    fileName.replace_extension();

    String resourceName(fileName.string().c_str());
    Resource* newResource = loader->Create(resourceName);
    DocumentState* document = new DocumentState(*newResource);

    return document;
  }

  bool EditorState::SaveProject()
  {
    bool ret = false;
    if (s_CurrentProject != nullptr && !s_CurrentProject->IsSaved())
    {
      Vector<DocumentState*> openedDocs;
      for (auto const& entry : s_OpenedDocuments)
      {
        openedDocs.push_back(entry.second.get());
      }
      for (auto doc : openedDocs)
      {
        if (doc != s_CurrentProject)
        {
          if (!AttemptToCloseEditor(doc))
          {
            return false;
          }
        }
      }
      openedDocs.clear();

      ResourceManager::UnloadUnusedResources();

      ret = (!!ResourceManager::Save(s_CurrentProject->GetResource()));
      UpdateProjectTypes();
    }
    else
    {
      ret = true;
    }

    if(ret)
    {
      emit GetState()->projectSaved();
    }
  
    return ret;
  }
  
  bool EditorState::CloseProject()
  {
    bool ret = false;
    if(s_CurrentProject != nullptr)
    {
      DocumentState* docState = GetCurrentProject();
      if(docState != nullptr && !docState->IsSaved())
      {
        /*A mettre dans la mainwindow ?*/
        QMessageBox msgBox(s_MainWindow);
        msgBox.setText("Project was modified, exit anyway?");
        msgBox.addButton(QMessageBox::Save);
        msgBox.addButton(QMessageBox::Discard);
        msgBox.addButton(QMessageBox::Cancel);
  
        int result = msgBox.exec();
        switch(result)
        {
        case QMessageBox::Save:
          ret = SaveProject();
          break;
        case QMessageBox::Discard:
          ret = true;
          break;
        case QMessageBox::Cancel:
          break;
        }
      }
      else
      {
        ret = true;
      }
    }
    else
    {
      ret = true;
    }
  
    if(ret)
    {
      emit GetState()->projectClosed();

      delete s_ProjectResources;
      s_ProjectResources = nullptr;
      delete s_CurrentProject;
      s_CurrentProject = nullptr;
      ResourceManager::UnloadUnusedResources();
      ResourceManager::Reset();
    }
  
    return ret;
  }

  ProjectResourcesModel* EditorState::GetProjectResourcesModel()
  {
    return s_ProjectResources;
  }

  Path const& EditorState::GetProjectDirectory()
  {
    static Path s_emptyPath;
    if (s_CurrentProject)
    {
      return s_ProjectPath;
    }

    return s_emptyPath;
  }

  void EditorState::AddResourceHandler(ResourceEditorHandler* iHandler)
  {
    if (iHandler)
    {
      s_ResourHandlers.insert(std::make_pair(iHandler->GetLoaderName(), iHandler));
    }
  }

  Vector<ResourceLoaderName> EditorState::GetAllRegisteredHandlers()
  {
    Vector<ResourceLoaderName> handlers;
    for (auto const& handler : s_ResourHandlers)
    {
      handlers.push_back(handler.first);
    }

    return handlers;
  }

  DocumentState* EditorState::CreateResource(ResourceLoaderName iName, QWidget* iParent)
  {
    auto iter = s_ResourHandlers.find(iName);
    if (iter == s_ResourHandlers.end())
    {
      LOG_ERROR << "Unknown resource kind : " << iName.get() << "\n";
      return nullptr;
    }

    ResourceEditorHandler* handler = iter->second;
    DocumentState* newDoc = handler->CreateNewDocument();
    if (newDoc)
    {
      s_OpenedDocuments.insert(std::make_pair(newDoc->GetResource()->GetHeader().m_ResourceId, newDoc));
      s_ProjectResources->OnResourceCreated(newDoc->GetResource());

      if (iParent)
      {
        OpenResourceEditor(newDoc, iParent);
      }
    }

    return newDoc;
  }

  ResourceEditor* EditorState::OpenResourceEditor(Resource::UUID const& iUUID, QWidget* iParent)
  {
    auto openedDocIter = s_OpenedDocuments.find(iUUID);

    if (openedDocIter == s_OpenedDocuments.end())
    {
      Resource* resource = ResourceManager::Load(iUUID);
      if (!resource)
      {
        LOG_ERROR << "Unknown UUID"
          << iUUID.uuid_dwords[0] << "-"
          << iUUID.uuid_dwords[1] << "-"
          << iUUID.uuid_dwords[2] << "-"
          << iUUID.uuid_dwords[3] << "\n";

        return nullptr;
      }
      DocumentState* newDoc = new DocumentState(*resource, true);
      openedDocIter = s_OpenedDocuments.insert(std::make_pair(iUUID, newDoc)).first;
    }

    DocumentState* openedDoc = openedDocIter->second.get();

    return OpenResourceEditor(openedDoc, iParent);
  }

  ResourceEditor* EditorState::OpenResourceEditor(DocumentState* iDoc, QWidget* iParent)
  {
    if (auto editor = iDoc->GetEditor())
    {
      return editor;
    }

    Resource* rsc = iDoc->GetResource();
    ResourceLoaderName loader = rsc->GetHeader().m_LoaderName;

    auto handlerIter = s_ResourHandlers.find(loader);

    if (handlerIter == s_ResourHandlers.end())
    {
      LOG_ERROR << "Unknown resource kind : " << loader.get() << "\n";
      return nullptr;
    }
    
    ResourceEditor* editor = handlerIter->second->CreateEditor(iParent, iDoc);
    if (editor)
    {
      iDoc->SetEditor(editor);
    }

    return editor;
  }

  ResourceEditor::ResourceEditor(QWidget* iParent, DocumentState* iDocument)
    : QWidget(iParent)
    , m_Document(iDocument)
  {
    setAttribute(Qt::WA_DeleteOnClose, true);
    connect(this, &ResourceEditor::resourceModified, [this]
    {
      m_Document->Touch();
    });
  }

  void ResourceEditor::Cleanup()
  {
    m_Document.reset();
  }

  bool EditorState::AttemptToCloseEditor(DocumentState* iDoc)
  {
    if (!iDoc->IsSaved())
    {
      Resource* rsc = iDoc->GetResource();
      QString message("Resource at ");
      message.append(QString::fromStdString(ResourceManager::GetPath(rsc->GetHeader().m_ResourceId).string()));
      message.append(" was not saved. Would you like to save it now ?");


      QMessageBox::StandardButton button = QMessageBox::question(nullptr, "Save resource", message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
      if (button == QMessageBox::Yes)
      {
        iDoc->Save();
      }

      if (button == QMessageBox::Cancel)
      {
        return false;
      }
    }

    if (GetCurrentActiveEditor() == iDoc->GetEditor())
    {
      SetCurrentActiveEditor(nullptr);
    }

    if (iDoc->IsSaved())
    {
      Resource* rsc = iDoc->GetResource();
      if (ResourceEditor* editor = iDoc->GetEditor())
      {
        if (editor->IsClosing())
        {
          return true;
        }
        editor->SetClosing();
        if (editor->m_SubWindow)
        {
          editor->m_SubWindow->close();
        }
        editor->Cleanup();
        editor->close();
        iDoc->SetEditor(nullptr);
      }
      s_OpenedDocuments.erase(rsc->GetHeader().m_ResourceId);
    }

    return true;
  }

  ResourceEditor* EditorState::GetCurrentActiveEditor()
  {
    return s_CurrentActiveEditor;
  }

  void EditorState::SetCurrentActiveEditor(ResourceEditor* Editor)
  {
    s_CurrentActiveEditor = Editor;
  }
}