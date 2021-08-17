#pragma once

#include <QMainWindow>
#include <QAbstractItemModel>
#include <core/type/dynobject.hpp>
#include <core/resource/resource.hpp>

namespace Ui 
{
  class MainWindow;
}
class QTreeView;
class QMdiArea;

namespace eXl
{
  class ResourceContainerModel;


  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

  Q_SIGNALS:
    void mainWindowClosed();

  protected:

    void closeEvent(QCloseEvent* event);

  protected slots:

    void newProject();
    void openProject();
    void closeProject();

    void save();
    void close();

    void projectOpened();

    void projectClosed();

    void onResourceDoubleClick(QModelIndex const& );

    void OpenResource(Resource::UUID const& );

  private slots:

    void onCreateAction(QObject*);

  private:
    class EventFilter;

    Ui::MainWindow *ui;
    QTreeView* m_ResourcesView = nullptr;
    QMdiArea* m_Documents;
  };
}