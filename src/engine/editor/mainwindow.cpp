#include "mainwindow.h"
#include "eXl_Editor/ui_mainwindow.h"

#include "eXl_Editor/vector_editor.h"
#include "eXl_Editor/quaternion_editor.h"
#include "eXl_Editor/objectptr_editor.h"
#include "eXl_Editor/resourcehandle_editor.h"

#include <core/type/tupletype.hpp>
#include <core/plugin.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/resourcehandletype.hpp>

#include <core/resource/resourcemanager.hpp>
#include <engine/common/project.hpp>

#include "editordef.hpp"
#include "editorstate.hpp"
#include "gamewidget.hpp"

#include "objectdelegate.hpp"

#include <math/math.hpp>
#include <core/type/coretype.hpp>

#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QSignalMapper>
#include <QPainter>
#include <QListView>
#include <QTreeView>
#include <QTextBrowser>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>

#include "modeltest.hpp"

#include <editor/tileset/tileseteditor.hpp>
#include <editor/tileset/tilinggroupeditor.hpp>
#include <editor/archetypeeditor.hpp>
#include <editor/mapeditor/mapeditor.hpp>
#include <editor/projecteditor.hpp>
#include <editor/scripteditor.hpp>
#include <editor/mcmcmodeleditor.hpp>
#include <engine/gfx/tileset.hpp>

namespace eXl
{
  class QLogDisplay : public QTextBrowser
  {
  public:
    QLogDisplay(QWidget* parent)
      :QTextBrowser(parent)
    {

    }

    void paintEvent(QPaintEvent* iEvent) override
    {
      for (auto const& msg : m_PendingMessages)
      {
        append(msg);
      }
      m_PendingMessages.clear();
      QTextBrowser::paintEvent(iEvent);
    }

    Vector<QString> m_PendingMessages;
  };

  class QTOutput : public Log_Manager::LogOutput
  {
  public:
    QTOutput(QLogDisplay* iTextOut)
      :m_Out(iTextOut)
    {
    }

    void write(const char* iText)
    {
      m_Out->m_PendingMessages.push_back(QString::fromUtf8(iText));
      m_Out->update();
    }

  private:
    QLogDisplay* m_Out;
  };

  struct VectorObjHandler : ObjectDelegate::ObjectHandler
  {
    QWidget* CreateEditor(QWidget* iParent) override
    {
      return Vector_Editor::Construct(iParent);
    }

    virtual void paint(QPainter* iPainter, const QStyleOptionViewItem &iOption, ConstDynObject const& iObj) override
    {
      bool isFloat = false;
      int numFields = 0;
      Type const* type = iObj.GetType();
      if (type == TypeManager::GetType<Vector2f>())
      {
        isFloat = true;
        numFields = 2;
      }
      if (type == TypeManager::GetType<Vector3f>())
      {
        isFloat = true;
        numFields = 3;
      }
      if (type == TypeManager::GetType<Vector4f>()
        || type == TypeManager::GetType<Quaternionf>())
      {
        isFloat = true;
        numFields = 4;
      }
      if (type == TypeManager::GetType<Vector2i>())
      {
        numFields = 2;
      }
      if (type == TypeManager::GetType<Vector3i>())
      {
        numFields = 3;
      }
      if (type == TypeManager::GetType<Vector4i>())
      {
        numFields = 4;
      }
      eXl_ASSERT_REPAIR_RET(numFields != 0, );

      char letter[] = { 'X', 'Y', 'Z', 'W' };

      String displayStr;

      if (isFloat)
      {
        float const* data = reinterpret_cast<float const*>(iObj.GetBuffer());
        for (int i = 0; i < numFields; ++i)
        {
          displayStr += letter[i];
          displayStr += ": ";
          String tempStr = StringUtil::FromFloat(data[i]);
          while (tempStr.size() > 1 && tempStr.back() == '0')
          {
            tempStr.pop_back();
          }
          if (tempStr.back() == '.')
          {
            tempStr.push_back('0');
          }
          displayStr += tempStr;
          displayStr += " ";
        }
      }
      else
      {
        int const* data = reinterpret_cast<int const*>(iObj.GetBuffer());
        for (int i = 0; i < numFields; ++i)
        {
          displayStr += letter[i];
          displayStr += ": ";
          displayStr += StringUtil::FromInt(data[i]);
          displayStr += " ";
        }
      }
      iPainter->drawText(iOption.rect, QString::fromUtf8(displayStr.c_str()));
    }
  };

