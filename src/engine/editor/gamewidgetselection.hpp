#pragma once

#include "gamewidget.hpp"

#include <math/aabb2d.hpp>

namespace eXl
{
  class MouseSelectionFilter : public QObject
  {
    Q_OBJECT
  public:
    MouseSelectionFilter(QObject* iParent) : QObject(iParent)
    {}

    bool eventFilter(QObject *dest, QEvent *event) override;

  Q_SIGNALS:
    void onSelectionStarted(AABB2Di const&);
    void onSelectionChanged(AABB2Di const&);
    void onSelectionEnded(AABB2Di const&);

  protected:
    AABB2Di m_Selection;
    bool m_Pressed = false;
    Vector2i m_PressPos;
  };

  class SelectionPainter : public GameWidget::PainterInterface
  {
    Q_OBJECT
  public:

    SelectionPainter(QObject* iParent) : PainterInterface(iParent)
    {}

    Vector<AABB2Di> m_Boxes;

  protected:
    void paint(QPainter& iPainter) const override;
  };
}