#pragma once

#include <QWidget>

#include <engine/common/world.hpp>

#include <editor/editorstate.hpp>
#include <math/vector2.hpp>

namespace eXl
{
  class MapResource;

  class MapEditor : public ResourceEditor
  {
    Q_OBJECT
  public:
    struct Impl;

    static ResourceEditorHandler& GetEditorHandler();

    MapEditor(QWidget* iParent, DocumentState* iArchetypeDoc);

    void Cleanup() override;

    void CommitDocument() override;

    ObjectHandle Place(Resource::UUID const& iUUID, Name iSubobject, Vector2i const& iPos);

  protected:

    std::unique_ptr<Impl> m_Impl;
  };
}