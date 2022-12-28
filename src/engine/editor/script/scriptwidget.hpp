#pragma once 

#include <lac/editor/LuaEditor.h>
#include <engine/script/eventsystem.hpp>
#include <engine/script/luafunctionlibrary.hpp>

namespace eXl
{
  class ScriptEditorWidget : public lac::editor::LuaEditor
  {
    Q_OBJECT
  public:

    ScriptEditorWidget(QWidget* iParent);

    void SetEngineFunctions(Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps = Vector<ResourceHandle<LuaFunctionLibrary>>(), EventsManifest::FunctionsMap const& iFunctions = EventsManifest::FunctionsMap());

  };
}