#pragma once

#include <QWidget>

#include <editor/editorstate.hpp>

namespace eXl
{
  class LuaEventHandler;

  class LuaScriptEditor : public ResourceEditor
  {
    Q_OBJECT
  public:

    static ResourceEditorHandler& GetEditorHandler();

    LuaScriptEditor(QWidget* iParent, DocumentState* iArchetypeDoc);

    void Cleanup() override;

  protected:

    struct Impl;

    std::unique_ptr<Impl> m_Impl;
  };
}