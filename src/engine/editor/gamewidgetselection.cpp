#include "gamewidgetselection.hpp"

#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

namespace eXl
{
  bool MouseSelectionFilter::eventFilter(QObject *dest, QEvent *event)
  {
    switch (event->type())
    {
    case QEvent::Leave:
      break;
    case QEvent::Enter:
      break;
    case QEvent::MouseButtonPress:
    {
      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      if (mouseEvent->button() == Qt::LeftButton)
      {
        m_PressPos = Vec2i(mouseEvent->pos().x(), mouseEvent->pos().y());
        m_Pressed = true;

        emit onSelectionStarted(AABB2Di(m_PressPos, One<Vec2i>()));

        return true;
      }
    }
      break;
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    {
      QMouseEvent* mouseEvent = (QMouseEvent*)event;
      bool rightReleased = event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::RightButton;
      bool leftReleased = event->type() == QEvent::MouseButtonRelease && mouseEvent->button() == Qt::LeftButton;
      bool wasPressed = m_Pressed;
      if (m_Pressed)
      {
        Vec2i endPos(mouseEvent->pos().x(), mouseEvent->pos().y());

        m_Selection.m_Data[0].x = Mathi::Min(m_PressPos.x, endPos.x);
        m_Selection.m_Data[0].y = Mathi::Min(m_PressPos.y, endPos.y);
        m_Selection.m_Data[1].x = Mathi::Max(m_PressPos.x, endPos.x);
        m_Selection.m_Data[1].y = Mathi::Max(m_PressPos.y, endPos.y);
        m_Selection.m_Data[1].x = Mathi::Max(m_Selection.m_Data[0].x + 1, m_Selection.m_Data[1].x);
        m_Selection.m_Data[1].y = Mathi::Max(m_Selection.m_Data[0].y + 1, m_Selection.m_Data[1].y);

        m_Pressed = !leftReleased;
      }

      if (rightReleased && m_Pressed)
      {
        m_Pressed = false;
      }

      if (m_Pressed)
      {
        emit onSelectionChanged(m_Selection);
      }
      else if(wasPressed)
      {
        emit onSelectionEnded(m_Selection);
        m_Selection = AABB2Di();
      }
      if (leftReleased)
      {
        return true;
      }
    }
      break;
    
    }

    return false;
  }

  void SelectionPainter::paint(QPainter& iPainter) const
  {
    //QPainter::CompositionMode oldCompositionMode = iPainter.compositionMode();
    //iPainter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
    QPen pen;
    pen.setStyle(Qt::SolidLine);
    pen.setColor(QColor(0, 0, 255));
    pen.setWidth(2);
    
    iPainter.setPen(pen);

    for (auto const& box : m_Boxes)
    {
      Vec2i size = box.GetSize();
      iPainter.drawRect(box.m_Data[0].x, box.m_Data[0].y, size.x, size.y);
    }
    //iPainter.setCompositionMode(oldCompositionMode);
  }
}