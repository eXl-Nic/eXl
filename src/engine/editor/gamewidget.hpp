#pragma once

#include <QOpenGLWidget>
#include <core/clock.hpp>
#include <engine/gfx/gfxsystem.hpp>

class QMouseEvent;

namespace eXl
{
  class InputSystem;

  class GameWidget : public QOpenGLWidget
  {
    Q_OBJECT
  public:

    class PainterInterface : public QObject
    {
    public:
      PainterInterface(QObject* iParent) : QObject(iParent)
      {}
      
      virtual void paint(QPainter& iPainter) const = 0;

      void SetScreenToWorldTransform(QTransform const& iTrans)
      {
        m_ScreenToWorld = iTrans;
      }
      void SetWorldToScreenTransform(QTransform const& iTrans)
      {
        m_WorldToScreen = iTrans;
      }

      QTransform const& GetScreenToWorldTransform() const { return m_ScreenToWorld; }
      QTransform const& GetWorldToScreenTransform() const { return m_WorldToScreen; }

    protected:
      QTransform m_ScreenToWorld;
      QTransform m_WorldToScreen;
    };

    GameWidget(QWidget *parent = 0);
    ~GameWidget();

    void SetPainterInterface(PainterInterface* iPainterItf);
    
    void SetGfxSystem(GfxSystem* iGfxSystem)
    {
      m_GfxSystem = iGfxSystem;
    }

    void SetInputSystem(InputSystem* iSystem)
    {
      m_Input = iSystem;
    }

    void SetTickCallback(std::function<void(float)> iTick)
    {
      m_Tick = std::move(iTick);
    }

    GfxSystem::ViewInfo& GetViewInfo() { return m_ViewInfo; }
    void ViewInfoUpdated();

    void SetAnimated(bool iAnimated);

    bool IsAnimated() const { return m_Animated; }

  protected:

    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void keyPressEvent(QKeyEvent* evt);
    void keyReleaseEvent(QKeyEvent* evt);
    void mouseMoveEvent(QMouseEvent* evt);
    void mousePressEvent (QMouseEvent* evt);
    void mouseReleaseEvent (QMouseEvent* evt);
    void wheelEvent (QWheelEvent* evt);

    int m_X;
    int m_Y;
    //float m_ForcedAspectRatio = -1.0;
    bool m_Animated = false;
    bool m_InitGLDone = false;

    std::function<void(float)> m_Tick;

    GfxSystem::ViewInfo m_ViewInfo;
    InputSystem* m_Input = nullptr;
    GfxSystem* m_GfxSystem = nullptr;
    PainterInterface* m_PainterItf = nullptr;
		Clock m_Clock;
  };
}
