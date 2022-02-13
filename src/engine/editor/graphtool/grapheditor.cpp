#include "grapheditor.hpp"
#include "graphdata.hpp"
#include "graphpainter.hpp"
#include "graphsimulate.hpp"

#include <editor/objectmodel.hpp>
#include <editor/objectdelegate.hpp>
#include <editor/gamewidget.hpp>
#include <editor/collectionmodel.hpp>
#include <editor/resourceselectionwidget.hpp>

#include <engine/game/archetype.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/common/app.hpp>
#include <core/input.hpp>

#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QListWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QLabel>
#include <QFileDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QComboBox>


namespace eXl
{
  using RulesCollectionModel = MapCollectionModel<String, Rule, RewriteSystem>;
  using TagsCollectionModel = MapCollectionModel<Name, TagDef, RewriteSystem>;

  ResourceEditorHandler& GraphEditor::GetEditorHandler()
  {
    static EditorHandler_T<RewriteSystem, GraphEditor> s_Handler;
    return s_Handler;
  }

	struct GraphEditor::Impl
	{
    Impl(GraphEditor*);

    WorldState m_World;
    InputSystem m_Inputs;
		
    QModelIndex m_GroupSelection;

    RulesCollectionModel* m_RulesCollectionModel;
    QListView* m_RulesCollectionView;

    String m_CurrentEditedRuleName;
    Rule* m_CurrentEditedRule = nullptr;

    int m_RuleTabIdx;

    TagsCollectionModel* m_TagsCollectionModel;
    QListView* m_TagsCollectionView;
    ResourceSelectionWidget* m_TagArchetypeSelection;
    QComboBox* m_TagTypeSelection;
    
    Name m_CurrentEditedTagName;
    TagDef* m_CurrentEditedTag = nullptr;

    int m_TagTabIdx;

    int m_NodesIdx;
    int m_EdgesIdx;

    QListWidget* m_NodesList;
    int m_NodeCutStart;
    int m_NodeNewStart;
    QComboBox* m_NodeTagSelection;

    QListWidget* m_EdgesList;
    int m_EdgeCutStart;
    int m_EdgeNewStart;
    QComboBox* m_EdgeTagSelection;

    QComboBox* m_Node1Sel;
    QComboBox* m_Node2Sel;
    QComboBox* m_Port1Sel;
    QComboBox* m_Port2Sel;

    QTabWidget* m_ObjPanel;
    int m_CurTab;

    GraphSimulateWidget* m_Simulate;
    int m_RuleDispTab;
    int m_SimulateTab;
    int m_CurDispTab;

    ResourceSelectionWidget* m_RuleScriptSelection;

    GraphEditor* m_Editor;
    RewriteSystem* m_Sys;

    GraphPainter* m_GraphPainter;
    Vector<ObjectHandle> m_DisplayNodes;

    QWidget* MakeRulesPanel();
    QWidget* MakeTagsPanel();
    QWidget* MakeNodesPanel();
    QWidget* MakeEdgesPanel();

    void UpdateCurRule();
    void UpdateCurTag();
    void RebuildNodeList();
    void RebuildEdgeList();
    void RemapEdgesNode(uint32_t iNodeIdx, bool iAdded);
    void RebuildNodeSelList(QComboBox* iBox, uint32_t iCurNode, uint32_t iOtherNode, int iEdgeKind);
    void RebuildPortSelList(QComboBox* iBox, uint32_t iCurNode, uint32_t iCurPort);
    void UpdateDisplay();
	};

