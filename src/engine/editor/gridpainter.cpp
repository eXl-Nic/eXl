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

    Vector2i min(worldRegion.left() / m_CellSize.X(), worldRegion.bottom() / m_CellSize.Y());
    Vector2i max(worldRegion.right() / m_CellSize.X(), worldRegion.top() / m_CellSize.Y());

    std::swap(min.Y(), max.Y());

    min -= Vector2i::ONE;
    max += Vector2i::ONE;

    Vector2f minF(min.X() * m_CellSize.X(), min.Y() * m_CellSize.Y());
    Vector2f maxF(max.X() * m_CellSize.X(), max.Y() * m_CellSize.Y());

    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColor(0, 0, 0));
    pen.setWidth(1);
    
    iPainter.setPen(pen);

    QTransform worldToScreen = GetWorldToScreenTransform();
    for (int x = 0; x < max.X() - min.X(); ++x)
    {
      float xCoord = minF.X() + x * m_CellSize.X();
      QLine line;
      
      line.setP1(worldToScreen.map(QPoint(xCoord, minF.Y())));
      line.setP2(worldToScreen.map(QPoint(xCoord, maxF.Y())));
      iPainter.drawLine(line);
    }

    for (int y = 0; y < max.Y() - min.Y(); ++y)
    {
      float yCoord = minF.Y() + y * m_CellSize.Y();
      QLine line;
      line.setP1(worldToScreen.map(QPoint(minF.X(), yCoord)));
      line.setP2(worldToScreen.map(QPoint(maxF.X(), yCoord)));
      iPainter.drawLine(line);
    }
  }
}