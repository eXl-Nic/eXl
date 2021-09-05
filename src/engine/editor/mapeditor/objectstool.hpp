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

		struct PlacedObject
		{
      CustomizationData m_CustoData;

      AABB2Di m_BoxCache;
		};

    static PropertySheetName ToolDataName();

		void Initialize(MapResource const& iMap);
		
		void EnableTool() override;
		void DisableTool() override;

    GameDataView<MapResource::ObjectHeader> const& GetObjects() const { return m_ObjectsView; }
    GameDataStorage<PlacedObject> const& GetObjectsAdditionalData() const { return m_ObjectsEditorData; }

    ObjectHandle AddAt(Archetype const* iArchetype, Vector2i iPixelPos);

    void Cleanup(ObjectHandle);

	protected:

		void CleanupToolConnections();

		void ChangeTool(CurTool iTool, bool iForceRefresh = false);
		void SelectObject(ObjectHandle);

		void ClearPenToolTile();
		void SetupPenTool();
		void SetPenToolTile();
    void SetupEraserTool();

		void SetupSelectionTool();

		ObjectHandle AddAt(Vector2i iPixelPos);
		ObjectHandle GetAt(Vector2i iWorldPos);
    void Remove(ObjectHandle iObject);

		void AddToWorld(ObjectHandle, MapResource::ObjectHeader const& iObject);
    void UpdateObjectBoxAndTile(ObjectHandle, MapResource::ObjectHeader& iObject);

    void AddTileToWorld(ObjectHandle iObj, GfxSpriteComponent::Desc const* iDesc);

    Archetype const* m_SelectedArchetype = nullptr;

		Tools m_Tools;
		Vector<QMetaObject::Connection> m_ToolConnections;

		World& m_World;
    GameDataView<MapResource::ObjectHeader>& m_ObjectsView;
    GameDataStorage<PlacedObject> m_ObjectsEditorData;
		Vector<BoxIndexEntry> m_ResultsCache;
		BoxIndex m_TilesIdx;
		UnorderedMap<uint64_t, ObjectHandle> m_IDs;
    ObjectHandle m_SelectionHandle;
		MapResource::ObjectHeader* m_Selection = nullptr;
		QAction* m_Actions[3];
		QIcon m_Icons[3];
		QIcon m_HIcons[3];

		QWidget* m_SelectionSettings;
    QTreeView* m_CustoView;
		QSpinBox* m_SelectionX;
		QSpinBox* m_SelectionY;
	};

  DEFINE_TYPE_EX(ObjectsTool::PlacedObject, ObjectsTool__PlacedObject, )
}