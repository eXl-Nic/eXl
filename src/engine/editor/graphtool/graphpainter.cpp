#include "graphpainter.hpp"
#include <QPainter>

namespace eXl
{
  GraphPainter::GraphPainter(QObject* iParent)
    : GameWidget::PainterInterface(iParent)
  {}

  void GraphPainter::paint(QPainter& iPainter) const
  {
    QRect screen = iPainter.viewport();
    QTransform const& worldToScreen = GetWorldToScreenTransform();
    QRect worldVp = GetScreenToWorldTransform().mapRect(screen);
    iPainter.setTransform(worldToScreen);
    QPen pen;
    pen.setWidthF(0.1);
    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
      QRect edgeRect(nodes[i].toPoint(), QSize(s_NodeSize, s_NodeSize));
      if (!worldVp.intersects(edgeRect))
      {
        continue;
      }

      pen.setColor(nodesColor[i]);
      iPainter.setPen(pen);

      QTransform trans;
      trans.translate(nodes[i].x(), nodes[i].y());
      trans = trans.rotate(180, Qt::ZAxis);
      trans = trans.scale(-0.1, 0.1);
      iPainter.setTransform(trans * worldToScreen);
      iPainter.drawText(QPointF(), nodeDesc[i]);
      iPainter.setTransform(worldToScreen);

      iPainter.drawEllipse(nodes[i], s_NodeSize, s_NodeSize);
    }

    for (uint32_t i = 0; i < edges.size(); ++i)
    {
      QRect edgeRect(edges[i].first.toPoint(), QSize(1, 1));
      edgeRect |= QRect(edges[i].second.toPoint(), QSize(1, 1));
      if (!worldVp.intersects(edgeRect))
      {
        continue;
      }

      pen.setColor(edgesColor[i]);
      iPainter.setPen(pen);

      iPainter.drawLine(edges[i].first, edges[i].second);

      QPointF middle = (edges[i].first + edges[i].second) * 0.5;

      QTransform trans;
      trans.translate(middle.x(), middle.y());
      trans = trans.rotate(180, Qt::ZAxis);
      trans = trans.scale(-0.1, 0.1);
      iPainter.setTransform(trans * worldToScreen);
      //iPainter.drawText(QPointF(), edgeDesc[i]);
      iPainter.setTransform(worldToScreen);
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