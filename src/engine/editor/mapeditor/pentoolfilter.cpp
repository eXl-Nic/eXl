#include "pentoolfilter.hpp"

#include <engine/common/transforms.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/game/commondef.hpp>
#include <math/mathtools.hpp>

#include <QMouseEvent>

namespace eXl
{
  PenToolFilter::PenToolFilter(QObject* iParent, World& iWorld)
    : QObject(iParent)
    , m_World(iWorld)
    , m_GfxSys(*m_World.GetSystem<GfxSystem>())
    , m_Transforms(*m_World.GetSystem<Transforms>())
  {

  }

  bool PenToolFilter::eventFilter(QObject *dest, QEvent *event)
  {
    switch (event->type())
    {
    case QEvent::Leave:
      StopDrawing();
      return false;
      break;
    case QEvent::Enter:
      return false;
      break;
    case QEvent::MouseButtonPress:
    {
      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      if (mouseEvent->button() == Qt::LeftButton)
      {
        m_IsDrawing = true;
        
        emit onStartDrawing();
        
        AddCurrentPosition(false);
        return true;
      }
      break;
    }
    case QEvent::MouseButtonRelease:
    {
      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      if (mouseEvent->button() == Qt::LeftButton)
      {
        StopDrawing();
        return true;
      }
      break;
    }
    case QEvent::MouseMove:
    {
      if (m_Snap.x == 0 || m_Snap.y == 0)
      {
        break;
      }

      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      m_CurrentMousePos = Vec2i(mouseEvent->pos().x(), mouseEvent->pos().y());

      Vec3 viewDir;
      m_GfxSys.ScreenToWorld(m_CurrentMousePos, m_CurrentWorldPos, viewDir);

      Vec2i snapPos(m_CurrentWorldPos.x * EngineCommon::s_WorldToPixel, m_CurrentWorldPos.y * EngineCommon::s_WorldToPixel);
      snapPos.x -= Mathi::Mod(snapPos.x, m_Snap.x);
      snapPos.y -= Mathi::Mod(snapPos.y, m_Snap.y);
      snapPos += m_Snap / 2;

      if (snapPos != m_CurrentSnapPos)
      {
        m_CurrentSnapPos = snapPos;
        if (m_PenObject.IsAssigned())
        {
          Mat4 newPos = Identity<Mat4>();
          reinterpret_cast<Vec2&>(newPos[3]) = Vec2(m_CurrentSnapPos.x, m_CurrentSnapPos.y) / EngineCommon::s_WorldToPixel;

          m_Transforms.UpdateTransform(m_PenObject, newPos);
          m_GfxSys.SynchronizeTransforms();
        }
        if (m_IsDrawing)
        {
          AddCurrentPosition(true);
        }
      }
    }
    }

    return false;
  }

  void PenToolFilter::StopDrawing()
  {
    if (m_IsDrawing)
    {
      m_IsDrawing = false;
      
      emit onStopDrawing();
    }
  }

  void PenToolFilter::SetSnapSize(Vec2i const& iSize)
  {
    m_Snap = iSize;
  }

  void PenToolFilter::AddCurrentPosition(bool iWasDrawing)
  {
    AABB2Di box(m_CurrentSnapPos, One<Vec2i>());
    //if (!m_DrawnRegion.empty())
    //{
    //  Vector<AABB2DPolygoni> inter;
    //  for (auto const& region : m_DrawnRegion)
    //  {
    //    region.Intersection(AABB2DPolygoni(box), inter);
    //    if (!inter.empty())
    //    {
    //      return;
    //    }
    //  }
    //}

    emit onAddPoint(m_CurrentSnapPos, iWasDrawing);

    //AABB2DPolygoni additionalRegion(box);
    //if (m_DrawnRegion.empty())
    //{
    //  m_DrawnRegion.push_back(std::move(additionalRegion));
    //}
    //else
    //{
    //  m_DrawnRegion.push_back(std::move(additionalRegion));
    //  AABB2DPolygoni::Merge(m_DrawnRegion);
    //}
  }
}