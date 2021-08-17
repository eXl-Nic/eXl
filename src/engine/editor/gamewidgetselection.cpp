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
        m_PressPos = Vector2i(mouseEvent->pos().x(), mouseEvent->pos().y());
        m_Pressed = true;

        emit onSelectionStarted(AABB2Di(m_PressPos, Vector2i::ONE));

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
        Vector2i endPos(mouseEvent->pos().x(), mouseEvent->pos().y());

        m_Selection.m_Data[0].X() = Mathi::Min(m_PressPos.X(), endPos.X());
        m_Selection.m_Data[0].Y() = Mathi::Min(m_PressPos.Y(), endPos.Y());
        m_Selection.m_Data[1].X() = Mathi::Max(m_PressPos.X(), endPos.X());
        m_Selection.m_Data[1].Y() = Mathi::Max(m_PressPos.Y(), endPos.Y());
        m_Selection.m_Data[1].X() = Mathi::Max(m_Selection.m_Data[0].X() + 1, m_Selection.m_Data[1].X());
        m_Selection.m_Data[1].Y() = Mathi::Max(m_Selection.m_Data[0].Y() + 1, m_Selection.m_Data[1].Y());

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
      Vector2i size = box.GetSize();
      iPainter.drawRect(box.m_Data[0].X(), box.m_Data[0].Y(), size.X(), size.Y());
    }
    //iPainter.setCompositionMode(oldCompositionMode);
  }
}