  void GraphEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  GraphEditor::GraphEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl(this))
  {
  }

  QWidget* GraphEditor::Impl::MakeRulesPanel()
  {
    QWidget* rulesCollection = new QWidget(m_Editor);
    QVBoxLayout* rulesCollectionLayout = new QVBoxLayout(rulesCollection);
    rulesCollection->setLayout(rulesCollectionLayout);
    QToolBar* rulesCollectionTool = new QToolBar(rulesCollection);
    m_RulesCollectionView = new QListView(rulesCollection);
    m_RulesCollectionView->setModel(m_RulesCollectionModel);
    m_RulesCollectionView->setSelectionModel(new QItemSelectionModel(m_RulesCollectionView->model()));

    QObject::connect(m_RulesCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_CurrentEditedRuleName = String();
          m_CurrentEditedRule = nullptr;
          
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            String const* ruleName = m_RulesCollectionModel->GetNameFromIndex(iSelected.indexes()[0]);
            if (ruleName)
            {
              m_CurrentEditedRuleName = *ruleName;
              m_CurrentEditedRule = &m_Sys->m_Rules[m_CurrentEditedRuleName];
            }
            else
            {
              return;
            }
          }
        }

        if (m_Simulate->isEnabled())
        {
          m_Simulate->SetSelectedRule(m_CurrentEditedRuleName);
        }
        else
        {
          UpdateCurRule();
        }
      });

    QObject::connect(m_RulesCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
      {
        m_RulesCollectionView->clearSelection();
        m_RulesCollectionView->selectionModel()->select(iIndex, QItemSelectionModel::Select);
        m_Editor->ModifyResource();
      });

    QObject::connect(m_RulesCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
      {
        m_RulesCollectionView->clearSelection();
        m_Editor->ModifyResource();
      });
    QObject::connect(m_RulesCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
      {
        m_RulesCollectionView->clearSelection();
        m_Editor->ModifyResource();
      });

    rulesCollectionLayout->addWidget(rulesCollectionTool);
    rulesCollectionLayout->addWidget(m_RulesCollectionView);

    rulesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Rule", [this]
      {
        uint32_t curRowCount = m_RulesCollectionModel->rowCount(QModelIndex());
        if (!m_RulesCollectionModel->GetIndexFromName("NewRule").isValid())
        {
          if (m_RulesCollectionModel->insertRow(curRowCount))
          {
            QModelIndex newIndex = m_RulesCollectionModel->index(curRowCount, 0, QModelIndex());
            m_RulesCollectionModel->setData(newIndex, QString("NewRule"), Qt::EditRole);
          }
        }
      });

    rulesCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Rule", [this]
      {
        QModelIndex currentSelection = m_RulesCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_RulesCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
        }
      });

    m_RuleScriptSelection = new ResourceSelectionWidget(rulesCollection, LuaScriptBehaviour::StaticLoaderName(), ResourceSelectionWidget::Combo);
    rulesCollectionLayout->addWidget(m_RuleScriptSelection);
    QObject::connect(m_RuleScriptSelection, &ResourceSelectionWidget::onResourceChanged, [this]()
      {
        if (m_CurrentEditedRule)
        {
          m_CurrentEditedRule->m_RewriteScript.SetUUID(m_RuleScriptSelection->GetSelectedResourceId());
          m_Editor->ModifyResource();
        }
      });
    m_RuleScriptSelection->setEnabled(false);

    return rulesCollection;
  }

  QWidget* GraphEditor::Impl::MakeTagsPanel()
  {
    QSplitter* tagsEditionRoot = new QSplitter(Qt::Vertical, m_Editor);
    QWidget* tagsCollection = new QWidget(m_Editor);
    QVBoxLayout* tagsCollectionLayout = new QVBoxLayout(tagsCollection);
    tagsCollection->setLayout(tagsCollectionLayout);
    QToolBar* tagsCollectionTool = new QToolBar(tagsCollection);
    m_TagsCollectionView = new QListView(tagsCollection);
    m_TagsCollectionView->setModel(m_TagsCollectionModel);
    m_TagsCollectionView->setSelectionModel(new QItemSelectionModel(m_TagsCollectionView->model()));

    QObject::connect(m_TagsCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_CurrentEditedTagName = Name();
          m_CurrentEditedTag = nullptr;
          UpdateCurTag();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            Name const* tagName = m_TagsCollectionModel->GetNameFromIndex(iSelected.indexes()[0]);
            if (tagName)
            {
              m_CurrentEditedTagName = *tagName;
              m_CurrentEditedTag = &m_Sys->m_Tags[m_CurrentEditedTagName];
              UpdateCurTag();
            }
          }
        }
      });

    QObject::connect(m_TagsCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
      {
        m_TagsCollectionView->clearSelection();
        m_TagsCollectionView->selectionModel()->select(iIndex, QItemSelectionModel::Select);
        m_Editor->ModifyResource();
      });

    QObject::connect(m_TagsCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
      {
        m_TagsCollectionView->clearSelection();
        m_Editor->ModifyResource();
      });
    QObject::connect(m_TagsCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
      {
        m_TagsCollectionView->clearSelection();
        m_Editor->ModifyResource();
      });

    tagsCollectionLayout->addWidget(tagsCollectionTool);
    tagsCollectionLayout->addWidget(m_TagsCollectionView);

    tagsCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_FileIcon), "Add New Tag", [this]
      {
        uint32_t curRowCount = m_TagsCollectionModel->rowCount(QModelIndex());
        if (!m_TagsCollectionModel->GetIndexFromName("NewTag").isValid())
        {
          if (m_TagsCollectionModel->insertRow(curRowCount))
          {
            QModelIndex newIndex = m_TagsCollectionModel->index(curRowCount, 0, QModelIndex());
            m_TagsCollectionModel->setData(newIndex, QString("NewTag"), Qt::EditRole);
          }
        }
      });

    tagsCollectionTool->addAction(m_Editor->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Tag", [this]
      {
        QModelIndex currentSelection = m_TagsCollectionView->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_TagsCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
        }
      });

    QWidget* tagEditionWidget = new QWidget(m_Editor);
    QVBoxLayout* tagsEditionLayout = new QVBoxLayout(tagEditionWidget);
    tagEditionWidget->setLayout(tagsEditionLayout);
    m_TagArchetypeSelection = new ResourceSelectionWidget(tagEditionWidget, Archetype::StaticLoaderName(), ResourceSelectionWidget::Combo);

    QObject::connect(m_TagArchetypeSelection, &ResourceSelectionWidget::onResourceChanged, [this]()
      {
        if (m_CurrentEditedTag)
        {
          m_CurrentEditedTag->m_Archetype.SetUUID(m_TagArchetypeSelection->GetSelectedResourceId());
          m_Editor->ModifyResource();
        }
      });

    m_TagTypeSelection = new QComboBox(tagEditionWidget);
    m_TagTypeSelection->addItem("Node");
    m_TagTypeSelection->addItem("Edge");

    QObject::connect(m_TagTypeSelection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int iIndex)
      {
        if (m_CurrentEditedTag)
        {
          m_CurrentEditedTag->m_IsNodeTag = m_TagTypeSelection->currentIndex() == 0;
          m_Editor->ModifyResource();
        }
      });

    tagsEditionLayout->addWidget(m_TagArchetypeSelection);
    tagsEditionLayout->addWidget(m_TagTypeSelection);

    // Could add archetype customization

    tagsEditionRoot->addWidget(tagsCollection);
    tagsEditionRoot->addWidget(tagEditionWidget);

    return tagsEditionRoot;
  }

  QWidget* GraphEditor::Impl::MakeNodesPanel()
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

          if(nodeTag == RewriteSystem::GetAnyTag())
          {
            LOG_ERROR << "Cannot create a node without a tag";
            return;
          }
        }
        int insertionPoint;
        if(nodeType == 0)
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
          else if(selIdx >= m_NodeCutStart)
          { 
            m_CurrentEditedRule->m_CutNodes.erase(m_CurrentEditedRule->m_CutNodes.begin() + (selIdx - m_NodeCutStart));
          }
          else
          {
            m_CurrentEditedRule->m_ContextNodes.erase(m_CurrentEditedRule->m_ContextNodes.begin() + selIdx );
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
          if(curTag == selTag)
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
        uint32_t* nodes =  (selIdx >= m_EdgeNewStart) ? m_CurrentEditedRule->m_NewEdge[selIdx - m_EdgeNewStart].nodes
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


  GraphEditor::Impl::Impl(GraphEditor* iEditor)
    : m_Editor(iEditor)
  {

    m_World.Init(EditorState::GetProjectProperties()).WithGfx();

    World& world = m_World.GetWorld();
    GfxSystem& gfx = *world.GetSystem<GfxSystem>();

    m_Sys = RewriteSystem::DynamicCast(iEditor->GetDocument()->GetResource());

    m_RulesCollectionModel = MakeMapCollectionModel(m_Editor, m_Sys, &RewriteSystem::m_Rules);
    m_TagsCollectionModel = MakeMapCollectionModel(m_Editor, m_Sys, &RewriteSystem::m_Tags);

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, m_Editor);
    
    m_ObjPanel = new QTabWidget(m_Editor);
    
    m_RuleTabIdx = m_ObjPanel->addTab(MakeRulesPanel(), "Rules");
    m_TagTabIdx = m_ObjPanel->addTab(MakeTagsPanel(), "Tags");

    m_NodesIdx = m_ObjPanel->addTab(MakeNodesPanel(), "Nodes");
    m_EdgesIdx = m_ObjPanel->addTab(MakeEdgesPanel(), "Edges");

    m_ObjPanel->setTabEnabled(m_NodesIdx, false);
    m_ObjPanel->setTabEnabled(m_EdgesIdx, false);

    QObject::connect(m_ObjPanel, &QTabWidget::currentChanged, [this](int iCurTab)
      {
        if (m_CurTab == m_RuleTabIdx)
        {
          
        }
        if (m_CurTab == m_TagTabIdx)
        {
          m_CurrentEditedTag = nullptr;
          UpdateCurTag();
        }
        m_CurTab = iCurTab;
      });

    rootSplitter->addWidget(m_ObjPanel);

    QTabWidget* displayArea = new QTabWidget(m_Editor);

    {
      GameWidget* gameWidget = new GameWidget(m_Editor);
      gameWidget->SetInputSystem(&m_Inputs);
      gameWidget->SetGfxSystem(m_World.GetWorld().GetSystem<GfxSystem>());

      m_GraphPainter = new GraphPainter(gameWidget);
      gameWidget->SetPainterInterface(m_GraphPainter);

      GfxSystem::ViewInfo& view = gameWidget->GetViewInfo();
      view.pos = Vector3f::UNIT_Z * 2;
      view.projection = GfxSystem::Orthographic;
      view.displayedSize = GraphPainter::s_NodeSize * 10;
      view.backgroundColor = Vector4f::ONE;

      m_World.GetCamera().view = view;

      gameWidget->SetTickCallback([this, gameWidget](float iDelta)
      {
        World& world = m_World.GetWorld();
        GfxSystem& gfx = *world.GetSystem<GfxSystem>();

        m_World.GetCamera().ProcessInputs(m_World.GetWorld(), m_Inputs, CameraState::WheelZoom | CameraState::RightClickPan);
        gameWidget->GetViewInfo().pos = m_World.GetCamera().view.pos;
        gameWidget->GetViewInfo().basis[0] = m_World.GetCamera().view.basis[0];
        gameWidget->GetViewInfo().basis[1] = m_World.GetCamera().view.basis[1];
        gameWidget->GetViewInfo().basis[2] = m_World.GetCamera().view.basis[2];
        gameWidget->GetViewInfo().displayedSize = m_World.GetCamera().view.displayedSize;
        gameWidget->ViewInfoUpdated();

        m_Inputs.Clear();

        m_World.Tick();
      });

      gameWidget->SetAnimated(true);

      m_RuleDispTab = displayArea->addTab(gameWidget, "CurrentRule");
    }
    
    m_Simulate = new GraphSimulateWidget(m_Editor, *m_Sys);
    m_Simulate->setEnabled(false);

    m_SimulateTab = displayArea->addTab(m_Simulate, "Simulate");
    QObject::connect(displayArea, &QTabWidget::currentChanged, [this](int iCurTab)
      {
        if (m_CurDispTab == m_RuleDispTab)
        {
          m_ObjPanel->setTabEnabled(m_TagTabIdx, false);
          m_RulesCollectionView->clearSelection();
          UpdateCurRule();
        }
        if (m_CurDispTab == m_SimulateTab)
        {
          m_Simulate->setEnabled(false);
        }
        m_CurDispTab = iCurTab;
        if (m_CurDispTab == m_RuleDispTab)
        {
          m_ObjPanel->setTabEnabled(m_TagTabIdx, true);
          m_RulesCollectionView->clearSelection();
        }
        if (m_CurDispTab == m_SimulateTab)
        {
          m_ObjPanel->setCurrentIndex(m_RuleTabIdx);
          m_Simulate->setEnabled(true);
        }
      });
    
    m_CurDispTab = m_RuleDispTab;
    rootSplitter->addWidget(displayArea);
    rootSplitter->setSizes({1000, 6000});

		QVBoxLayout* layout = new QVBoxLayout(m_Editor);
		layout->addWidget(rootSplitter);
		m_Editor->setLayout(layout);
	}

  void GraphEditor::Impl::RebuildNodeList()
  {
    m_NodesList->clear();
    
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      auto const& nodeDesc = m_CurrentEditedRule->m_ContextNodes[i];
      String nodeStr = String("Ctx Node ") + StringUtil::FromInt(m_NodesList->count()) + " (" + String(nodeDesc.get()) + ")" ;
      m_NodesList->addItem(nodeStr.c_str());
    }
    m_NodeCutStart = m_NodesList->count();
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutNodes.size(); ++i)
    {
      auto const& nodeDesc = m_CurrentEditedRule->m_CutNodes[i];
      String nodeStr = String("Cut Node ") + StringUtil::FromInt(m_NodesList->count()) + " (" + String(nodeDesc.get()) + ")";
      m_NodesList->addItem(nodeStr.c_str());
    }
    m_NodeNewStart = m_NodesList->count();
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CreateNodes.size(); ++i)
    {
      auto const& nodeDesc = m_CurrentEditedRule->m_CreateNodes[i];
      String nodeStr = String("New Node ") + StringUtil::FromInt(m_NodesList->count()) + " (" + String(nodeDesc.get()) + ")";
      m_NodesList->addItem(nodeStr.c_str());
    }
    UpdateDisplay();
  }

  void GraphEditor::Impl::RebuildEdgeList()
  {
    m_EdgesList->clear();

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edgeDesc = m_CurrentEditedRule->m_ContextEdges[i];
      String edgeStr = String("Ctx Edge ") + StringUtil::FromInt(m_EdgesList->count()) + " (" + String(edgeDesc.tag.get()) + ")";
      m_EdgesList->addItem(edgeStr.c_str());
    }
    m_EdgeCutStart = m_EdgesList->count();
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutEdge.size(); ++i)
    {
      auto const& edgeDesc = m_CurrentEditedRule->m_CutEdge[i];
      String edgeStr = String("Cut Edge ") + StringUtil::FromInt(m_EdgesList->count()) + " (" + String(edgeDesc.tag.get()) + ")";
      m_EdgesList->addItem(edgeStr.c_str());
    }
    m_EdgeNewStart = m_EdgesList->count();
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_NewEdge.size(); ++i)
    {
      auto const& edgeDesc = m_CurrentEditedRule->m_NewEdge[i];
      String edgeStr = String("New Edge ") + StringUtil::FromInt(m_EdgesList->count()) + " (" + String(edgeDesc.tag.get()) + ")";
      m_EdgesList->addItem(edgeStr.c_str());
    }
    UpdateDisplay();
  }

  void GraphEditor::Impl::UpdateCurRule()
  {
    if (m_CurrentEditedRule == nullptr)
    {
      m_RuleScriptSelection->setEnabled(false);
      m_ObjPanel->setTabEnabled(m_NodesIdx, false);
      m_ObjPanel->setTabEnabled(m_EdgesIdx, false);
      UpdateDisplay();
    }
    else
    {
      m_RuleScriptSelection->setEnabled(true);
      QSignalBlocker blocker(m_RuleScriptSelection);
      m_RuleScriptSelection->ForceSelection(m_CurrentEditedRule->m_RewriteScript.GetUUID());
      m_ObjPanel->setTabEnabled(m_NodesIdx, true);
      m_ObjPanel->setTabEnabled(m_EdgesIdx, true);

      RebuildNodeList();
      RebuildEdgeList();
    }
  }

  void GraphEditor::Impl::UpdateCurTag()
  {
    if (m_CurrentEditedTag == nullptr)
    {
      m_TagArchetypeSelection->Clear();
      m_TagTypeSelection->setEnabled(false);
      m_TagArchetypeSelection->setEnabled(false);
    }
    else
    {
      m_TagTypeSelection->setEnabled(true);
      m_TagArchetypeSelection->setEnabled(true);
      m_TagArchetypeSelection->ForceSelection(m_CurrentEditedTag->m_Archetype.GetUUID());
      m_TagTypeSelection->setCurrentIndex(m_CurrentEditedTag->m_IsNodeTag ? 0 : 1);
    }
  }

  void GraphEditor::Impl::RemapEdgesNode(uint32_t iNodeIdx, bool iAdded)
  {
    auto cleanupEdges = [&](auto& iCollection)
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

    auto shiftEdges = [&](auto& iCollection)
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

    if (!iAdded)
    {
      cleanupEdges(m_CurrentEditedRule->m_ContextEdges);
      cleanupEdges(m_CurrentEditedRule->m_CutEdge);
      cleanupEdges(m_CurrentEditedRule->m_NewEdge);
    }
    else
    {
      shiftEdges(m_CurrentEditedRule->m_ContextEdges);
      shiftEdges(m_CurrentEditedRule->m_CutEdge);
      shiftEdges(m_CurrentEditedRule->m_NewEdge);
    }
  }

  void GraphEditor::Impl::RebuildNodeSelList(QComboBox* iBox, uint32_t iCurNode, uint32_t iOtherNode, int iEdgeKind)
  {
    QSignalBlocker blocker(iBox);
    iBox->clear();
    int curIdx = -1;
    for (int i = 0; i < m_NodesList->count(); ++i)
    {
      if (i == iOtherNode
        || ((iEdgeKind != 2) && i >= m_NodeNewStart)
        || ((iEdgeKind != 1) && (i >= m_NodeCutStart && i < m_NodeNewStart)))
      {
        continue;
      }
      if (i == iCurNode)
      {
        curIdx = iBox->count();
      }
      iBox->addItem(m_NodesList->item(i)->text(), i);
    }
    iBox->setCurrentIndex(curIdx);
  }

  void GraphEditor::Impl::RebuildPortSelList(QComboBox* iBox, uint32_t iCurNode, uint32_t iCurPort)
  {
    QSignalBlocker blocker(iBox);
    iBox->clear();
    iBox->addItem("<New Port>", -1);
    int curIdx = 0;
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutEdge.size(); ++i)
    {
      uint32_t edgeIdx = m_EdgeCutStart + i;
      auto const& deletedEdge = m_CurrentEditedRule->m_CutEdge[i];
      if (deletedEdge.nodes[0] == iCurNode
        || deletedEdge.nodes[1] == iCurNode)
      {
        if (edgeIdx == iCurPort)
        {
          curIdx = iBox->count();
        }
        iBox->addItem(m_EdgesList->item(edgeIdx)->text(), edgeIdx);
      }
    }
    iBox->setCurrentIndex(curIdx);
  }
}

