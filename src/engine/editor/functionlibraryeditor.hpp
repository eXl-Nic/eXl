#pragma once

#include <QWidget>

#include <engine/common/world.hpp>

#include "editorstate.hpp"

namespace eXl
{
  class LuaFunctionLibrary;

  class LuaFunctionLibraryEditor : public ResourceEditor
  {
    Q_OBJECT
  public:

    static ResourceEditorHandler& GetEditorHandler();

    LuaFunctionLibraryEditor(QWidget* iParent, DocumentState* iArchetypeDoc);

    void Cleanup() override;

  protected:

    struct Impl;

    std::unique_ptr<Impl> m_Impl;
  };
}