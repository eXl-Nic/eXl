#pragma once

#include <editor/gamewidget.hpp>

#include <QWidget>

namespace eXl
{
  class EditorTool : public QWidget
  {
    Q_OBJECT
  public:

    EditorTool(QWidget* iParent);

    virtual void EnableTool() {}
    virtual void DisableTool() {}

  Q_SIGNALS:
    void onToolChanged(QObject*, GameWidget::PainterInterface*);
    void onEditDone();
  };
}