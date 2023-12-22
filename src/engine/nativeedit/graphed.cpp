#include "graphed.hpp"
#include <core/resource/resourcemanager.hpp>
#include <engine/common/gamedatabase.hpp>

#include <imgui.h>

namespace eXl 
{ 
  void UpdateDisplay(GraphEdState& iState);

  void DrawRulesPanel(GraphEdState & iState)
  {
    if (ImGui::BeginPopupModal("RuleNameInput")) 
    {
      ImGui::InputText("Name", iState.m_NewRuleName, sizeof(iState.m_NewRuleName));
      if (ImGui::Button("Ok")) 
      {
        String newRuleName(iState.m_NewRuleName);
        iState.m_NewRuleName[0] = 0;
        auto iter = iState.m_Sys->m_Rules.find(newRuleName);
        if ( !newRuleName.empty() && iter == iState.m_Sys->m_Rules.end())
        {
          iState.m_Sys->m_Rules.insert(std::make_pair(newRuleName, Rule()));
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if( ImGui::Button("Cancel"))
      {
        ImGui::CloseCurrentPopup();
      }
    }

    if (ImGui::Button("+")) 
    {
      ImGui::OpenPopup("RuleNameInput");
    }
    ImGui::SameLine();

    if (ImGui::Button("-")) 
    {
      if (iState.m_CurrentEditedRule != nullptr)
      {
        iState.m_Sys->m_Rules.erase(iState.m_CurrentEditedRuleName);
        iState.m_CurrentEditedRule = nullptr;
        iState.m_CurrentEditedRuleName.clear();
      }
    }

    ImGui::BeginListBox("Rules");

    for ( auto& ruleEntry : iState.m_Sys->m_Rules) {
      bool selected = iState.m_CurrentEditedRuleName == ruleEntry.first;
      if (ImGui::Selectable(ruleEntry.first.c_str(), &selected)) {
        iState.m_CurrentEditedRuleName = ruleEntry.first;
        iState.m_CurrentEditedRule = &ruleEntry.second;
        iState.m_NodeNewStart = iState.m_CurrentEditedRule->m_ContextNodes.size();
        iState.m_NodeCutStart = iState.m_NodeNewStart + iState.m_CurrentEditedRule->m_CreateNodes.size();
        iState.m_EdgeNewStart = iState.m_CurrentEditedRule->m_ContextEdges.size();
        iState.m_EdgeCutStart = iState.m_EdgeNewStart + iState.m_CurrentEditedRule->m_NewEdge.size();
      }
    }
    
    ImGui::EndListBox();

    ImGui::LabelText("RuleScript", "Rule Script");
    ImGui::SameLine();
    
    auto scriptEntry = iState.m_RuleScripts.find( iState.m_CurrentEditedRuleName );
    const Resource::UUID* uuid = scriptEntry != iState.m_RuleScripts.end() ? &scriptEntry->second.GetUUID() : nullptr;

    const Resource::Header* selRsc = uuid != nullptr ? ResourceManager::GetHeader(*uuid) : nullptr;

    if (ImGui::BeginCombo("RuleScriptSelector", selRsc != nullptr ? selRsc->m_ResourceName.c_str() : "<none>"))
    {
      Vector<Resource::Header> resources = ResourceManager::ListResources(LuaEventHandler::StaticLoaderName());

      if (ImGui::Selectable("<none>", scriptEntry == iState.m_RuleScripts.end())) {
        iState.m_RuleScripts.erase(scriptEntry);
      }
      if (iState.m_CurrentEditedRule != nullptr) {
        for (const auto& rsc : resources) {
          bool selected = uuid != nullptr && *uuid == rsc.m_ResourceId;
          if (ImGui::Selectable(rsc.m_ResourceName.c_str(), &selected)) {
            ResourceHandle< LuaEventHandler > handle;
            handle.SetUUID(rsc.m_ResourceId);
            iState.m_RuleScripts.insert(std::make_pair(iState.m_CurrentEditedRuleName, handle));
          }
        }
      }

      ImGui::EndCombo();
    }
  }

  void DrawTagsPanel(GraphEdState& iState)
  {
    if (ImGui::Button("+"))
    {
      auto iter = iState.m_Sys->m_Tags.find("NewTag");
      if (iter == iState.m_Sys->m_Tags.end())
      {
        iState.m_Sys->m_Tags.insert(std::make_pair("NewRule", TagDef()));
      }
    }
    ImGui::SameLine();

    if (ImGui::Button("-"))
    {
      if (iState.m_CurrentEditedTag != nullptr)
      {
        iState.m_Sys->m_Tags.erase(iState.m_CurrentEditedTagName);
        iState.m_CurrentEditedTag = nullptr;
        iState.m_CurrentEditedTagName = Name();
      }
    }

    ImGui::BeginListBox("TagsList");

    for (auto& tagEntry : iState.m_Sys->m_Tags) {
      bool selected = iState.m_CurrentEditedTagName == tagEntry.first;
      if (ImGui::Selectable(tagEntry.first.c_str(), &selected)) {
        iState.m_CurrentEditedTagName = tagEntry.first;
        iState.m_CurrentEditedTag = &tagEntry.second;
      }
    }

    ImGui::EndListBox();

    auto archetypeEntry = iState.m_TagsArch.find(iState.m_CurrentEditedTagName);
    const Resource::UUID* uuid = archetypeEntry != iState.m_TagsArch.end() ? &archetypeEntry->second.GetUUID() : nullptr;

    const Resource::Header* selRsc = uuid != nullptr ? ResourceManager::GetHeader(*uuid) : nullptr;

    if (ImGui::BeginCombo("TagArchetypeSelector", selRsc != nullptr ? selRsc->m_ResourceName.c_str() : "<none>"))
    {
      Vector<Resource::Header> resources = ResourceManager::ListResources(Archetype::StaticLoaderName());
      if (ImGui::Selectable("<none>", archetypeEntry == iState.m_TagsArch.end())) {
        iState.m_TagsArch.erase(archetypeEntry);
      }
      if (iState.m_CurrentEditedTag != nullptr) {
        for (const auto& rsc : resources) {
          bool selected = uuid != nullptr && *uuid == rsc.m_ResourceId;
          if (ImGui::Selectable(rsc.m_ResourceName.c_str(), &selected)) {
            ResourceHandle< LuaEventHandler > handle;
            handle.SetUUID(rsc.m_ResourceId);
            iState.m_RuleScripts.insert(std::make_pair(iState.m_CurrentEditedRuleName, handle));
          }
        }
      }
    }

    

    ImGui::EndCombo();
  }

  void DrawNodesPanel(GraphEdState& iState)
  {
    QWidget* nodesCollection = new QWidget(m_Editor);
    QVBoxLayout* nodesCollectionLayout = new QVBoxLayout(nodesCollection);
    nodesCollection->setLayout(nodesCollectionLayout);
    QToolBar* nodesCollectionTool = new QToolBar(nodesCollection);
    m_NodesList = new QListWidget(nodesCollection);

    nodesCollectionLayout->addWidget(nodesCollectionTool);

    QComboBox* nodeTypeSel = new QComboBox(nodesCollection);
    nodeTypeSel->addItem("Context");
    nodeTypeSel->addItem("Cut");
    nodeTypeSel->addItem("New");
    nodesCollectionLayout->addWidget(nodeTypeSel);

    int nodeType = 

    QObject::connect(m_NodesList->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_NodeTagSelection->clear();
          m_NodeTagSelection->setEnabled(false);
        }
        else
        {
          auto fillSelectorAndSet = [&](Name nodeTag)
          {
            int nodeIdx = 0;
            for (auto const& tag : m_Sys->m_Tags)
            {
              if (tag.second.m_IsNodeTag)
              {
                if (tag.first == nodeTag)
                {
                  nodeIdx = m_NodeTagSelection->count();
                }
                m_NodeTagSelection->addItem(tag.first.c_str());
              }
            }
            m_NodeTagSelection->setCurrentIndex(nodeIdx);
            m_NodeTagSelection->setEnabled(true);
          };

          if (iSelected.indexes().size() == 1)
          {
            QSignalBlocker block(m_NodeTagSelection);
            int selIdx = iSelected.indexes()[0].row();
            if (selIdx >= m_NodeNewStart)
            {
              Name nodeTag = m_CurrentEditedRule->m_CreateNodes[selIdx - m_NodeNewStart];
              m_NodeTagSelection->clear();
              fillSelectorAndSet(nodeTag);
            }
            else if (selIdx >= m_NodeCutStart)
            {
              Name nodeTag = m_CurrentEditedRule->m_CutNodes[selIdx - m_NodeCutStart];
              m_NodeTagSelection->clear();
              m_NodeTagSelection->addItem(RewriteSystem::GetAnyTag().c_str());
              fillSelectorAndSet(nodeTag);
            }
            else
            {
              Name nodeTag = m_CurrentEditedRule->m_ContextNodes[selIdx];
              m_NodeTagSelection->clear();
              m_NodeTagSelection->addItem(RewriteSystem::GetAnyTag().c_str());
              fillSelectorAndSet(nodeTag);
            }
          }
        }
      });

    nodesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Node", [this, nodeTypeSel]
      {
        int nodeType = nodeTypeSel->currentIndex();
        Name nodeTag = RewriteSystem::GetAnyTag();
        if (nodeType == 2)
        {
          for (auto const& tag : m_Sys->m_Tags)
          {
            if (tag.second.m_IsNodeTag)
            {
              nodeTag = tag.first;
            }
          }

          if (nodeTag == RewriteSystem::GetAnyTag())
          {
            LOG_ERROR << "Cannot create a node without a tag";
            return;
          }
        }
        int insertionPoint;
        if (nodeType == 0)
        {
          insertionPoint = m_CurrentEditedRule->m_ContextNodes.size();
          m_CurrentEditedRule->m_ContextNodes.push_back(nodeTag);
        }
        else if (nodeType == 1)
        {
          insertionPoint = m_CurrentEditedRule->m_ContextNodes.size()
            + m_CurrentEditedRule->m_CutNodes.size();
          m_CurrentEditedRule->m_CutNodes.push_back(nodeTag);
        }
        else
        {
          insertionPoint = m_CurrentEditedRule->m_ContextNodes.size()
            + m_CurrentEditedRule->m_CutNodes.size()
            + m_CurrentEditedRule->m_CreateNodes.size();
          m_CurrentEditedRule->m_CreateNodes.push_back(nodeTag);
        }

        RemapEdgesNode(insertionPoint, true);
        RebuildNodeList();
      });

    nodesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Node", [this]
      {
        if (m_NodesList->selectedItems().count() == 1)
        {
          int selIdx = m_NodesList->row(m_NodesList->selectedItems()[0]);
          if (selIdx >= m_NodeNewStart)
          {
            m_CurrentEditedRule->m_CreateNodes.erase(m_CurrentEditedRule->m_CreateNodes.begin() + (selIdx - m_NodeNewStart));
          }
          else if (selIdx >= m_NodeCutStart)
          {
            m_CurrentEditedRule->m_CutNodes.erase(m_CurrentEditedRule->m_CutNodes.begin() + (selIdx - m_NodeCutStart));
          }
          else
          {
            m_CurrentEditedRule->m_ContextNodes.erase(m_CurrentEditedRule->m_ContextNodes.begin() + selIdx);
          }

          RemapEdgesNode(selIdx, false);
          RebuildNodeList();
          RebuildEdgeList();
        }
      });

    nodesCollectionLayout->addWidget(m_NodesList);
    m_NodeTagSelection = new QComboBox(nodesCollection);
    QObject::connect(m_NodeTagSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int iIndex)
      {
        if (iIndex == -1)
        {
          return;
        }
        if (m_NodesList->selectedItems().isEmpty())
        {
          return;
        }
        Name selTag(m_NodeTagSelection->itemText(iIndex).toUtf8());

        int selIdx = m_NodesList->row(m_NodesList->selectedItems()[0]);
        Name& curTag =
          (selIdx >= m_NodeNewStart) ? m_CurrentEditedRule->m_CreateNodes[selIdx - m_NodeNewStart] :
          ((selIdx >= m_NodeCutStart) ? m_CurrentEditedRule->m_CutNodes[selIdx - m_NodeCutStart] :
            m_CurrentEditedRule->m_ContextNodes[selIdx]);

        if (selTag == RewriteSystem::GetAnyTag())
        {
          if (curTag == selTag)
          {
            return;
          }
        }
        else
        {
          auto iter = m_Sys->m_Tags.find(selTag);

          if (iter != m_Sys->m_Tags.end()
            && iter->second.m_IsNodeTag
            && curTag == iter->first)
          {
            return;
          }
        }
        curTag = selTag;
        m_Editor->ModifyResource();
        RebuildNodeList();
        m_NodesList->setItemSelected(m_NodesList->item(selIdx), true);
      });
    m_NodeTagSelection->setEnabled(false);
    nodesCollectionLayout->addWidget(m_NodeTagSelection);

    return nodesCollection;
  }

  QWidget* GraphEditor::Impl::MakeEdgesPanel()
  {
    QWidget* edgesCollection = new QWidget(m_Editor);

    QVBoxLayout* edgesCollectionLayout = new QVBoxLayout(edgesCollection);
    edgesCollection->setLayout(edgesCollectionLayout);
    QToolBar* edgesCollectionTool = new QToolBar(edgesCollection);
    m_EdgesList = new QListWidget(edgesCollection);

    edgesCollectionLayout->addWidget(edgesCollectionTool);

    QComboBox* edgeTypeSel = new QComboBox(edgesCollection);
    edgeTypeSel->addItem("Context");
    edgeTypeSel->addItem("Cut");
    edgeTypeSel->addItem("New");
    edgesCollectionLayout->addWidget(edgeTypeSel);

    m_EdgeTagSelection = new QComboBox(edgesCollection);

    QObject::connect(m_EdgesList->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_EdgeTagSelection->clear();
          m_EdgeTagSelection->setEnabled(false);
          m_Node1Sel->setEnabled(false);
          m_Node2Sel->setEnabled(false);
          m_Port1Sel->setEnabled(false);
          m_Port2Sel->setEnabled(false);
        }
        else
        {
          auto fillSelectorAndSet = [&](Name edgeTag)
          {
            int edgeIdx = 0;
            for (auto const& tag : m_Sys->m_Tags)
            {
              if (!tag.second.m_IsNodeTag)
              {
                if (tag.first == edgeTag)
                {
                  edgeIdx = m_EdgeTagSelection->count();
                }
                m_EdgeTagSelection->addItem(tag.first.c_str());
              }
            }
            m_EdgeTagSelection->setCurrentIndex(edgeIdx);
            m_EdgeTagSelection->setEnabled(true);
          };

          if (iSelected.indexes().size() == 1)
          {
            m_Node1Sel->setEnabled(true);
            m_Node2Sel->setEnabled(true);
            int nodes[2];

            QSignalBlocker block(m_EdgeTagSelection);
            int selIdx = iSelected.indexes()[0].row();
            bool const isNewEdge = selIdx >= m_EdgeNewStart;
            bool const isCutEdge = !isNewEdge && selIdx >= m_EdgeCutStart;
            int edgeKind = isNewEdge ? 2 : (isCutEdge ? 1 : 0);
            if (isNewEdge)
            {
              auto const& edgeDesc = m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart];
              m_EdgeTagSelection->clear();
              fillSelectorAndSet(edgeDesc.tag);
              std::copy(edgeDesc.nodes, ArrayEnd(edgeDesc.nodes), nodes);

              m_Port1Sel->setEnabled(true);
              m_Port2Sel->setEnabled(true);
              RebuildPortSelList(m_Port1Sel, edgeDesc.nodes[0], edgeDesc.port[0]);
              RebuildPortSelList(m_Port2Sel, edgeDesc.nodes[1], edgeDesc.port[1]);
            }
            else if (isCutEdge)
            {
              auto const& edgeDesc = m_CurrentEditedRule->m_CutEdge[selIdx - m_EdgeCutStart];
              m_EdgeTagSelection->clear();
              m_EdgeTagSelection->addItem(RewriteSystem::GetAnyTag().c_str());
              fillSelectorAndSet(edgeDesc.tag);
              std::copy(edgeDesc.nodes, ArrayEnd(edgeDesc.nodes), nodes);

              m_Port1Sel->setEnabled(false);
              m_Port2Sel->setEnabled(false);
            }
            else
            {
              auto const& edgeDesc = m_CurrentEditedRule->m_ContextEdges[selIdx];
              m_EdgeTagSelection->clear();
              m_EdgeTagSelection->addItem(RewriteSystem::GetAnyTag().c_str());
              fillSelectorAndSet(edgeDesc.tag);
              std::copy(edgeDesc.nodes, ArrayEnd(edgeDesc.nodes), nodes);

              m_Port1Sel->setEnabled(false);
              m_Port2Sel->setEnabled(false);
            }
            RebuildNodeSelList(m_Node1Sel, nodes[0], nodes[1], edgeKind);
            RebuildNodeSelList(m_Node2Sel, nodes[1], nodes[0], edgeKind);
          }
        }
      });

    edgesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Tag", [this, edgeTypeSel]
      {
        if (m_NodesList->count() < 2)
        {
          LOG_ERROR << "Cannot create an edge without nodes";
          return;
        }
        int edgeType = edgeTypeSel->currentIndex();
        Name edgeTag = RewriteSystem::GetAnyTag();
        if (edgeType == 2)
        {
          for (auto const& tag : m_Sys->m_Tags)
          {
            if (!tag.second.m_IsNodeTag)
            {
              edgeTag = tag.first;
            }
          }

          if (edgeTag == RewriteSystem::GetAnyTag())
          {
            LOG_ERROR << "Cannot create a edge without a tag";
            return;
          }
        }
        if (edgeType == 0)
        {
          Rule::Edge ctxEdge;
          ctxEdge.tag = edgeTag;
          ctxEdge.nodes[0] = 0;
          ctxEdge.nodes[1] = 1;
          m_CurrentEditedRule->m_ContextEdges.push_back(ctxEdge);
        }
        if (edgeType == 1)
        {
          Rule::Edge cutEdge;
          cutEdge.tag = edgeTag;
          cutEdge.nodes[0] = 0;
          cutEdge.nodes[1] = 1;
          m_CurrentEditedRule->m_CutEdge.push_back(cutEdge);
        }
        if (edgeType == 2)
        {
          Rule::NewEdge newEdge;
          newEdge.tag = edgeTag;
          newEdge.nodes[0] = 0;
          newEdge.nodes[1] = 1;
          newEdge.port[0] = -1;
          newEdge.port[1] = -1;

          m_CurrentEditedRule->m_NewEdge.push_back(newEdge);
        }
        RebuildEdgeList();
      });

    edgesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Tag", [this]
      {
        if (m_EdgesList->selectedItems().count() == 1)
        {
          int selIdx = m_EdgesList->row(m_EdgesList->selectedItems()[0]);
          if (selIdx >= m_EdgeNewStart)
          {
            m_CurrentEditedRule->m_NewEdge.erase(m_CurrentEditedRule->m_NewEdge.begin() + (selIdx - m_EdgeNewStart));
          }
          else if (selIdx >= m_EdgeCutStart)
          {
            m_CurrentEditedRule->m_CutEdge.erase(m_CurrentEditedRule->m_CutEdge.begin() + (selIdx - m_EdgeCutStart));
          }
          else
          {
            m_CurrentEditedRule->m_ContextEdges.erase(m_CurrentEditedRule->m_ContextEdges.begin() + selIdx);
          }

          RebuildEdgeList();
        }
      });

    edgesCollectionLayout->addWidget(m_EdgesList);
    QObject::connect(m_EdgeTagSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int iIndex)
      {
        if (iIndex == -1)
        {
          return;
        }
        if (m_EdgesList->selectedItems().isEmpty())
        {
          return;
        }
        Name selTag(m_EdgeTagSelection->itemText(iIndex).toUtf8());

        int selIdx = m_EdgesList->row(m_EdgesList->selectedItems()[0]);
        Name& curTag =
          (selIdx >= m_EdgeNewStart) ? m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart].tag :
          ((selIdx >= m_EdgeCutStart) ? m_CurrentEditedRule->m_CutEdge[selIdx - m_EdgeCutStart].tag :
            m_CurrentEditedRule->m_ContextEdges[selIdx].tag);

        if (selTag == RewriteSystem::GetAnyTag())
        {
          if (curTag == selTag)
          {
            return;
          }
        }
        else
        {
          auto iter = m_Sys->m_Tags.find(selTag);

          if (iter != m_Sys->m_Tags.end()
            && !iter->second.m_IsNodeTag
            && curTag == iter->first)
          {
            return;
          }
        }

        curTag = selTag;
        m_Editor->ModifyResource();
        RebuildEdgeList();
        m_EdgesList->setItemSelected(m_EdgesList->item(selIdx), true);
      });
    m_EdgeTagSelection->setEnabled(false);
    edgesCollectionLayout->addWidget(m_EdgeTagSelection);

    m_Node1Sel = new QComboBox(edgesCollection);
    m_Node2Sel = new QComboBox(edgesCollection);

    m_Port1Sel = new QComboBox(edgesCollection);
    m_Port2Sel = new QComboBox(edgesCollection);

    edgesCollectionLayout->addWidget(m_Node1Sel);
    edgesCollectionLayout->addWidget(m_Node2Sel);
    edgesCollectionLayout->addWidget(m_Port1Sel);
    edgesCollectionLayout->addWidget(m_Port2Sel);

    m_Node1Sel->setEnabled(false);
    m_Node2Sel->setEnabled(false);
    m_Port1Sel->setEnabled(false);
    m_Port2Sel->setEnabled(false);

    auto onNodeChanged = [this](uint32_t iNodeIndex, int iNodeNum, QComboBox* iOtherBox, QComboBox* iPortSel)
    {
      if (m_EdgesList->selectedItems().count() == 1)
      {
        int selIdx = m_EdgesList->row(m_EdgesList->selectedItems()[0]);
        uint32_t* nodes = (selIdx >= m_EdgeNewStart) ? m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart].nodes
          : (selIdx >= m_EdgeCutStart ? m_CurrentEditedRule->m_CutEdge[selIdx - m_EdgeCutStart].nodes
            : m_CurrentEditedRule->m_ContextEdges[selIdx].nodes);

        bool const isNewEdge = selIdx >= m_EdgeNewStart;
        bool const isCutEdge = !isNewEdge && selIdx >= m_EdgeCutStart;
        int edgeKind = isNewEdge ? 2 : (isCutEdge ? 1 : 0);

        nodes[iNodeNum] = iNodeIndex;
        if (isNewEdge)
        {
          RebuildPortSelList(iPortSel, iNodeIndex, m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart].port[iNodeNum]);
        }
        RebuildNodeSelList(iOtherBox, nodes[1 - iNodeNum], nodes[iNodeNum], edgeKind);
        UpdateDisplay();
        m_Editor->ModifyResource();
      }
    };

    QObject::connect(m_Node1Sel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, onNodeChanged](int iIndex)
      {
        onNodeChanged(m_Node1Sel->itemData(iIndex).toInt(), 0, m_Node2Sel, m_Port1Sel);
      });

    QObject::connect(m_Node2Sel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, onNodeChanged](int iIndex)
      {
        onNodeChanged(m_Node2Sel->itemData(iIndex).toInt(), 1, m_Node1Sel, m_Port2Sel);
      });

    auto onPortChanged = [this](uint32_t iEdgeIndex, int iNodeNum)
    {
      if (m_EdgesList->selectedItems().count() == 1)
      {
        int selIdx = m_EdgesList->row(m_EdgesList->selectedItems()[0]);
        eXl_ASSERT(selIdx >= m_EdgeNewStart);

        Rule::NewEdge& newEdge = m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart];
        newEdge.port[iNodeNum] = iEdgeIndex;
        m_Editor->ModifyResource();
      }
    };

    QObject::connect(m_Port1Sel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, onPortChanged](int iIndex)
      {
        onPortChanged(m_Port1Sel->itemData(iIndex).toInt(), 0);
      });

    QObject::connect(m_Port2Sel, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, onPortChanged](int iIndex)
      {
        onPortChanged(m_Port2Sel->itemData(iIndex).toInt(), 1);
      });

    return edgesCollection;
  }


  void Draw(GraphEdState const& iState) {
    if (ImGui::Begin("GraphEditor")) {
      ImGui::BeginMenuBar();

      ImGui::EndMenuBar();

      ImGui::BeginTabBar( "EditRoot" );

      ImGui::BeginTabItem("Rules");
      ImGui::BeginListBox("RuleList");
      ImGui::EndListBox();
      ImGui::EndTabBar();
      
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

  void UpdateDisplay(World& iWorld, GraphEdState & iState)
  {
    iState.m_DrawInfos.Clear();
    if (iState.m_CurrentEditedRule == nullptr)
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

    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      Name nodeTag = iState.m_CurrentEditedRule->m_ContextNodes[i];
      nodeTags.push_back(nodeTag);
      iState.m_DrawInfos.nodesColor.push_back(Vec4(0, 0, 1, 1));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      iState.m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }
    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_CutNodes.size(); ++i)
    {
      Name nodeTag = iState.m_CurrentEditedRule->m_CutNodes[i];
      nodeTags.push_back(nodeTag);
      iState.m_DrawInfos.nodesColor.push_back(Vec4(1.0, 0, 0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      iState.m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i + iState.m_NodeCutStart) + " : " + String(nodeTag.get()));
    }

    Vector<ES_RuleSystem::GraphEdge> edges;
    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = iState.m_CurrentEditedRule->m_ContextEdges[i];
      iState.m_DrawInfos.edgesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      iState.m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }
    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_CutEdge.size(); ++i)
    {
      auto const& edge = iState.m_CurrentEditedRule->m_CutEdge[i];
      iState.m_DrawInfos.edgesColor.push_back(Vec4(1.0, 0, 0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      iState.m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i + iState.m_EdgeCutStart) + " : " + String(edge.tag.get()));
    }

    uint32_t const totNumNodes = iState.m_CurrentEditedRule->m_ContextNodes.size() 
                               + iState.m_CurrentEditedRule->m_CreateNodes.size()
                               + iState.m_CurrentEditedRule->m_CutNodes.size();

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
      iState.m_DrawInfos.nodes.push_back(Vec2(pos[0], pos[1]));

      Name tag = nodeTags[i];
      auto iter = iState.m_Sys->m_Tags.find(tag);
      if (iter != iState.m_Sys->m_Tags.end())
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
      iState.m_DrawInfos.edges.push_back(Segmentf(Vec2(pos1[0], pos1[1]), Vec2(pos2[0], pos2[1])));
    }

    rectangle = boost::rectangle_topology<>(dist * 0.3, -dist, dist * 1.3, dist);
    srcGraph.clear();
    nodes.clear();
    nodeTags.clear();
    edges.clear();
    positionMap.m_Map.clear();

    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      Name nodeTag = iState.m_CurrentEditedRule->m_ContextNodes[i];
      nodeTags.push_back(nodeTag);
      iState.m_DrawInfos.nodesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      iState.m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i) + " : " + String(nodeTag.get()));
    }
    nodes.resize(iState.m_NodeNewStart, ES_RuleSystem::Graph::null_vertex());
    nodeTags.resize(iState.m_NodeNewStart, Name());
    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_CreateNodes.size(); ++i)
    {
      Name nodeTag = iState.m_CurrentEditedRule->m_CreateNodes[i];
      nodeTags.push_back(nodeTag);
      iState.m_DrawInfos.nodesColor.push_back(Vec4(0, 1.0, 0, 1.0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      iState.m_DrawInfos.nodeDesc.push_back(StringUtil::FromInt(i + iState.m_NodeNewStart) + " : " + String(nodeTag.get()));
    }

    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = iState.m_CurrentEditedRule->m_ContextEdges[i];
      iState.m_DrawInfos.edgesColor.push_back(Vec4(0, 0, 1.0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      iState.m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i) + " : " + String(edge.tag.get()));
    }
    for (uint32_t i = 0; i < iState.m_CurrentEditedRule->m_NewEdge.size(); ++i)
    {
      auto const& edge = iState.m_CurrentEditedRule->m_NewEdge[i];
      iState.m_DrawInfos.edgesColor.push_back(Vec4(0, 1.0, 0, 1.0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      iState.m_DrawInfos.edgeDesc.push_back(StringUtil::FromInt(i + iState.m_EdgeNewStart) + " : " + String(edge.tag.get()));
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
      iState.m_DrawInfos.nodes.push_back(Vec2(pos[0], pos[1]));

      Name tag = nodeTags[i];
      auto iter = iState.m_Sys->m_Tags.find(tag);
      if (iter != iState.m_Sys->m_Tags.end())
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
      iState.m_DrawInfos.edges.push_back(Segmentf(Vec2(pos1[0], pos1[1]), Vec2(pos2[0], pos2[1])));
    }
  }
}
