#pragma once
#include <engine/common/app.hpp>
#include <QObject>

class QApplication;

namespace eXl
{
  class GameWidget;
  
  class QTApplication : public DunAtk_Application
  {
  public:

    QTApplication();

    static QTApplication& Get();

    void Start();

    void Terminated();

    void AddGameWidgetToTick(GameWidget*);

    void RemoveGameWidgetToTick(GameWidget*);

    void Tick(float iDelta) override;

    void PumpMessages(float iDelta) override;

    //void SwapView(GfxView* iView);
  protected:
    QApplication* m_Appl;
    UnorderedMap<GameWidget*, QMetaObject::Connection> m_ToTick;
  };
}