  struct QuaternionHandler : VectorObjHandler
  {
    QWidget* CreateEditor(QWidget* iParent) override
    {
      return Quaternion_Editor::Construct(iParent);
    }
  };

  struct ResourceHandleHandler : ObjectDelegate::ObjectHandler
  {
    QWidget* CreateEditor(QWidget* iParent) override
    {
      return ResourceHandle_Editor::Construct(iParent);
    }

    virtual void paint(QPainter* iPainter, const QStyleOptionViewItem &iOption, ConstDynObject const& iObj) override
    {
      ResourceHandleType const* typeId = ResourceHandleType::DynamicCast(iObj.GetType());
      
      Resource::UUID const& uuid = typeId->GetUUID(iObj.GetBuffer());
      Resource::Header const* header = ResourceManager::GetHeader(uuid);
      if (header)
      {
        iPainter->drawText(iOption.rect.bottomLeft(), QString::fromUtf8(header->m_ResourceName.c_str()));
      }
      else
      {
        iPainter->drawText(iOption.rect.bottomLeft(), QString::fromUtf8("(null)"));
      }
    }
  };

  VectorObjHandler s_VectorHandler;
  QuaternionHandler s_QuaternionHandler;
  ResourceHandleHandler s_ResourceHandlerHandler;

  class MainWindow::EventFilter : public QObject
  {
  public:
    EventFilter(QObject* iParent)
      : QObject(iParent)
    {

    }
  protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
      MainWindow* mainWin = static_cast<MainWindow*>(obj);

