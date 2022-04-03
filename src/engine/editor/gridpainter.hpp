#pragma once

#include "gamewidget.hpp"

#include <math/aabb2d.hpp>

namespace eXl
{
  class GridPainter : public GameWidget::PainterInterface
  {
    Q_OBJECT
  public:

    GridPainter(QObject* iParent) : PainterInterface(iParent)
    {}

    Vec2 m_CellSize = One<Vec2>();

  protected:
    void paint(QPainter& iPainter) const override;
  };
}