#include "qtapplication.hpp"
#include "mainwindow.h"
#include "gamewidget.hpp"
#include <core/plugin.hpp>
#include <core/image/imagestreamer.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/utils/filetextreader.hpp>
#include "editor_gen.hpp"

#include <QtGui>
#include <QtWidgets/qapplication.h>

namespace eXl
{
  void Register_EDITOR_Types();

  QTApplication::QTApplication()
    :m_Appl(NULL)
  {

  }

  QTApplication& QTApplication::Get()
  {
    return static_cast<QTApplication&>(Application::GetAppl());
  }

  void QTApplication::Start()
  {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);

    m_Appl = new QApplication(m_Argc, (char**)m_ArgV);

    const char* styleSheet = R"(
      QSplitter::handle {
    background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, 
stop:0 rgba(255, 255, 255, 0), 
stop:0.407273 rgba(200, 200, 200, 255), 
stop:0.4825 rgba(101, 104, 113, 235), 
stop:0.6 rgba(255, 255, 255, 0));
    image: url(:/images/splitter.png);
     }
    )";
    
    m_Appl->setStyleSheet(styleSheet);

    ResourceManager::SetTextFileReadFactory([](char const* iFilePath)
    {
      return std::unique_ptr<TextReader>(FileTextReader::Create(iFilePath));
    });
    
    Engine_Application::Start();
    Register_EDITOR_Types();
    InitConsoleLog();
    InitFileLog("EngineCommonEditor.log");
    MainWindow* mainWindow = new MainWindow;
    mainWindow->show();

    m_Appl->setQuitOnLastWindowClosed(true);
    QObject::connect(mainWindow, &MainWindow::mainWindowClosed, [this]
    {
      Stop();
    });
  }

  void QTApplication::AddGameWidgetToTick(GameWidget* iWidget)
  {
    if (m_ToTick.count(iWidget) > 0)
    {
      return;
    }

    auto connection = QObject::connect(iWidget, &QObject::destroyed, [this, iWidget]
    {
      m_ToTick.erase(iWidget);
    });

    m_ToTick.insert(std::make_pair(iWidget, connection));
  }

  void QTApplication::RemoveGameWidgetToTick(GameWidget* iWidget)
  {
    auto iter = m_ToTick.find(iWidget);
    if (iter != m_ToTick.end())
    {
      QObject::disconnect(iter->second);
      m_ToTick.erase(iter);
    }
  }

  void QTApplication::Terminated()
  {
    //delete m_Appl;
  }

  //GfxView* QTApplication::GetDefaultView()
  //{
  //  return NULL;
  //}

  void QTApplication::Tick(float iDelta)
  {
    for (auto& toTick : m_ToTick)
    {
      toTick.first->update();
    }
    PumpMessages(iDelta);
  }

  void QTApplication::PumpMessages(float iDelta)
  {
    m_Appl->processEvents();
    m_Appl->sendPostedEvents();
  }

  //void QTApplication::SwapView(GfxView* iView)
  //{
  //
  //}
}