      if (event->type() == QEvent::Close)
      {
        QList<QMdiSubWindow*> windows = mainWin->m_Documents->subWindowList();
        windows.detach();
        for (auto editorWindow : windows)
        {
          mainWin->m_Documents->setActiveSubWindow(editorWindow);
          ResourceEditor* editor = static_cast<ResourceEditor*>(editorWindow->widget());
          if (!EditorState::AttemptToCloseEditor(editor->GetDocument()))
          {
            event->accept();
            return true;
          }
          else
          {
            editorWindow->removeEventFilter(editor->m_CloseFilter);
            mainWin->m_Documents->closeActiveSubWindow();
          }
        }
      }
      return QObject::eventFilter(obj, event);
    }
  };

  MainWindow::MainWindow(QWidget *parent)
    :QMainWindow(parent)
    , ui(new Ui::MainWindow)
  {
    ui->setupUi(this);

    EditorState::BuildState(this);
    EditorState::AddResourceHandler(&TilesetEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&TilingGroupEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&ArchetypeEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&MapEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&ProjectEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&LuaScriptEditor::GetEditorHandler());
    EditorState::AddResourceHandler(&MCMCModelEditor::GetEditorHandler());

    connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::openProject);
    connect(ui->actionCloseProject, &QAction::triggered, this, &MainWindow::closeProject);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::save);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);

    QSignalMapper* createActSigMapper = new QSignalMapper(this);
    connect(createActSigMapper, SIGNAL(mapped(QObject*)), this, SLOT(onCreateAction(QObject*)));

    Vector<ResourceLoaderName> Loaders = EditorState::GetAllRegisteredHandlers();
    for (auto loader : Loaders)
    {
      if (loader == Project::StaticLoaderName())
      {
        continue;
      }
      QAction* newAction = new QAction(QString::fromUtf8(loader.c_str()), this);
      newAction->setData(QVariant::fromValue(loader));
      createActSigMapper->setMapping(newAction, newAction);
      connect(newAction, SIGNAL(triggered()), createActSigMapper, SLOT(map()));
      newAction->setDisabled(true);
      ui->menuCreate->addAction(newAction);
    }

    QAction* editProjectAction = new QAction("Edit Type definitions");
    QObject::connect(editProjectAction, &QAction::triggered, [this]
    {
      DocumentState* projectDoc = EditorState::GetCurrentProject();
      OpenResource(projectDoc->GetResource()->GetHeader().m_ResourceId);
    });

    editProjectAction->setDisabled(true);
    ui->menuEdit->addAction(editProjectAction);

    connect(EditorState::GetState(), SIGNAL(projectOpened()), SLOT(projectOpened()));
    connect(EditorState::GetState(), SIGNAL(projectClosed()), SLOT(projectClosed()));

    //ObjectDelegate::RegisterFactory(Resource_Editor::Construct,TypeManager::GetCoreType<Handle<Resource const> >()->GetTypeId());
    //ObjectDelegate::RegisterFactory(ObjectPtr_Editor::Construct,TypeManager::GetCoreType<RttiOPtr >()->GetTypeId());
    ObjectDelegate::RegisterType(&s_QuaternionHandler, TypeManager::GetType<Quaternionf>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector2i>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector3i>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector4i>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector2f>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector3f>());
    ObjectDelegate::RegisterType(&s_VectorHandler, TypeManager::GetType<Vector4f>());

    Vector<ResourceLoaderName> loaders = EditorState::GetAllRegisteredHandlers();
    for (auto loader : loaders)
    {
      ObjectDelegate::RegisterType(&s_ResourceHandlerHandler, ResourceManager::GetHandleType(loader));
    }

    QDockWidget* consoleDockWidget = new QDockWidget(this);
    consoleDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    consoleDockWidget->setTitleBarWidget(new QLabel("Log", consoleDockWidget));

    QLogDisplay* logOut = new QLogDisplay(this);
    consoleDockWidget->setWidget(logOut);

    addDockWidget(Qt::BottomDockWidgetArea, consoleDockWidget);

    m_ResourcesView = new QTreeView;

    connect(m_ResourcesView, SIGNAL(doubleClicked(QModelIndex const&)), this, SLOT(onResourceDoubleClick(QModelIndex const&)));

    QDockWidget* resourcesDockWidget = new QDockWidget(this);
    resourcesDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
    resourcesDockWidget->setTitleBarWidget(new QLabel("Resources", resourcesDockWidget));
    resourcesDockWidget->setWidget(m_ResourcesView);

    addDockWidget(Qt::LeftDockWidgetArea, resourcesDockWidget);

    QTOutput* output = new QTOutput(logOut);
    Log_Manager::AddOutput(output, INFO_STREAM_FLAG | ERROR_STREAM_FLAG | WARNING_STREAM_FLAG | LUA_OUT_STREAM_FLAG | LUA_ERR_STREAM_FLAG);

    m_Documents = new QMdiArea(this);
    m_Documents->setViewMode(QMdiArea::TabbedView);
    m_Documents->setTabsClosable(true);
    setCentralWidget(m_Documents);

    installEventFilter(new EventFilter(this));

    EditorState::RestoreProject();
  }

  MainWindow::~MainWindow()
  {
    delete ui;
  }

  void MainWindow::closeEvent(QCloseEvent* event)
  {
    QMainWindow::closeEvent(event);

    emit mainWindowClosed();
  }

  void MainWindow::newProject()
  {
    closeProject();

    QString file = QFileDialog::getSaveFileName(this, "New Project",
      "",
      "Project file (*.eXlProject)");

    if (!file.isEmpty())
    {
      Path filePath(file.toStdString());
      Path projectDir = filePath.parent_path();
      Path projectName = filePath.filename();
      if (!EditorState::CreateProject(projectDir, projectName.string().c_str()))
      {
        LOG_ERROR << "Could not create new project at " << filePath.string() << "\n";
      }
    }
  }

  void MainWindow::openProject()
  {
    closeProject();

    QString file = QFileDialog::getOpenFileName(this, "New Project",
      "",
      "Project file (*.eXlProject)");

    if (!file.isEmpty())
    {
      Path filePath(file.toStdString());
      EditorState::OpenProject(filePath);
    }
  }

  void MainWindow::closeProject()
  {
    EditorState::CloseProject();
  }

  void MainWindow::projectOpened()
  {
    Path projectPath = EditorState::GetProjectDirectory();

    m_ResourcesView->setModel(EditorState::GetProjectResourcesModel());

    QList<QAction*> actions = ui->menuCreate->actions();
    for (unsigned int i = 0; i < actions.size(); ++i)
    {
      actions[i]->setEnabled(true);
    }

    actions = ui->menuEdit->actions();
    for (unsigned int i = 0; i < actions.size(); ++i)
    {
      actions[i]->setEnabled(true);
    }
  }

  void MainWindow::projectClosed()
  {
    QList<QAction*> actions = ui->menuCreate->actions();
    for (unsigned int i = 0; i < actions.size(); ++i)
    {
      actions[i]->setEnabled(false);
    }

    actions = ui->menuEdit->actions();
    for (unsigned int i = 0; i < actions.size(); ++i)
    {
      actions[i]->setEnabled(false);
    }
  }

  void MainWindow::onCreateAction(QObject* iObject)
  {
    if (EditorState::GetCurrentProject())
    {
      QAction* action = static_cast<QAction*>(iObject);
      auto loaderName = action->data().value<ResourceLoaderName>();
      if (DocumentState* newDoc = EditorState::CreateResource(loaderName, this))
      {
        OpenResource(newDoc->GetResource()->GetHeader().m_ResourceId);
      }
    }
  }

  void MainWindow::onResourceDoubleClick(QModelIndex const& iIndex)
  {
    if (EditorState::GetCurrentProject())
    {
      ProjectResourcesModel* resources = EditorState::GetProjectResourcesModel();
      Resource::UUID const* uuid = resources->GetResourceIDFromIndex(iIndex);
      if (uuid)
      {
        OpenResource(*uuid);
      }
    }
  }

  class EditorEventFilter : public QObject
  {
  public:
    EditorEventFilter(QObject* iParent)
      : QObject(iParent)
    {

    }
  protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
      QMdiSubWindow* editorWindow = static_cast<QMdiSubWindow*>(obj);
      ResourceEditor* editor = static_cast<ResourceEditor*>(editorWindow->widget());
      if (event->type() == QEvent::Close)
      {
        if (editor->IsClosing() || !EditorState::AttemptToCloseEditor(editor->GetDocument()))
        {
          event->accept();
          return true;
        }
        editor->m_SubWindow = nullptr;
      }
      return QObject::eventFilter(obj, event);
    }
  };

  void MainWindow::OpenResource(Resource::UUID const& iUUID)
  {
    if (ResourceEditor* newEditor = EditorState::OpenResourceEditor(iUUID, this))
    {
      if (newEditor->m_SubWindow)
      {
        m_Documents->setActiveSubWindow(newEditor->m_SubWindow);
        return;
      }

      DocumentState* doc = newEditor->GetDocument();
      Resource* rsc = doc->GetResource();

      newEditor->m_SubWindow = m_Documents->addSubWindow(newEditor);
      newEditor->m_SubWindow->setWindowTitle(QString::fromUtf8(rsc->GetName().c_str()));
      newEditor->m_SubWindow->show();

      EditorState::SetCurrentActiveEditor(newEditor);

      QObject::connect(newEditor->m_SubWindow, &QMdiSubWindow::aboutToActivate, [newEditor]()
      {
        EditorState::SetCurrentActiveEditor(newEditor);
      });
      newEditor->m_CloseFilter = new EditorEventFilter(newEditor->m_SubWindow);
      newEditor->m_SubWindow->installEventFilter(newEditor->m_CloseFilter);
    }
  }

  void MainWindow::save()
  {
    if (ResourceEditor* editor = EditorState::GetCurrentActiveEditor())
    {
      QMdiSubWindow* activeWindow = m_Documents->activeSubWindow();
      eXl_ASSERT(activeWindow->widget() == editor);

      if (!editor->GetDocument()->IsSaved())
      {
        editor->GetDocument()->Save();
      }
    }
  }

  void MainWindow::close()
  {
    if (ResourceEditor* editor = EditorState::GetCurrentActiveEditor())
    {
      QMdiSubWindow* activeWindow = m_Documents->activeSubWindow();
      eXl_ASSERT(activeWindow->widget() == editor);
      if(editor->m_SubWindow)
      {
        editor->m_SubWindow->close();
      }
      else
      {
        EditorState::AttemptToCloseEditor(editor->GetDocument());
      }
    }
  }
}
