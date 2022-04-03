#include "gamewidget.hpp"

#include <core/input.hpp>

#include <QPaintEvent>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "qtapplication.hpp"

#include <engine/common/app.hpp>
#include <engine/common/world.hpp>
#include <engine/gfx/gfxsystem.hpp>

namespace eXl
{
  GameWidget::GameWidget(QWidget *parent)
    :QOpenGLWidget(parent)
    ,m_X(-1)
    ,m_Y(-1)
  {
    setMouseTracking(true);
    setMinimumSize(parent->width(),parent->height());
  }

  GameWidget::~GameWidget()
  {
  }

  void GameWidget::initializeGL()
  {
    QOpenGLWidget::initializeGL();
		GfxSystem::StaticInit();
		if (m_GfxSystem)
		{
			m_ViewInfo.viewportSize = Vec2i(width(), height());
		}
    m_InitGLDone = true;
  }

  void GameWidget::resizeGL(int w, int h)
  {
    QOpenGLWidget::resizeGL(w, h);
    if (m_GfxSystem)
    {
      m_ViewInfo.viewportSize = Vec2i(w,h);
    }
  }

  void GameWidget::paintGL()
  {
    if (!m_InitGLDone)
    {
      return;
    }

    std::aligned_storage_t<sizeof(QPainter), alignof(QPainter)> painterBuff;
    QPainter* painter = nullptr;
    if (m_PainterItf)
    {
      painter = (QPainter*)&painterBuff._Val;
      new(painter) QPainter(this);
    }
    
    if (painter)
    {
      painter->beginNativePainting();
    }

    QOpenGLWidget::paintGL();
		float elapsed = m_Clock.GetTime();
		if (m_GfxSystem)
		{
      if (m_Tick)
      {
        m_Tick(elapsed);
      }
      m_GfxSystem->SynchronizeTransforms();
      m_GfxSystem->SetView(m_ViewInfo);
			m_GfxSystem->RenderFrame(IsAnimated() ? elapsed : 0);
		}

    if (painter)
    {
      painter->endNativePainting();
      context()->functions()->glDisable(GL_DEPTH_TEST);
      m_PainterItf->paint(*painter);

      painter->~QPainter();
      painter = nullptr;
    }
  }
  
  void GameWidget::keyPressEvent(QKeyEvent* evt)
  {
    if (m_Input)
    {
      m_Input->InjectKeyEvent(evt->text().at(0).unicode(), 0, true);

      evt->accept();
    }
  }

  void GameWidget::keyReleaseEvent(QKeyEvent* evt)
  {
    if (m_Input)
    {
      m_Input->InjectKeyEvent(evt->text().at(0).unicode(), 0, false);

      evt->accept();
    }
  }

  void GameWidget::wheelEvent (QWheelEvent* evt)
  {
    if (m_Input)
    {
      m_Input->InjectMouseMoveEvent(0, 0, evt->angleDelta().x() / 120, evt->angleDelta().y() / 120, true);

      evt->accept();
    }
  }

  void GameWidget::mouseMoveEvent(QMouseEvent* evt)
  {
    if (m_Input)
    {
      int curX = evt->x();
      int curY = evt->y();
      if (m_X > 0 && m_Y > 0)
      {
        m_Input->InjectMouseMoveEvent(curX, curY, curX - m_X, curY - m_Y, false);
      }
      else
      {
        m_Input->InjectMouseMoveEvent(curX, curY, 0, 0, false);
      }
      m_X = curX;
      m_Y = curY;

      evt->accept();
    }
  }

  void GameWidget::mousePressEvent (QMouseEvent* evt)
  {
    if (m_Input)
    {
      MouseButton button;
      switch (evt->button())
      {
      case Qt::LeftButton:
        button = MouseButton::Left;
        break;
      case Qt::RightButton:
        button = MouseButton::Right;
        break;
      case Qt::MiddleButton:
        button = MouseButton::Middle;
        break;
      default:
        return;
        break;

      }
      m_Input->InjectMouseEvent(button, true);

      evt->accept();
    }
    
  }

  void GameWidget::mouseReleaseEvent (QMouseEvent* evt)
  {
    if (m_Input)
    {
      MouseButton button;
      switch (evt->button())
      {
      case Qt::LeftButton:
        button = MouseButton::Left;
        break;
      case Qt::RightButton:
        button = MouseButton::Right;
        break;
      case Qt::MiddleButton:
        button = MouseButton::Middle;
        break;
      default:
        return;
        break;

      }
      m_Input->InjectMouseEvent(button, false);

      evt->accept();
    }
  }

  void GameWidget::SetAnimated(bool iAnimated)
  {
    if (iAnimated != m_Animated)
    {
      m_Clock.GetTime();
      m_Animated = iAnimated;
      if (m_Animated)
      {
        QTApplication::Get().AddGameWidgetToTick(this);
      }
      else
      {
        QTApplication::Get().RemoveGameWidgetToTick(this);
      }
    }
  }

  void GameWidget::SetPainterInterface(PainterInterface* iPainterItf)
  {
    m_PainterItf = iPainterItf;
    ViewInfoUpdated();
  }

  void GameWidget::ViewInfoUpdated()
  {
    if (m_PainterItf)
    {
      CameraMatrix const& camera = m_GfxSystem->GetCurrentCamera();
      QTransform screenToClip(2.0 / m_ViewInfo.viewportSize.x, 0.0
        , 0.0, -2.0 / m_ViewInfo.viewportSize.y
        , -1.0, 1.0);

      QTransform clipToScreen(0.5 * m_ViewInfo.viewportSize.x, 0.0
        , 0.0,  -0.5 * m_ViewInfo.viewportSize.y
        , 0.5 * m_ViewInfo.viewportSize.x, 0.5 * m_ViewInfo.viewportSize.y);

      Mat4 mat = camera.projMatrix * camera.viewMatrix;

      QTransform worldToClip(mat[0][0], mat[0][1]
        , mat[1][0], mat[1][1]
        , mat[3][0], mat[3][1]);

      Mat4 invMat = inverse(mat);

      QTransform clipToWorld(invMat[0][0], invMat[0][1]
        , invMat[1][0], invMat[1][1]
        , invMat[3][0], invMat[3][1]);

      m_PainterItf->SetScreenToWorldTransform(screenToClip * clipToWorld);
      m_PainterItf->SetWorldToScreenTransform(worldToClip * clipToScreen);
    }
  }
}
