#pragma once

#include <QWidget>

#include <editor/editorstate.hpp>

namespace eXl
{
  class ModelEditor : public ResourceEditor
  {
    Q_OBJECT
  public:

    static ResourceEditorHandler& GetEditorHandler();

    ModelEditor(QWidget* iParent, DocumentState* iTilesetDoc);

    void Cleanup() override;

  protected:

    struct Impl;

    std::unique_ptr<Impl> m_Impl;
  };
}