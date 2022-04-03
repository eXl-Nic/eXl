#pragma once

#include <math/aabb2dpolygon.hpp>
#include <engine/common/world.hpp>

#include <QObject>

namespace eXl
{
  class GfxSystem;

  class PenToolFilter : public QObject
  {
    Q_OBJECT
  public:
    PenToolFilter(QObject* iParent, World& iWorld);

    bool eventFilter(QObject *dest, QEvent *event) override;

    void StopDrawing();
    void AddCurrentPosition(bool iWasDrawing);

    ObjectHandle GetPenObject()
    {
      return m_PenObject;
    }
    void SetPenObject(ObjectHandle iHandle)
    {
      m_PenObject = iHandle;
    }

    void SetSnapSize(Vec2i const& iSize);

  Q_SIGNALS:
    void onStartDrawing();
    void onAddPoint(Vec2i, bool);
    void onStopDrawing();

  protected:
    bool m_IsDrawing = false;

    World& m_World;
    GfxSystem& m_GfxSys;
    Transforms& m_Transforms;
    ObjectHandle m_PenObject;
    Vec2i m_CurrentMousePos;
    Vec3 m_CurrentWorldPos;
    Vec2i m_CurrentSnapPos;
    Vec2i m_Snap;
  };
}