#include <gen/pregraph.hpp>
#include <gen/graphutils.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/fruchterman_reingold.hpp>

namespace eXl
{
  void GraphEditor::Impl::UpdateDisplay()
  {
    World& world = m_World.GetWorld();
    for (auto obj : m_DisplayNodes)
    {
      world.DeleteObject(obj);
    }
    m_DisplayNodes.clear();

    m_GraphPainter->Clear();
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
      m_GraphPainter->nodesColor.push_back(qRgb(0,0,255));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_GraphPainter->nodeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i) + " : " + String(nodeTag.get())).c_str()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_CutNodes[i];
      nodeTags.push_back(nodeTag);
      m_GraphPainter->nodesColor.push_back(qRgb(255, 0, 0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_GraphPainter->nodeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i + m_NodeCutStart) + " : " + String(nodeTag.get())).c_str()));
    }

    Vector<ES_RuleSystem::GraphEdge> edges;
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_ContextEdges[i];
      m_GraphPainter->edgesColor.push_back(qRgb(0, 0, 255));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i) + " : " + String(edge.tag.get())).c_str()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CutEdge.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_CutEdge[i];
      m_GraphPainter->edgesColor.push_back(qRgb(255, 0, 0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i + m_EdgeCutStart) + " : " + String(edge.tag.get())).c_str()));
    }

    float dist = Mathf::Max(m_NodesList->count() + 2 / 3, 1) * GraphPainter::s_NodeSize;

    boost::rectangle_topology<> rectangle(-dist * 1.3,-dist, -dist * 0.3 ,dist);
    boost::random_graph_layout(srcGraph, MakeRef(positionMap), rectangle);
    boost::fruchterman_reingold_force_directed_layout(srcGraph, MakeRef(positionMap), rectangle);

    Transforms& trans = *world.GetSystem<Transforms>();
    GfxSystem& gfx = *world.GetSystem<GfxSystem>();
    GameDatabase& database = *world.GetSystem<GameDatabase>();

    for (uint32_t i = 0; i<nodes.size(); ++i)
    {
      auto const& vtx = nodes[i];
      auto pos = boost::get(positionMap, vtx);
      m_GraphPainter->nodes.push_back(QPointF(pos[0],pos[1]));

      Name tag = nodeTags[i];
      auto iter = m_Sys->m_Tags.find(tag);
      if (iter != m_Sys->m_Tags.end()
        && iter->second.m_Archetype.GetUUID().IsValid())
      {
        Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
        if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
        {
          ObjectHandle obj = world.CreateObject();
          database.InstantiateArchetype(obj, arch, nullptr);
          trans.AddTransform(obj, Matrix4f::FromPosition(Vector3f(pos[0], pos[1], 0.0)));
          gfx.CreateSpriteComponent(obj);
          m_DisplayNodes.push_back(obj);
        }
      }
    }

    for (auto const& edge : edges)
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vector2d& pos1V = reinterpret_cast<Vector2d&>(pos1);
      Vector2d& pos2V = reinterpret_cast<Vector2d&>(pos2);
      Vector2d dir = pos2V - pos1V;
      dir.Normalize();
      pos2V -= dir * GraphPainter::s_NodeSize;
      pos1V += dir * GraphPainter::s_NodeSize;
      m_GraphPainter->edges.push_back(qMakePair(QPointF(pos1[0], pos1[1]), QPointF(pos2[0], pos2[1])));
    }

    rectangle = boost::rectangle_topology<>(dist * 0.3, -dist, dist* 1.3, dist);
    srcGraph.clear();
    nodes.clear();
    nodeTags.clear();
    edges.clear();
    positionMap.m_Map.clear();

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_ContextNodes[i];
      nodeTags.push_back(nodeTag);
      m_GraphPainter->nodesColor.push_back(qRgb(0, 0, 255));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) -1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_GraphPainter->nodeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i) + " : " + String(nodeTag.get())).c_str()));
    }
    nodes.resize(m_NodeNewStart, ES_RuleSystem::Graph::null_vertex());
    nodeTags.resize(m_NodeNewStart, Name());
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_CreateNodes.size(); ++i)
    {
      Name nodeTag = m_CurrentEditedRule->m_CreateNodes[i];
      nodeTags.push_back(nodeTag);
      m_GraphPainter->nodesColor.push_back(qRgb(0, 255, 0));
      nodes.push_back(boost::add_vertex(srcGraph));
      boost::put(boost::vertex_index, srcGraph, nodes.back(), boost::num_vertices(srcGraph) - 1);
      boost::put(positionMap, nodes.back(), defaultPos);
      m_GraphPainter->nodeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i + m_NodeNewStart) + " : " + String(nodeTag.get())).c_str()));
    }

    for (uint32_t i = 0; i < m_CurrentEditedRule->m_ContextEdges.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_ContextEdges[i];
      m_GraphPainter->edgesColor.push_back(qRgb(0, 0, 255));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i) + " : " + String(edge.tag.get())).c_str()));
    }
    for (uint32_t i = 0; i < m_CurrentEditedRule->m_NewEdge.size(); ++i)
    {
      auto const& edge = m_CurrentEditedRule->m_NewEdge[i];
      m_GraphPainter->edgesColor.push_back(qRgb(0, 255, 0));
      edges.push_back(boost::add_edge(nodes[edge.nodes[0]], nodes[edge.nodes[1]], srcGraph).first);
      m_GraphPainter->edgeDesc.push_back(QString::fromUtf8(
        (StringUtil::FromInt(i + m_EdgeNewStart) + " : " + String(edge.tag.get())).c_str()));
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
      m_GraphPainter->nodes.push_back(QPointF(pos[0], pos[1]));

      Name tag = nodeTags[i];
      auto iter = m_Sys->m_Tags.find(tag);
      if (iter != m_Sys->m_Tags.end()
        && iter->second.m_Archetype.GetUUID().IsValid())
      {
        Archetype const* arch = iter->second.m_Archetype.GetOrLoad();
        if (arch && arch->GetProperties().count(EngineCommon::GfxSpriteDescName()) > 0)
        {
          ObjectHandle obj = world.CreateObject();
          database.InstantiateArchetype(obj, arch, nullptr);
          trans.AddTransform(obj, Matrix4f::FromPosition(Vector3f(pos[0], pos[1], 0.0)));
          gfx.CreateSpriteComponent(obj);
          m_DisplayNodes.push_back(obj);
        }
      }
    }

    for (auto const& edge : edges)
    {
      auto pos1 = boost::get(positionMap, edge.m_source);
      auto pos2 = boost::get(positionMap, edge.m_target);
      Vector2d& pos1V = reinterpret_cast<Vector2d&>(pos1);
      Vector2d& pos2V = reinterpret_cast<Vector2d&>(pos2);
      Vector2d dir = pos2V - pos1V;
      dir.Normalize();
      pos2V -= dir * GraphPainter::s_NodeSize;
      pos1V += dir * GraphPainter::s_NodeSize;
      m_GraphPainter->edges.push_back(qMakePair(QPointF(pos1[0], pos1[1]), QPointF(pos2[0], pos2[1])));
    }
  }
}