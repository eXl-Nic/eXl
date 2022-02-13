#pragma once

#include <editor/gamewidget.hpp>

namespace eXl
{
  struct GraphPainter : public GameWidget::PainterInterface
  {
    static constexpr float s_NodeSize = 2;
    GraphPainter(QObject* iParent);

    void paint(QPainter& iPainter) const override;
    void Clear();

    Vector<QString> nodeDesc;
    Vector<QPointF> nodes;
    Vector<QColor> nodesColor;
    Vector<QPair<QPointF, QPointF>> edges;
    Vector<QString> edgeDesc;
    Vector<QColor> edgesColor;
  };
}