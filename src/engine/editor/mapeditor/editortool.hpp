#pragma once

#include <editor/gamewidget.hpp>
#include <editor/editordef.hpp>
#include <engine/common/world.hpp>

#include <QWidget>

namespace eXl
{
  class ToolEditorComponentMgr : public ComponentManager
  {
  public:
    DECLARE_RTTI(ToolEditorComponentMgr, ComponentManager);

    using ComponentManager::CreateComponent;
  };

  class EditorTool : public QWidget
  {
    Q_OBJECT
  public:

    EditorTool(QWidget* iParent, World& iWorld);

    virtual void EnableTool() {}
    virtual void DisableTool() {}

  Q_SIGNALS:
    void onToolChanged(QObject*, GameWidget::PainterInterface*);
    void onEditDone();

  protected:
    ToolEditorComponentMgr& m_EditorMgr;
  };
}