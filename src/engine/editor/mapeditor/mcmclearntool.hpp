#pragma once

#include "editortool.hpp"
#include "mapeditor.hpp"

#include "commonwidgets.hpp"
#include "utils.hpp"
#include "../miniaturecache.hpp"

#include <gen/mcmcsynthesis.hpp>
#include <engine/map/mcmcmodelrsc.hpp>

class QListWidget;
class QAction;
class QSpinBox;
class QComboBox;
class QCheckBox;
class ResourceHandle_Editor;

namespace eXl
{
  

  class MCMCModelRsc;
  struct TempElementInfo;

  class MCMCLearnTool : public EditorTool
  {
    Q_OBJECT
  public:
    
    MCMCLearnTool(QWidget* iParent, World& iWorld, MapEditor* iEditor);
    ~MCMCLearnTool();

    void EnableTool() override;
    void DisableTool() override;

    struct MCMCTag
    {};
    
  protected:

    void ReadElements();
    void GetTileInfo(ResourceHandle<Tileset> const& iHandle, TileName iName, TempElementInfo& oInfo);
    void GetTileInfo(ResourceHandle<Archetype> const& iArchetype, TempElementInfo& oInfo);

    void TrainModel();
    void RunModel();

    World& m_World;
    Vector<MCMC2D::Element> m_Elements;
    Vector<MCMCModelRsc::ElementRef> m_Ref;
    Vector<bool> m_Enabled;
    Vector<ObjectHandle> m_RunSpacesHandle;
    Vector<Polygoni> m_RunSpaces;
    MapEditor* m_ParentEditor;
    MiniatureCache m_Cache;
    DenseGameDataStorage<MCMCTag> m_MCMCData;

    QListWidget* m_ElementsDisplay;
    QSpinBox* m_ShapeScale;
    QSpinBox* m_ShapeShrink;
    QSpinBox* m_InteractionRadius;
    QCheckBox* m_UseQuantileCull;
    ResourceHandle_Editor* m_CurModelSelector;
    QSpinBox* m_RunIter;
    QCheckBox* m_CleanRun;
    QListWidget* m_RunSpaceList;

    std::unique_ptr<Random> m_Rand;
  };
}