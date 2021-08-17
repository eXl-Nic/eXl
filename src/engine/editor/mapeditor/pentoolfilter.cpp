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
      if (m_Snap.X() == 0 || m_Snap.Y() == 0)
      {
        break;
      }

      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      m_CurrentMousePos = Vector2i(mouseEvent->pos().x(), mouseEvent->pos().y());

      Vector3f viewDir;
      m_GfxSys.ScreenToWorld(m_CurrentMousePos, m_CurrentWorldPos, viewDir);

      Vector2i snapPos(m_CurrentWorldPos.X() * DunAtk::s_WorldToPixel, m_CurrentWorldPos.Y() * DunAtk::s_WorldToPixel);
      snapPos.X() -= Mathi::Mod(snapPos.X(), m_Snap.X());
      snapPos.Y() -= Mathi::Mod(snapPos.Y(), m_Snap.Y());
      snapPos += m_Snap / 2;

      if (snapPos != m_CurrentSnapPos)
      {
        m_CurrentSnapPos = snapPos;
        if (m_PenObject.IsAssigned())
        {
          Matrix4f newPos;
          newPos.MakeIdentity();
          MathTools::GetPosition2D(newPos) = Vector2f(m_CurrentSnapPos.X(), m_CurrentSnapPos.Y()) / DunAtk::s_WorldToPixel;

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

  void PenToolFilter::SetSnapSize(Vector2i const& iSize)
  {
    m_Snap = iSize;
  }

  void PenToolFilter::AddCurrentPosition(bool iWasDrawing)
  {
    AABB2Di box(m_CurrentSnapPos, Vector2i::ONE);
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