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

    void SetSnapSize(Vector2i const& iSize);

  Q_SIGNALS:
    void onStartDrawing();
    void onAddPoint(Vector2i, bool);
    void onStopDrawing();

  protected:
    bool m_IsDrawing = false;

    World& m_World;
    GfxSystem& m_GfxSys;
    Transforms& m_Transforms;
    ObjectHandle m_PenObject;
    Vector2i m_CurrentMousePos;
    Vector3f m_CurrentWorldPos;
    Vector2i m_CurrentSnapPos;
    Vector2i m_Snap;
  };
}