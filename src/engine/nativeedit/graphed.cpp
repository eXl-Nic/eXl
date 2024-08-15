#include <engine/nativeedit/graphed.hpp>
#include <core/resource/resourcemanager.hpp>
#include <engine/common/gamedatabase.hpp>

#include <imgui.h>

namespace eXl 
{ 

  void GraphEdState::DrawRulesPanel()
  {
    if (ImGui::BeginPopupModal("RuleNameInput")) 
    {
      ImGui::InputText("Name", m_NewRuleName, sizeof(m_NewRuleName));
      if (ImGui::Button("Ok")) 
      {
        String newRuleName(m_NewRuleName);
        m_NewRuleName[0] = 0;
        auto iter = m_Sys->m_Rules.find(newRuleName);
        if ( !newRuleName.empty() && iter == m_Sys->m_Rules.end())
        {
          m_Sys->m_Rules.insert(std::make_pair(newRuleName, Rule()));
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if( ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::Button("+")) 
    {
      ImGui::OpenPopup("RuleNameInput");
    }
    ImGui::SameLine();

    if (ImGui::Button("-")) 
    {
      if (m_CurrentEditedRule != nullptr)
      {
        m_Sys->m_Rules.erase(m_CurrentEditedRuleName);
        m_CurrentEditedRule = nullptr;
        m_CurrentEditedRuleName.clear();
      }
    }

    if (ImGui::BeginListBox("Rules")) 
    {
      for (auto& ruleEntry : m_Sys->m_Rules) {
        bool selected = m_CurrentEditedRuleName == ruleEntry.first;
        if (ImGui::Selectable(ruleEntry.first.c_str(), &selected)) {
          m_CurrentEditedRuleName = ruleEntry.first;
          m_CurrentEditedRule = &ruleEntry.second;
        }
      }

      ImGui::EndListBox();

    }
    
    auto scriptEntry = m_RuleScripts.find( m_CurrentEditedRuleName );
    const Resource::UUID* uuid = scriptEntry != m_RuleScripts.end() ? &scriptEntry->second.GetUUID() : nullptr;

    const Resource::Header* selRsc = uuid != nullptr ? ResourceManager::GetHeader(*uuid) : nullptr;

    if (ImGui::BeginCombo("Rule Script Selector", selRsc != nullptr ? selRsc->m_ResourceName.c_str() : "<none>"))
    {
      Vector<Resource::Header> resources = ResourceManager::ListResources(LuaEventHandler::StaticLoaderName());

      if (ImGui::Selectable("<none>", scriptEntry == m_RuleScripts.end())) {
        m_RuleScripts.erase(scriptEntry);
      }
      if (m_CurrentEditedRule != nullptr) {
        for (const auto& rsc : resources) {
          bool selected = uuid != nullptr && *uuid == rsc.m_ResourceId;
          if (ImGui::Selectable(rsc.m_ResourceName.c_str(), &selected)) {
            ResourceHandle< LuaEventHandler > handle;
            handle.SetUUID(rsc.m_ResourceId);
            m_RuleScripts.insert(std::make_pair(m_CurrentEditedRuleName, handle));
          }
        }
      }

      ImGui::EndCombo();
    }
  }

  void GraphEdState::DrawTagsPanel()
  {
    if (ImGui::BeginPopupModal("TagNameInput"))
    {
      ImGui::InputText("Name", m_NewTagName, sizeof(m_NewTagName));
      ImGui::Checkbox("Tag For Node", &m_NewTagForNode);
      if (ImGui::Button("Ok"))
      {
        Name newTagName(m_NewTagName);
        
        m_NewTagName[0] = 0;
        auto iter = m_Sys->m_Tags.find(newTagName);
        if (!newTagName.get().empty() && iter == m_Sys->m_Tags.end())
        {
          TagDef& newDef = m_Sys->m_Tags.insert(std::make_pair(newTagName, TagDef())).first->second;
          newDef.m_IsNodeTag = m_NewTagForNode;
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::Button("+"))
    {
      ImGui::OpenPopup("TagNameInput");
    }
    ImGui::SameLine();

    if (ImGui::Button("-"))
    {
      if (m_CurrentEditedTag != nullptr)
      {
        m_Sys->m_Tags.erase(m_CurrentEditedTagName);
        m_CurrentEditedTag = nullptr;
        m_CurrentEditedTagName = Name();
      }
    }

    if (ImGui::BeginListBox("TagsList"))
    {
      for (auto& tagEntry : m_Sys->m_Tags) {
        bool selected = m_CurrentEditedTagName == tagEntry.first;
        if (ImGui::Selectable(tagEntry.first.c_str(), &selected)) {
          m_CurrentEditedTagName = tagEntry.first;
          m_CurrentEditedTag = &tagEntry.second;
        }
      }

      ImGui::EndListBox();
    }

    auto archetypeEntry = m_TagsArch.find(m_CurrentEditedTagName);
    const Resource::UUID* uuid = archetypeEntry != m_TagsArch.end() ? &archetypeEntry->second.GetUUID() : nullptr;

    const Resource::Header* selRsc = uuid != nullptr ? ResourceManager::GetHeader(*uuid) : nullptr;

    if (ImGui::BeginCombo("Tag Archetype Selector", selRsc != nullptr ? selRsc->m_ResourceName.c_str() : "<none>"))
    {
      Vector<Resource::Header> resources = ResourceManager::ListResources(Archetype::StaticLoaderName());
      if (ImGui::Selectable("<none>", archetypeEntry == m_TagsArch.end())) 
      {
        m_TagsArch.erase(archetypeEntry);
      }
      if (m_CurrentEditedTag != nullptr) 
      {
        for (const auto& rsc : resources) 
        {
          bool selected = uuid != nullptr && *uuid == rsc.m_ResourceId;
          if (ImGui::Selectable(rsc.m_ResourceName.c_str(), &selected)) 
          {
            ResourceHandle< LuaEventHandler > handle;
            handle.SetUUID(rsc.m_ResourceId);
            m_RuleScripts.insert(std::make_pair(m_CurrentEditedRuleName, handle));
          }
        }
      }

      ImGui::EndCombo();
    }

  }

  uint32_t GraphEdState::ComputeCurRuleTotNodes() const {
    return m_CurrentEditedRule->m_ContextNodes.size()
      + m_CurrentEditedRule->m_CreateNodes.size()
      + m_CurrentEditedRule->m_CutNodes.size();
  }

  uint32_t GraphEdState::ComputeCurRuleTotEdges() const {
      return m_CurrentEditedRule->m_ContextEdges.size()
          + m_CurrentEditedRule->m_NewEdge.size()
          + m_CurrentEditedRule->m_CutEdge.size();
  }
  
  bool GraphEdState::IsCurNodeValid() const {
    if (m_NodeIdx >= 0) {
      if (m_NodeIdx < ComputeCurRuleTotNodes())
      {
        return true;
      }
    }
    return false;
  }

  bool GraphEdState::IsCurEdgeValid() const {
      if (m_EdgeIdx >= 0) {
          if (m_EdgeIdx < ComputeCurRuleTotEdges())
          {
              return true;
          }
      }
      return false;
  }

  Vector<Name>& GraphEdState::GetCurNode(uint32_t& oLocalOffset) {
    ItemType type = GetNodeType( m_NodeIdx, &oLocalOffset );
    switch (type) {
    case Context : 
      return m_CurrentEditedRule->m_ContextNodes;
    case New:
      return m_CurrentEditedRule->m_CreateNodes;
    case Cut:
        return m_CurrentEditedRule->m_CutNodes;
    }
    static Vector<Name> dummy;
    return dummy;
  }

  Name& GraphEdState::GetCurNodeTag() {
    uint32_t localIdx;
    Vector<Name>& actualArray = GetCurNode(localIdx);
    return actualArray[localIdx];
  }

  Name GraphEdState::GetCurNodeTag() const {
    return const_cast<GraphEdState*>(this)->GetCurNodeTag();
  }

  void GraphEdState::SetCurNodeTag(Name iName) {
    GetCurNodeTag() = iName;
  }

  void GraphEdState::EraseCurrentEdge() {
      if (IsCurEdgeValid()) {
          uint32_t locIdx;
          ItemType type = GetEdgeType(m_NodeIdx, &locIdx);
          switch (type) {
          case Context:
            m_CurrentEditedRule->m_ContextEdges.erase(m_CurrentEditedRule->m_ContextEdges.begin() + locIdx);
            break;
          case New:
            m_CurrentEditedRule->m_NewEdge.erase(m_CurrentEditedRule->m_NewEdge.begin() + locIdx);
            break;
          case Cut:
            m_CurrentEditedRule->m_CutEdge.erase(m_CurrentEditedRule->m_CutEdge.begin() + locIdx);

            for (int i = 0; i < m_CurrentEditedRule->m_NewEdge.size(); ++i)
            {
              Rule::NewEdge& edge = m_CurrentEditedRule->m_NewEdge[i];
              if (edge.port[0] == m_EdgeIdx) {
                edge.port[0] = -1;
              }
              else if (edge.port[0] > m_EdgeIdx) {
                --edge.port[0];
              }
              if (edge.port[1] == m_EdgeIdx) {
                edge.port[1] = -1;
              }
              else if (edge.port[1] > m_EdgeIdx) {
                --edge.port[1];
              }
            }
            break;
          }
          m_EdgeIdx = -1;
      }
  }

  void GraphEdState::RemapEdgesNode(uint32_t iNodeIdx, bool iAdded)
  {
    if (!iAdded)
    {
      auto cleanupEdges = [iNodeIdx](auto& iCollection)
        {
          for (int i = 0; i < iCollection.size(); ++i)
          {
            auto& edge = iCollection[i];
            if (edge.nodes[0] == iNodeIdx
              || edge.nodes[1] == iNodeIdx)
            {
              iCollection.erase(iCollection.begin() + i);
              --i;
              continue;
            }
            if (edge.nodes[0] > iNodeIdx)
            {
              --edge.nodes[0];
            }
            if (edge.nodes[1] > iNodeIdx)
            {
              --edge.nodes[1];
            }
          }
        };

      cleanupEdges(m_CurrentEditedRule->m_ContextEdges);
      cleanupEdges(m_CurrentEditedRule->m_CutEdge);
      cleanupEdges(m_CurrentEditedRule->m_NewEdge);
    }
    else
    {
      auto shiftEdges = [iNodeIdx](auto& iCollection)
        {
          for (int i = 0; i < iCollection.size(); ++i)
          {
            auto& edge = iCollection[i];
            if (edge.nodes[0] >= iNodeIdx)
            {
              ++edge.nodes[0];
            }
            if (edge.nodes[1] >= iNodeIdx)
            {
              ++edge.nodes[1];
            }
          }
        };

      shiftEdges(m_CurrentEditedRule->m_ContextEdges);
      shiftEdges(m_CurrentEditedRule->m_CutEdge);
      shiftEdges(m_CurrentEditedRule->m_NewEdge);
    }
  }

  void GraphEdState::EraseCurrentNode() {

    if (IsCurNodeValid())
    {
      RemapEdgesNode(m_NodeIdx, false);
      uint32_t localIdx;
      Vector<Name>& actualArray = GetCurNode(localIdx);
      actualArray.erase(actualArray.begin() + localIdx);
      m_NodeIdx = -1;
    }
  }

  void GraphEdState::DrawNodesPanel()
  {
    if (ImGui::BeginPopupModal("NewNode"))
    {
      const char* typeNames[] = { "Context", "New", "Cut" };

      if (ImGui::BeginCombo("Node Type", typeNames[m_NewNodeType])) 
      {
        for (int i = 0; i < 3; ++i) 
        {
          bool selected = m_NewNodeType == i;
          if (ImGui::Selectable(typeNames[i])) {
            m_NewNodeType = i;
          }
        }
        ImGui::EndCombo();
      }
      
      if (ImGui::BeginCombo("Tag", m_CurrentSelectedTag == Name() ? "<None>" : m_CurrentSelectedTag.c_str()))
      {
        if (m_CurrentSelectedTag == Name()) {
          ImGui::Selectable("<None>");
        }
        for ( auto tag : m_Sys->m_Tags )
        {
          if (tag.second.m_IsNodeTag) {
            bool selected = tag.first == m_CurrentSelectedTag;
            if (ImGui::Selectable(tag.first.c_str())) {
              m_CurrentSelectedTag = tag.first;
            }
          }
        }
        ImGui::EndCombo();
      }
      
      if (ImGui::Button("Ok"))
      {
        if (m_CurrentSelectedTag != Name()) {

          int insertionPoint;
          switch (m_NewNodeType)
          {
          case 0:
            insertionPoint = m_CurrentEditedRule->m_ContextNodes.size();
            m_CurrentEditedRule->m_ContextNodes.push_back( m_CurrentSelectedTag );
            break;
          case 1:
            insertionPoint = m_CurrentEditedRule->m_ContextNodes.size() + m_CurrentEditedRule->m_CreateNodes.size();
            m_CurrentEditedRule->m_CreateNodes.push_back( m_CurrentSelectedTag );
            break;
          case 2:
            insertionPoint = ComputeCurRuleTotNodes();
            m_CurrentEditedRule->m_CutNodes.push_back( m_CurrentSelectedTag );
            break;
          }
          RemapEdgesNode( insertionPoint, true );
          m_CurrentSelectedTag = Name();
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
      {
        m_CurrentSelectedTag = Name();
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::Button("+"))
    {
      ImGui::OpenPopup("NewNode");
    }
    ImGui::SameLine();

    if (ImGui::Button("-"))
    {
      EraseCurrentNode();
    }

    if (ImGui::BeginListBox("NodesList"))
    {
      uint32_t nodeIdx = 0;
      uint32_t locIdx = 0;
      for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextNodes.size(); ++i ) {
        bool selected = m_NodeIdx == nodeIdx;
        String nodeName = ComputeCtxNodeName(locIdx);
        if (ImGui::Selectable(nodeName.c_str(), &selected)) {
          m_NodeIdx = nodeIdx;
        }
        ++nodeIdx;
        ++locIdx;
      }
      locIdx = 0;
      for (auto& newNode : m_CurrentEditedRule->m_CreateNodes) {
        bool selected = m_NodeIdx == nodeIdx;
        String nodeName = ComputeNewNodeName(locIdx);
        if (ImGui::Selectable(nodeName.c_str(), &selected)) {
          m_NodeIdx = nodeIdx;
        }
        ++nodeIdx;
        ++locIdx;
      }
      locIdx = 0;
      for (auto& curNode : m_CurrentEditedRule->m_CutNodes) {
        bool selected = m_NodeIdx == nodeIdx;
        String nodeName = ComputeCutNodeName(locIdx);
        if (ImGui::Selectable(nodeName.c_str(), &selected)) {
          m_NodeIdx = nodeIdx;
        }
        ++nodeIdx;
        ++locIdx;
      }

      ImGui::EndListBox();
    }
  }

  GraphEdState::ItemType GraphEdState::GetNodeType( uint32_t nodeIdx, uint32_t* locIdx) const 
  {
    ItemType type;
    if (nodeIdx > m_CurrentEditedRule->m_ContextNodes.size())
    {
      nodeIdx -= m_CurrentEditedRule->m_ContextNodes.size();
      if (nodeIdx > m_CurrentEditedRule->m_CreateNodes.size())
      {
        nodeIdx -= m_CurrentEditedRule->m_CreateNodes.size();
        type = Cut;
      }
      else
      {
        type = New;
      }
    }
    else
    {
      type = Context;
    }
    if (locIdx != nullptr) {
      *locIdx = nodeIdx;
    }
    return type;
  }

  GraphEdState::ItemType GraphEdState::GetEdgeType(uint32_t edgeIdx, uint32_t* locIdx) const {
    ItemType type;
    if (edgeIdx > m_CurrentEditedRule->m_ContextEdges.size())
    {
      edgeIdx -= m_CurrentEditedRule->m_ContextEdges.size();
      if (edgeIdx > m_CurrentEditedRule->m_NewEdge.size())
      {
        edgeIdx -= m_CurrentEditedRule->m_NewEdge.size();
        type = Cut;
      }
      else
      {
        type = New;
      }
    }
    else
    {
      type = Context;
    }
    if (locIdx != nullptr) {
      *locIdx = edgeIdx;
    }
    return type;
  }

  String GraphEdState::ComputeNodeName(int iNodeIdx) {
    uint32_t locIdx;
    ItemType type = GetNodeType(iNodeIdx, &locIdx);
    switch (type) {
    case Context:
      return ComputeCtxNodeName(locIdx);
    case New:
      return ComputeNewNodeName(locIdx);
    case Cut:
      return ComputeCutNodeName(locIdx);
    }
    return "";
  }

  String GraphEdState::ComputeCtxNodeName(int locIdx) {
    return "ContextNode " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_ContextNodes[locIdx].c_str());
  }

  String GraphEdState::ComputeNewNodeName(int locIdx) {
    return "NewNode " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_CreateNodes[locIdx].c_str());
  }

  String GraphEdState::ComputeCutNodeName(int locIdx) {
    return "CutNode " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_CutNodes[locIdx].c_str());
  }

  String GraphEdState::ComputeEdgeName(int iEdgeIdx) {
    uint32_t locIdx;
    ItemType type = GetEdgeType(iEdgeIdx, &locIdx);
    switch (type) {
    case Context:
      return ComputeCtxEdgeName(locIdx);
    case New:
      return ComputeNewEdgeName(locIdx);
    case Cut:
      return ComputeCutEdgeName(locIdx);
    }
    return "";
  }

  String GraphEdState::ComputeCtxEdgeName(int locIdx) {
    return "ContextEdge " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_ContextEdges[locIdx].tag.c_str());
  }

  String GraphEdState::ComputeNewEdgeName(int locIdx) {
    return "NewEdge " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_NewEdge[locIdx].tag.c_str());
  }

  String GraphEdState::ComputeCutEdgeName(int locIdx) {
    return "CutEdge " + StringUtil::FromInt(locIdx) + " : " + String(m_CurrentEditedRule->m_CutEdge[locIdx].tag.c_str());
  }

  void GraphEdState::DrawEdgesPanel()
  {
    if (ImGui::BeginPopupModal("NewEdge"))
    {
      const char* typeNames[] = { "Context", "New", "Cut" };

      if (ImGui::BeginCombo("Edge Type", typeNames[m_NewNodeType]))
      {
        for (int i = 0; i < 3; ++i)
        {
          bool selected = m_NewEdgeType == i;
          if (ImGui::Selectable(typeNames[i])) {
            m_NewEdgeType = i;
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::BeginCombo("Tag", m_CurrentSelectedTag == Name() ? "<None>" : m_CurrentSelectedTag.c_str()))
      {
        if (m_CurrentSelectedTag == Name()) {
          ImGui::Selectable("<None>");
        }
        for (auto tag : m_Sys->m_Tags)
        {
          if (!tag.second.m_IsNodeTag) {
            bool selected = tag.first == m_CurrentSelectedTag;
            if (ImGui::Selectable(tag.first.c_str())) {
              m_CurrentSelectedTag = tag.first;
            }
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::Button("Ok"))
      {
        if (m_CurrentSelectedTag != Name()) {

          int insertionPoint;
          switch (m_NewEdgeType)
          {
          case Context:
          {
            insertionPoint = m_CurrentEditedRule->m_ContextEdges.size();
            Rule::Edge ctxEdge;
            ctxEdge.nodes[0] = ctxEdge.nodes[1] = 0;
            ctxEdge.tag = m_CurrentSelectedTag;
            m_CurrentEditedRule->m_ContextEdges.push_back(ctxEdge);
          }
            break;
          case New:
          {
            insertionPoint = m_CurrentEditedRule->m_ContextEdges.size() + m_CurrentEditedRule->m_NewEdge.size();
            Rule::NewEdge newEdge;
            newEdge.nodes[0] = newEdge.nodes[1] = 0;
            newEdge.port[0] = newEdge.port[1] = -1;
            newEdge.tag = m_CurrentSelectedTag;
            m_CurrentEditedRule->m_NewEdge.push_back(newEdge);
          }
            break;
          case Cut:
          {
            Rule::Edge cutEdge;
            cutEdge.nodes[0] = cutEdge.nodes[1] = 0;
            cutEdge.tag = m_CurrentSelectedTag;
            insertionPoint = ComputeCurRuleTotEdges();
            m_CurrentEditedRule->m_CutEdge.push_back(cutEdge);
          }
            break;
          }
          m_CurrentSelectedTag = Name();
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel"))
      {
        m_CurrentSelectedTag = Name();
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    if (ImGui::Button("+"))
    {
      ImGui::OpenPopup("NewEdge");
    }
    ImGui::SameLine();

    if (ImGui::Button("-"))
    {
      EraseCurrentNode();
    }

    if (ImGui::BeginListBox("EdgesList"))
    {
      int edgeIdx = 0;
      int locEdgeIdx = 0;
      for (auto& ctxEdge : m_CurrentEditedRule->m_ContextEdges) {
        bool selected = m_EdgeIdx == edgeIdx;
        String edgeName = ComputeCtxEdgeName(locEdgeIdx);
        if (ImGui::Selectable(edgeName.c_str(), &selected)) {
          m_EdgeIdx = edgeIdx;
        }
        ++locEdgeIdx;
        ++edgeIdx;
      }
      locEdgeIdx = 0;
      for (auto& newEdge : m_CurrentEditedRule->m_NewEdge) {
        bool selected = m_EdgeIdx == edgeIdx;
        String edgeName = ComputeNewEdgeName(locEdgeIdx);
        if (ImGui::Selectable(edgeName.c_str(), &selected)) {
          m_EdgeIdx = edgeIdx;
        }
        ++locEdgeIdx;
        ++edgeIdx;
      }
      locEdgeIdx = 0;
      for (auto& cutEdge : m_CurrentEditedRule->m_CutEdge) {
        bool selected = m_EdgeIdx == edgeIdx;
        String edgeName = ComputeCutEdgeName(locEdgeIdx);
        if (ImGui::Selectable(edgeName.c_str(), &selected)) {
          m_EdgeIdx = edgeIdx;
        }
        ++locEdgeIdx;
        ++edgeIdx;
      }

      ImGui::EndListBox();
    }

    if (IsCurEdgeValid()) {

      const uint32_t newNodesOffset = m_CurrentEditedRule->m_ContextNodes.size();
      const uint32_t cutNodesOffset = m_CurrentEditedRule->m_ContextNodes.size() + m_CurrentEditedRule->m_CreateNodes.size();
      const uint32_t cutEdgesOffset = m_CurrentEditedRule->m_ContextEdges.size() + m_CurrentEditedRule->m_NewEdge.size();
      const uint32_t numNodes = ComputeCurRuleTotNodes();

      uint32_t locIdx;
      const ItemType edgeType = GetEdgeType(m_EdgeIdx, &locIdx);

      const char* nodeNames[] = { "Node 1", "Node 2" };

      auto selNodeEdge = [this, nodeNames, numNodes]< typename EdgeType > (EdgeType & edge, int idx, uint32_t allowedTypes) {
        if (ImGui::BeginCombo(nodeNames[idx], ComputeNodeName(edge.nodes[idx]).c_str())) {
          for (uint32_t i = 0; i < numNodes; ++i) {
            ItemType type = GetNodeType(i);
            if (((1 << type) & allowedTypes) != 0 &&
              i != edge.nodes[0] && i != edge.nodes[1]) {
              if (ImGui::Selectable(ComputeNodeName(i).c_str(), edge.nodes[idx] == i)) {
                edge.nodes[idx] = i;
              }
            }
          }
          ImGui::EndCombo();
        }
      };

      switch (edgeType) {
      case Context:
      {
        Rule::Edge& edge = m_CurrentEditedRule->m_ContextEdges[locIdx];
        selNodeEdge(edge, 0, (1 << Context) | (1 << Cut));
        selNodeEdge(edge, 1, (1 << Context) | (1 << Cut));
      }
      break;
      case New:
      {
        Rule::NewEdge& edge = m_CurrentEditedRule->m_NewEdge[locIdx];
        selNodeEdge(edge, 0, (1 << Context) | (1 << New));
        selNodeEdge(edge, 1, (1 << Context) | (1 << New));

        auto selEdgePort = [this, cutEdgesOffset](Rule::NewEdge & edge, int idx, uint32_t allowedTypes) {
          const char* portNames[] = { "Port 1", "Port 2" };
          const char* noPort = "<None>";
          if (ImGui::BeginCombo(portNames[idx], edge.port[idx] >= 0 ? ComputeEdgeName(edge.port[idx]).c_str() : noPort)) {
            if (ImGui::Selectable(noPort, edge.port[idx] < 0)) {
              edge.port[idx] = -1;
            }
            for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutEdge.size(); ++i) {
              const uint32_t edgeIdx = cutEdgesOffset + i;
              if (edgeIdx != edge.port[idx]) {
                if (ImGui::Selectable(ComputeCutEdgeName(i).c_str(), edge.port[idx] == edgeIdx)) {
                  edge.nodes[idx] = edgeIdx;
                }
              }
            }
            ImGui::EndCombo();
          }
        };
      }
      break;
      case Cut:
      {
        Rule::Edge& edge = m_CurrentEditedRule->m_CutEdge[locIdx];
        selNodeEdge(edge, 0, (1 << Context) | (1 << Cut));
        selNodeEdge(edge, 1, (1 << Context) | (1 << Cut));
      }
      break;
      }
    }
  }

  void GraphEdState::Draw() {
    if (ImGui::Begin("GraphEditor")) {
      
      ImGui::BeginTable("GraphEditorRoot", 2, ImGuiTableFlags_Resizable );
      ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_WidthFixed, 400);
      ImGui::TableSetupColumn("View");
      ImGui::TableNextColumn();
;      const char* tabs[] = { "Tags", "Rules", "Nodes", "Edges" };
      bool opened[] = { false, false, false, false };
      opened[m_CurrentTab] = true;

      void(GraphEdState ::*drawFun[])() = { &GraphEdState::DrawTagsPanel, &GraphEdState::DrawRulesPanel, &GraphEdState::DrawNodesPanel, &GraphEdState::DrawEdgesPanel};

      ImGui::BeginTabBar( "EditRoot" );
      for( uint32_t i = 0; i< 4; ++i)
      {
        if (i >= 2 && m_CurrentEditedRule == nullptr) 
        {
          break;
        }
        if (ImGui::BeginTabItem(tabs[i])) 
        {
          m_CurrentTab = i;
          (this->*drawFun[i])();
        
          ImGui::EndTabItem();
        } 
      }
      
      ImGui::EndTabBar();
     
      ImGui::TableNextColumn();
      

      ImGui::EndTable();

    }
    ImGui::End();
  }
}

#include <gen/pregraph.hpp>
#include <gen/graphutils.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/fruchterman_reingold.hpp>

namespace eXl
{
  void GraphEdState::DrawInfos::Clear() {
    edgeDesc.clear();
    edges.clear();
    edgesColor.clear();
    nodeDesc.clear();
    nodes.clear();
    nodesColor.clear();
  }

  void GraphEdState::UpdateDisplay(World& iWorld)
  {
    m_DrawInfos.Clear();
    if (m_CurrentEditedRule == nullptr)
    {
      return;
    }

    ES_RuleSystem::Graph srcGraph;
    Vector<ES_RuleSystem::GraphVtx> nodes;
    Vector<Name> nodeTags;
    TGraphMap < ES_RuleSystem::Graph, boost::rectangle_topology<>::point_type> positionMap;

    boost::rectangle_topology<>::point_type defaultPos;
    defaultPos[0] = 0;
    defaultPos[1] = 0;

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_ContextNodes[i];
      nodeTags.push_back(nodeTag);
      m_DrawInfos.nodesColor.push_back(Vec4(0, 0, 1, 1));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_CutNodes[i];
      nodeTags.push_back(nodeTag);
      m_DrawInfos.nodesColor.push_back(Vec4(1.0, 0, 0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }

    Vector<ES_RuleSystem::GraphEdge> edges;
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_ContextEdges[i];
      m_DrawInfos.edgesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutEdge.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_CutEdge[i];
      m_DrawInfos.edgesColor.push_back(Vec4(1.0, 0, 0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }

    uint32_t const totNumNodes = m_CurrentEditedRule->m_ContextNodes.size()
                               + m_CurrentEditedRule->m_CreateNodes.size()
                               + m_CurrentEditedRule->m_CutNodes.size();

    const float nodeSize = 10;

    float dist = Mathf::Max(totNumNodes + 2 / 3, 1) * nodeSize ;

    boost::rectangle_topology<> rectangle(-dist * 1.3, -dist, -dist * 0.3, dist);
    boost::random_graph_layout(srcGraph, MakeRef(positionMap), rectangle);
    boost::fruchterman_reingold_force_directed_layout(srcGraph, MakeRef(positionMap), rectangle);

    GameDatabase& database = *iWorld.GetSystem<GameDatabase>();

    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
      auto const& vtx = nodes[i];
      auto pos = boost::get(positionMap, vtx);
      m_DrawInfos.nodes.push_back(Vec2(pos[0], pos[1]));

      Name tag = nodeTags[i];
      auto iter = m_Sys->m_Tags.find(tag);
      if (iter != m_Sys->m_Tags.end())
      {
        
        
        //auto iterAdd = m_SysRsc->m_Tags.find(tag);
        //if (iterAdd != m_SysRsc->m_Tags.end() && iterAdd->second.m_Archetype.GetUUID().IsValid())
        //{
        //  Archetype const* arch = iterAdd->second.m_Archetype.GetOrLoad();
        //  if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
        //  {
        //    ObjectHandle obj = world.CreateObject();
        //    database.InstantiateArchetype(obj, arch, nullptr);
        //    trans.AddTransform(obj, glm::translate(Identity<Mat4>(), Vec3(pos[0], pos[1], 0.0)));
        //    gfx.CreateSpriteComponent(obj);
        //    m_DisplayNodes.push_back(obj);
        //  }
        //}
      }
    }
    
    for (auto const& edge : edges)
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vec2d& pos1V = reinterpret_cast<Vec2d&>(pos1);
      Vec2d& pos2V = reinterpret_cast<Vec2d&>(pos2);
      Vec2d dir = normalize(pos2V - pos1V);
      pos2V -= dir * double(nodeSize);
      pos1V += dir * double(nodeSize);
      m_DrawInfos.edges.push_back(Segmentf(Vec2(pos1[0], pos1[1]), Vec2(pos2[0], pos2[1])));
    }

    rectangle = boost::rectangle_topology<>(dist * 0.3, -dist, dist * 1.3, dist);
    srcGraph.clear();
    nodes.clear();
    nodeTags.clear();
    edges.clear();
    positionMap.m_Map.clear();

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_ContextNodes[i];
      nodeTags.push_back(nodeTag);
      m_DrawInfos.nodesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CreateNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_CreateNodes[i];
      nodeTags.push_back(nodeTag);
      m_DrawInfos.nodesColor.push_back(Vec4(0, 1.0, 0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_ContextEdges[i];
      m_DrawInfos.edgesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_NewEdge.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_NewEdge[i];
      m_DrawInfos.edgesColor.push_back(Vec4(0, 1.0, 0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }

    boost::random_graph_layout(srcGraph, MakeRef(positionMap), rectangle);
    boost::fruchterman_reingold_force_directed_layout(srcGraph, MakeRef(positionMap), rectangle);

    for (uint32_t i = 0; i < nodes.size(); ++i)
    {
      auto const& vtx = nodes[i];
      if (vtx == ES_RuleSystem::Graph::null_vertex())
      {
        continue;
      }
      auto pos = boost::get(positionMap, vtx);
      m_DrawInfos.nodes.push_back(Vec2(pos[0], pos[1]));

      Name tag = nodeTags[i];
      auto iter = m_Sys->m_Tags.find(tag);
      if (iter != m_Sys->m_Tags.end())
      {
        //auto iterAdd = m_SysRsc->m_Tags.find(tag);
        //if (iterAdd != m_SysRsc->m_Tags.end() && iterAdd->second.m_Archetype.GetUUID().IsValid())
        //{
        //  Archetype const* arch = iterAdd->second.m_Archetype.GetOrLoad();
        //  if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
        //  {
        //    ObjectHandle obj = world.CreateObject();
        //    database.InstantiateArchetype(obj, arch, nullptr);
        //    trans.AddTransform(obj, translate(Identity<Mat4>(), Vec3(pos[0], pos[1], 0.0)));
        //    gfx.CreateSpriteComponent(obj);
        //    m_DisplayNodes.push_back(obj);
        //  }
        //}
      }
    }

    for (auto const& edge : edges)
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vec2d& pos1V = reinterpret_cast<Vec2d&>(pos1);
      Vec2d& pos2V = reinterpret_cast<Vec2d&>(pos2);
      Vec2d dir = normalize(pos2V - pos1V);
      pos2V -= dir * double(nodeSize);
      pos1V += dir * double(nodeSize);
      m_DrawInfos.edges.push_back(Segmentf(Vec2(pos1[0], pos1[1]), Vec2(pos2[0], pos2[1])));
    }
  }
}
