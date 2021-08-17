#pragma once

#include <QWidget>

#include <engine/common/world.hpp>

#include <editor/editorstate.hpp>

namespace eXl
{
	class MapResource;

	class MapEditor : public ResourceEditor
	{
		Q_OBJECT
  public:

    static ResourceEditorHandler& GetEditorHandler();

		MapEditor(QWidget* iParent, DocumentState* iArchetypeDoc);

    void Cleanup() override;

	protected:

    void CommitDocument() override;
		
		struct Impl;

    std::unique_ptr<Impl> m_Impl;
	};
}