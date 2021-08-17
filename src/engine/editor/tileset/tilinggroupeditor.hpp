#pragma once

#include <QWidget>

#include <engine/common/world.hpp>
#include <editor/editorstate.hpp>

namespace eXl
{
	class TilingGroup;

	class TilingGroupEditor : public ResourceEditor
	{
		Q_OBJECT
	public:

    static ResourceEditorHandler& GetEditorHandler();

		TilingGroupEditor(QWidget* iParent, DocumentState* iTilesetDoc);

    void Cleanup() override;

	protected:
		
		struct Impl;

		std::unique_ptr<Impl> m_Impl;
	};
}