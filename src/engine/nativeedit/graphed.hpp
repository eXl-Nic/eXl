#pragma once 

#include <gen/graphrules.hpp>
#include <math/segment.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luaeventhandler.hpp>


namespace eXl {
  struct GraphEdState {
    RewriteSystem * m_Sys;
    UnorderedMap<Name, ResourceHandle<Archetype> > m_TagsArch;
    UnorderedMap<String, ResourceHandle<LuaEventHandler> > m_RuleScripts;

    int m_CurrentTab = 0;

    char m_NewRuleName[32] = {0};
    String m_CurrentEditedRuleName;
    Rule* m_CurrentEditedRule = nullptr;
    

    //RewriteSystemRsc::RuleAdditionalData* m_RuleAddData = nullptr;
    
    //int m_RuleTabIdx;

    //TagsCollectionModel* m_TagsCollectionModel;
    //QListView* m_TagsCollectionView;
    //ResourceSelectionWidget* m_TagArchetypeSelection;
    //QComboBox* m_TagTypeSelection;

    char m_NewTagName[32] = { 0 };
    Name m_CurrentEditedTagName;
    TagDef* m_CurrentEditedTag = nullptr;
    //RewriteSystemRsc::TagAdditionalData* m_TagAdditionalData = nullptr;

    //int m_TagTabIdx;

    int m_NodesIdx = 0;
    int m_EdgesIdx = 0;

    //QListWidget* m_NodesList;
    int m_NodeCutStart = 0;
    int m_NodeNewStart = 0;
    //QComboBox* m_NodeTagSelection;

    //QListWidget* m_EdgesList;
    int m_EdgeCutStart = 0;
    int m_EdgeNewStart = 0;
    //QComboBox* m_EdgeTagSelection;
    struct DrawInfos {
      void Clear();
      Vector<String> nodeDesc;
      Vector<Vec2> nodes;
      Vector<Vec4> nodesColor;
      Vector<Segment<float> > edges;
      Vector<String> edgeDesc;
      Vector<Vec4> edgesColor;
    };
    DrawInfos m_DrawInfos;
  };

  void Draw(GraphEdState const& iState);
}

