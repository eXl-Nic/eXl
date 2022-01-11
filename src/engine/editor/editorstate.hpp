#pragma once

#include <core/coredef.hpp>
#include <core/path.hpp>
#include <core/refcobject.hpp>
#include <core/log.hpp>

#include <core/resource/resource.hpp>

#include "projectresourcesmodel.hpp"

#include <QWidget>

class QSettings;
class QMdiSubWindow;

namespace eXl
{
  class MainWindow;
  class Resource;
  class ResourceEditor;
  class DocumentState;
  class PropertiesManifest;

  class ResourceEditorHandler
  {
  public:

    virtual DocumentState* CreateNewDocument();
    virtual ResourceEditor* CreateEditor(QWidget* iParent, DocumentState* iDoc) = 0;

    ResourceLoaderName GetLoaderName() const { return m_Loader; }

    static bool IsInProjectFolder(Path const& iPath);

  protected:
    ResourceEditorHandler(ResourceLoaderName iName)
      : m_Loader(iName)
    {}

    ResourceLoaderName m_Loader;
  };

  class DocumentState
  {
    DECLARE_RefC;
  public:
    DocumentState(Resource& iRsc, bool iIsSaved = false);

    virtual ~DocumentState();

    virtual bool IsSaved();

    virtual void Touch();

    virtual bool Save();

    String const& GetName() const;

    Resource* GetResource() { return &m_Resource; }

    ResourceEditor* GetEditor() { return m_Editor; };
    ResourceEditor* OpenEditor(QWidget* iParent);
    void SetEditor(ResourceEditor* Editor) { m_Editor = Editor; }

  protected:
    Resource& m_Resource;
    ResourceEditor* m_Editor = nullptr;
    ResourceHandle<Resource> m_Handle;
    bool m_Saved;
  };

  class ResourceEditor : public QWidget
  {
    Q_OBJECT;
  public:
    ResourceEditor(QWidget* iParent, DocumentState* iDocument);

    bool IsClosing()
    {
      return m_Closing;
    }

    void SetClosing()
    {
      m_Closing = true;
    }

    virtual void Cleanup();

    DocumentState* GetDocument() const { return m_Document.get(); }
    virtual void CommitDocument() {}
    QMdiSubWindow* m_SubWindow = nullptr;
    QObject* m_CloseFilter = nullptr;

    void ModifyResource()
    {
      emit resourceModified(m_Document.get());
    }

  Q_SIGNALS:
    void resourceModified(DocumentState*);

  protected:
    bool m_Closing = false;
    IntrusivePtr<DocumentState> m_Document;
  };

  class EditorState : public QObject
  {
    Q_OBJECT;
  public:

    static EditorState* GetState();

    static void BuildState(MainWindow* iMainWindow);

    static QSettings& GetSettings();

    static void RestoreProject();

    static DocumentState* GetCurrentProject();
    static DocumentState* CreateProject(Path const& iDir, String const& iName);
    static DocumentState* OpenProject(Path const& iPath);
    static Path const& GetProjectDirectory();
    static ProjectResourcesModel* GetProjectResourcesModel();
    static bool SaveProject();
    static bool CloseProject();
    static PropertiesManifest const& GetProjectProperties();

    static void AddResourceHandler(ResourceEditorHandler* iHandler);
    static Vector<ResourceLoaderName> GetAllRegisteredHandlers();

    static DocumentState* CreateResource(ResourceLoaderName iName, QWidget* iParent = nullptr);
    static DocumentState* GetOpenedDocument(Resource::UUID const& iUUID);
    static DocumentState* OpenDocument(Resource::UUID const& iUUID);

    static ResourceEditor* OpenResourceEditor(Resource::UUID const& iUUID, QWidget* iParent);
    static ResourceEditor* OpenResourceEditor(DocumentState* iDoc, QWidget* iParent);

    static ResourceEditor* GetCurrentActiveEditor();
    static void SetCurrentActiveEditor(ResourceEditor*);

    static bool AttemptToCloseEditor(DocumentState*);

signals:

    void projectOpened();

    void projectClosed();

    void projectSaved();
  };

  template <typename Resource, typename Editor>
  class EditorHandler_T : public ResourceEditorHandler
  {
  public:

    EditorHandler_T()
      : ResourceEditorHandler(Resource::StaticLoaderName())
    {}

    DocumentState* CreateNewDocument() override
    {
      QString resourceLoaderName = m_Loader.c_str();
      QString title("New ");
      title.append(QString::fromUtf8(Resource::StaticLoaderName().c_str()));

      QString file = QFileDialog::getSaveFileName(nullptr, title,
        QString::fromStdString(EditorState::GetProjectDirectory().string()),
        "Resource file (*.eXlAsset)");

      if (file.isEmpty())
      {
        return nullptr;
      }

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

      Path newResourceDir = Filesystem::absolute(Filesystem::canonical(newResourcePath.parent_path()));
      Path resourceName = newResourcePath.filename();
      resourceName.replace_extension();
      Resource* newResource = Resource::Create(newResourceDir, resourceName.string().c_str());
      if (newResource)
      {
        DocumentState* document = new DocumentState(*newResource);
        return document;
      }
      return nullptr;
    }

    ResourceEditor* CreateEditor(QWidget* iParent, DocumentState* iDoc) override
    {
      return new Editor(iParent, iDoc);
    }

  };
}
