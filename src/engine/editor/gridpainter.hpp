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

    Vector2f m_CellSize = Vector2f::ONE;

  protected:
    void paint(QPainter& iPainter) const override;
  };
}