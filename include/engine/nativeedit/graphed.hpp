#pragma once 

#include <gen/graphrules.hpp>
#include <math/segment.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luaeventhandler.hpp>


namespace eXl {
  struct EXL_ENGINE_API GraphEdState {
    RewriteSystem * m_Sys;
    UnorderedMap<Name, ResourceHandle<Archetype> > m_TagsArch;
    UnorderedMap<String, ResourceHandle<LuaEventHandler> > m_RuleScripts;

    enum ItemType {
      Context = 0,
      New = 1,
      Cut = 2
    };

    int m_CurrentTab = 0;

    char m_NewRuleName[32] = {0};
    String m_CurrentEditedRuleName;
    Rule* m_CurrentEditedRule = nullptr;
    uint32_t ComputeCurRuleTotNodes() const;
    uint32_t ComputeCurRuleTotEdges() const;
    ItemType GetNodeType( uint32_t nodeIdx, uint32_t* locIdx = nullptr ) const;
    ItemType GetEdgeType( uint32_t nodeIdx, uint32_t* locIdx = nullptr ) const;
    
    //RewriteSystemRsc::RuleAdditionalData* m_RuleAddData = nullptr;

    char m_NewTagName[32] = { 0 };
    bool m_NewTagForNode = true;
    Name m_CurrentEditedTagName;
    Name m_CurrentSelectedTag;
    TagDef* m_CurrentEditedTag = nullptr;

    int m_NewNodeType = 0;
    int m_NewEdgeType = 0;

    int m_NodeIdx = 0;
    Vector<String> m_nodeNames;
    int m_EdgeIdx = 0;
    Vector<String> m_edgeNames;

    bool IsCurNodeValid() const;
    bool IsCurEdgeValid() const;
    Vector<Name>& GetCurNode(uint32_t& oLocalOffset);
    
    Name& GetCurNodeTag();
    Name GetCurNodeTag() const;
    void SetCurNodeTag(Name iName);
    void EraseCurrentNode();
    void EraseCurrentEdge();

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

    void Draw();

  private :

    String ComputeNodeName(int nodeIdx);
    String ComputeEdgeName(int edgeIdx);

    String ComputeCtxNodeName(int locIdx);
    String ComputeNewNodeName(int locIdx);
    String ComputeCutNodeName(int locIdx);
    String ComputeCtxEdgeName(int locIdx);
    String ComputeNewEdgeName(int locIdx);
    String ComputeCutEdgeName(int locIdx);

    void RemapEdgesNode(uint32_t iNodeIdx, bool iAdded);
    void UpdateDisplay();
    void DrawRulesPanel();
    void DrawTagsPanel();
    void DrawNodesPanel();
    void DrawEdgesPanel();
    void UpdateDisplay(World& iWorld);
  };

  
}

