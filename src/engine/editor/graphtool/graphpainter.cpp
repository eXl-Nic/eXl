#include "graphpainter.hpp"
#include <QPainter>

namespace eXl
{
  GraphPainter::GraphPainter(QObject* iParent)
    : GameWidget::PainterInterface(iParent)
  {}

  void GraphPainter::paint(QPainter& iPainter) const
  {
    iPainter.setTransform(GetWorldToScreenTransform());
    QPen pen;
    pen.setWidthF(0.1);
    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
      pen.setColor(nodesColor[i]);
      iPainter.setPen(pen);

      QTransform trans;
      trans.translate(nodes[i].x(), nodes[i].y());
      trans = trans.rotate(180, Qt::ZAxis);
      trans = trans.scale(-0.1, 0.1);
      iPainter.setTransform(trans * GetWorldToScreenTransform());
      iPainter.drawText(QPointF(), nodeDesc[i]);
      iPainter.setTransform(GetWorldToScreenTransform());

      iPainter.drawEllipse(nodes[i], s_NodeSize, s_NodeSize);
    }

    for (uint32_t i = 0; i < edges.size(); ++i)
    {
      pen.setColor(edgesColor[i]);
      iPainter.setPen(pen);

      iPainter.drawLine(edges[i].first, edges[i].second);

      QPointF middle = (edges[i].first + edges[i].second) * 0.5;

      QTransform trans;
      trans.translate(middle.x(), middle.y());
      trans = trans.rotate(180, Qt::ZAxis);
      trans = trans.scale(-0.1, 0.1);
      iPainter.setTransform(trans * GetWorldToScreenTransform());
      //iPainter.drawText(QPointF(), edgeDesc[i]);
      iPainter.setTransform(GetWorldToScreenTransform());
    }
    iPainter.setTransform(QTransform());
  }

  void GraphPainter::Clear()
  {
    nodeDesc.clear();
    nodes.clear();
    nodesColor.clear();
    edges.clear();
    edgeDesc.clear();
    edgesColor.clear();
  }
}