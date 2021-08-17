#pragma once

#include "editortool.hpp"

#include <core/refcobject.hpp>
#include <engine/game/archetype.hpp>
#include <engine/map/map.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include "utils.hpp"

#include <QIcon>

class QAction;
class QSpinBox;
class QTreeView;

namespace eXl
{
	class MouseSelectionFilter;
	class PenToolFilter;
	class TileSelectionWidget;

	class ObjectsTool : public EditorTool
	{
		Q_OBJECT
	public:
		enum CurTool
		{
			None, // Selection/Move
			PenTool,
			EraserTool
		};

		struct Tools
		{
			MouseSelectionFilter* m_Selection;
			PenToolFilter* m_Pen;
		};

		ObjectsTool(QWidget* parent, Tools iTools, World& iWorld);

		CurTool m_Tool = None;

		class PlacedObject : public HeapObject
		{
			DECLARE_RefC;
		public:

			Vector2i m_Position;
			ResourceHandle<Archetype> m_Archetype;
			uint64_t m_UUID;
			uint32_t m_Index;

      CustomizationData m_CustoData;

      AABB2Di m_BoxCache;

			ObjectHandle m_WorldObject;
		};

		void Initialize(MapResource const& iMap);
		

		void EnableTool() override;
		void DisableTool() override;

		UnorderedMap<uint32_t, IntrusivePtr<PlacedObject>> const& GetObjects() const { return m_Objects; }

	protected:

		void CleanupToolConnections();

		void ChangeTool(CurTool iTool, bool iForceRefresh = false);
		void SelectObject(PlacedObject* iObject);

		void ClearPenToolTile();
		void SetupPenTool();
		void SetPenToolTile();
    void SetupEraserTool();

		void SetupSelectionTool();

		PlacedObject* AddAt(Vector2i iPixelPos, bool iAppend);
		PlacedObject* GetAt(Vector2i iWorldPos);
    void Remove(PlacedObject& iObject);

		void AddToWorld(PlacedObject&);
    void UpdateObjectBoxAndTile(PlacedObject&);
		void RemoveFromWorld(PlacedObject&);

    void AddTileToWorld(ObjectHandle iObj, GfxSpriteComponent::Desc const* iDesc);

    Archetype const* m_SelectedArchetype = nullptr;

		Tools m_Tools;
		Vector<QMetaObject::Connection> m_ToolConnections;

		World& m_World;
		Vector<BoxIndexEntry> m_ResultsCache;
		BoxIndex m_TilesIdx;
		UnorderedMap<uint32_t, IntrusivePtr<PlacedObject>> m_Objects;
		UnorderedMap<uint64_t, uint32_t> m_IDs;
		uint32_t m_Counter = 0;
		PlacedObject* m_Selection = nullptr;
		QAction* m_Actions[3];
		QIcon m_Icons[3];
		QIcon m_HIcons[3];

		QWidget* m_SelectionSettings;
    QTreeView* m_CustoView;
		QSpinBox* m_SelectionX;
		QSpinBox* m_SelectionY;
	};
}