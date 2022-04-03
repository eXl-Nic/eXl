#include "gridpainter.hpp"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

namespace eXl
{
  void GridPainter::paint(QPainter& iPainter) const
  {
    QRect region = iPainter.viewport();
    QRectF regionF = region;
    QTransform screenToWorld = GetScreenToWorldTransform();

    QRectF worldRegion = screenToWorld.mapRect(regionF);

    Vec2i min(worldRegion.left() / m_CellSize.x, worldRegion.bottom() / m_CellSize.y);
    Vec2i max(worldRegion.right() / m_CellSize.x, worldRegion.top() / m_CellSize.y);

    std::swap(min.y, max.y);

    min -= One<Vec2i>();
    max += One<Vec2i>();

    Vec2 minF(min.x * m_CellSize.x, min.y * m_CellSize.y);
    Vec2 maxF(max.x * m_CellSize.x, max.y * m_CellSize.y);

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColor(0, 0, 0));
    pen.setWidth(1);
    
    iPainter.setPen(pen);

    QTransform worldToScreen = GetWorldToScreenTransform();
    for (int x = 0; x < max.x - min.x; ++x)
    {
      float xCoord = minF.x + x * m_CellSize.x;
      QLine line;
      
      line.setP1(worldToScreen.map(QPoint(xCoord, minF.y)));
      line.setP2(worldToScreen.map(QPoint(xCoord, maxF.y)));
      iPainter.drawLine(line);
    }

    for (int y = 0; y < max.y - min.y; ++y)
    {
      float yCoord = minF.y + y * m_CellSize.y;
      QLine line;
      line.setP1(worldToScreen.map(QPoint(minF.x, yCoord)));
      line.setP2(worldToScreen.map(QPoint(maxF.x, yCoord)));
      iPainter.drawLine(line);
    }
  }
}