#include "editortool.hpp"

namespace eXl
{
  IMPLEMENT_RTTI(ToolEditorComponentMgr)

  EditorTool::EditorTool(QWidget* iParent, World& iWorld)
    : QWidget(iParent)
    , m_EditorMgr(*iWorld.GetSystem<ToolEditorComponentMgr>())
  {}
}