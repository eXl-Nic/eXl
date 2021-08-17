#include "tilinggroupeditor.hpp"
#include <editor/editordef.hpp>
#include <editor/aspectratiowidget.hpp>
#include <editor/gamewidget.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/objectmodel.hpp>
#include <editor/objectdelegate.hpp>
#include <editor/collectionmodel.hpp>
#include <editor/eXl_Editor/ui_tilinggroup_toolbox.h>

#include "tilecollectionmodel.hpp"
#include "tileselectionwidget.hpp"
#include "tileitemdelegate.hpp"

#include <engine/common/transforms.hpp>
#include <engine/common/transforms.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/map/tilinggroup.hpp>
#include <math/mathtools.hpp>

#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QListView>
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
  class TilingGroupCollectionModel : public CollectionModel<TilingGroup::PatternName, TilingPattern, TilingGroup>
  {
    //Q_OBJECT
  public:
    static TilingGroupCollectionModel* Create(QObject* iParent, TilingGroup* iGroup);

  protected:
    TilingGroupCollectionModel(QObject* iParent, TilingGroup* iGroup);
    bool AddToResource(TilingGroup::PatternName const& iName, TilingPattern const& iObject) override;
    bool RemoveFromResource(TilingGroup::PatternName const& iName) override;
    TilingPattern const* FindInResource(TilingGroup::PatternName const& iName) const override;
  };

  TilingGroupCollectionModel* TilingGroupCollectionModel::Create(QObject* iParent, TilingGroup* iGroup)
  {
    TilingGroupCollectionModel* newModel = new TilingGroupCollectionModel(iParent, iGroup);
    newModel->BuildMap(iGroup->m_Patterns);

    return newModel;
  }

  TilingGroupCollectionModel::TilingGroupCollectionModel(QObject* iParent, TilingGroup* iGroup)
    : CollectionModel(iParent, iGroup)
  {
  }

  bool TilingGroupCollectionModel::AddToResource(TilingGroup::PatternName const& iName, TilingPattern const& iObject)
  {
    return m_Resource->m_Patterns.insert(std::make_pair(iName, iObject)).second;
  }

  bool TilingGroupCollectionModel::RemoveFromResource(TilingGroup::PatternName const& iName)
  {
    m_Resource->m_Patterns.erase(iName);
    return true;
  }

  TilingPattern const* TilingGroupCollectionModel::FindInResource(TilingGroup::PatternName const& iName) const
  {
    auto iter = m_Resource->m_Patterns.find(iName);
    if (iter != m_Resource->m_Patterns.end())
    {
      return &iter->second;
    }
    return nullptr;
  }

  class TilingEditorToolboxWidget : public QWidget
  {
  public:
  
    TilingEditorToolboxWidget(QWidget* iParent)
      : QWidget(iParent)
    {
      m_Ui.setupUi(this);
    }
  
    Ui::TilingGroupToolbox m_Ui;
  };

  ResourceEditorHandler& TilingGroupEditor::GetEditorHandler()
  {
    static EditorHandler_T<TilingGroup, TilingGroupEditor> s_Handler;
    return s_Handler;
  }

  void TilingGroupEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

	struct TilingGroupEditor::Impl
	{
		Impl()
      : m_World(DunAtk::GetComponents())
		{
			m_Transforms = m_World.AddSystem(std::make_unique<Transforms>());
			m_Gfx = m_World.AddSystem(std::make_unique<GfxSystem>(*m_Transforms));
      m_World.AddSystem(std::make_unique<GameDatabase>(EditorState::GetProjectProperties()));
		}
		World m_World;
		Transforms* m_Transforms;
		GfxSystem* m_Gfx;

    ObjectHandle m_ImageHandle;
    ObjectHandle m_AnimHandle;

    QModelIndex m_GroupSelection;

    TilingGroupCollectionModel* m_GroupCollectionModel;
    QTreeView* m_GroupDataView;
    QListView* m_GroupCollectionView;
    QWidget*   m_GridPatternWidget;

    QModelIndex m_CurrentPatternIdx;

    TilingPattern m_CurrentEditedPattern;
    ObjectModel* m_CurrentObjectModel = nullptr;
    QModelIndex patternSizeIdx;
    QModelIndex patternIdx;
    QModelIndex anchorIdx;
    QModelIndex drawElementsIdx;
    TilingGroupEditor* m_Editor;
    TilingGroup* m_Group;

    TilingEditorToolboxWidget* m_Toolbox;

    void UpdatePattern(QModelIndex iIndex);
    void PatternSizeChanged();
    void RebuildPatternGridWidget();
    void OnPatternDataChanged();
    void UpdateDrawElements();

    TileSelectionWidget* m_DefaultTileSelector;

    QTreeView* m_DrawElements;
	};

	TilingGroupEditor::TilingGroupEditor(QWidget* iParent, DocumentState* iDoc)
		: ResourceEditor(iParent, iDoc)
		, m_Impl(new Impl)
	{

    m_Impl->m_Editor = this;
    m_Impl->m_Group = TilingGroup::DynamicCast(iDoc->GetResource());

    m_Impl->m_GroupCollectionModel = TilingGroupCollectionModel::Create(this, m_Impl->m_Group);

		QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, this);
    QSplitter* viewSplitter = new QSplitter(Qt::Vertical, this);
    QSplitter* dataSplitter = new QSplitter(Qt::Vertical, this);

    QWidget* groupCollection = new QWidget(this);
    QVBoxLayout* groupCollectionLayout = new QVBoxLayout(this);
    groupCollection->setLayout(groupCollectionLayout);
    QToolBar* groupCollectionTool = new QToolBar(this);
    m_Impl->m_GroupCollectionView = new QListView(this);
    m_Impl->m_GroupCollectionView->setModel(m_Impl->m_GroupCollectionModel);
    m_Impl->m_GroupCollectionView->setSelectionModel(new QItemSelectionModel(m_Impl->m_GroupCollectionView->model()));

    QObject::connect(m_Impl->m_GroupCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
    {
      if (iSelected.isEmpty())
      {
        ObjectModel::ClearModelFromView(m_Impl->m_GroupDataView);
        m_Impl->m_CurrentObjectModel = nullptr;
      }
      else
      {
        if (iSelected.indexes().size() == 1)
        {
          m_Impl->m_CurrentPatternIdx = *iSelected.indexes().begin();
          m_Impl->UpdatePattern(m_Impl->m_CurrentPatternIdx);
        }
      }
    });

    QObject::connect(m_Impl->m_GroupCollectionModel, &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
    {
      m_Impl->m_Editor->ModifyResource();
    });

    QObject::connect(m_Impl->m_GroupCollectionModel, &QAbstractItemModel::rowsInserted, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });
    QObject::connect(m_Impl->m_GroupCollectionModel, &QAbstractItemModel::rowsRemoved, [this]()
    {
      m_Impl->m_Editor->ModifyResource();
    });

    groupCollectionLayout->addWidget(new QLabel(QString::fromUtf8("Tileset and Default Tile"), groupCollection));

    TileSelectionWidget::Conf widgetConf;
    m_Impl->m_DefaultTileSelector = new TileSelectionWidget(this, widgetConf, m_Impl->m_Group->GetTileset(), m_Impl->m_Group->m_DefaultTile);
    groupCollectionLayout->addWidget(m_Impl->m_DefaultTileSelector);

    QObject::connect(m_Impl->m_DefaultTileSelector, &TileSelectionWidget::onTilesetChanged, [this]
    {
      m_Impl->m_Group->SetTilesetId(m_Impl->m_DefaultTileSelector->GetTileset().GetUUID());
      m_Impl->UpdateDrawElements();
      m_Impl->m_Editor->ModifyResource();
    });
    QObject::connect(m_Impl->m_DefaultTileSelector, &TileSelectionWidget::onTileChanged, [this]
    {
      m_Impl->m_Group->m_DefaultTile = m_Impl->m_DefaultTileSelector->GetTileName();
      m_Impl->m_Editor->ModifyResource();
    });

    groupCollectionLayout->addWidget(new QLabel(QString::fromUtf8("Patterns"), groupCollection));
    groupCollectionLayout->addWidget(groupCollectionTool);
    groupCollectionLayout->addWidget(m_Impl->m_GroupCollectionView);

    groupCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this]
    {
      uint32_t curRowCount = m_Impl->m_GroupCollectionModel->rowCount(QModelIndex());
      m_Impl->m_GroupCollectionModel->insertRow(curRowCount);
      
    });

    groupCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
    {
      QModelIndex currentSelection = m_Impl->m_GroupCollectionView->selectionModel()->currentIndex();
      if (currentSelection.isValid())
      {
        m_Impl->m_GroupCollectionModel->removeRow(currentSelection.row(), currentSelection.parent());
      }
    });

		m_Impl->m_GroupDataView = new QTreeView(this);
    m_Impl->m_GroupDataView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_Impl->m_GroupDataView->setItemDelegate(new ObjectDelegate);

    dataSplitter->addWidget(groupCollection);
    dataSplitter->addWidget(m_Impl->m_GroupDataView);

    {
      m_Impl->m_Toolbox = new TilingEditorToolboxWidget(this);

      QObject::connect(m_Impl->m_Toolbox->m_Ui.patternSizeX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_CurrentEditedPattern.patternSize.X() = i;
        if (m_Impl->m_CurrentObjectModel)
        {
          emit m_Impl->m_CurrentObjectModel->dataChanged(m_Impl->patternSizeIdx, m_Impl->patternSizeIdx);
        }
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.patternSizeY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_CurrentEditedPattern.patternSize.Y() = i;
        if (m_Impl->m_CurrentObjectModel)
        {
          emit m_Impl->m_CurrentObjectModel->dataChanged(m_Impl->patternSizeIdx, m_Impl->patternSizeIdx);
        }
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.anchorX, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_CurrentEditedPattern.anchor.X() = i;
        if (m_Impl->m_CurrentObjectModel)
        {
          emit m_Impl->m_CurrentObjectModel->dataChanged(m_Impl->anchorIdx, m_Impl->anchorIdx);
        }
      });
      QObject::connect(m_Impl->m_Toolbox->m_Ui.anchorY, QOverload<int>::of(&QSpinBox::valueChanged), [this](int i)
      {
        m_Impl->m_CurrentEditedPattern.anchor.Y() = i;
        if (m_Impl->m_CurrentObjectModel)
        {
          emit m_Impl->m_CurrentObjectModel->dataChanged(m_Impl->anchorIdx, m_Impl->anchorIdx);
        }
      });

      QWidget* workspaceWidget = new QWidget(this);
      QVBoxLayout* workspaceLayout = new QVBoxLayout(workspaceWidget);
      workspaceLayout->setSizeConstraint(QLayout::SetMinimumSize);
      workspaceWidget->setLayout(workspaceLayout);

      workspaceLayout->addWidget(m_Impl->m_Toolbox);

      m_Impl->m_GridPatternWidget = new QWidget(this);
      m_Impl->m_GridPatternWidget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
      workspaceLayout->addWidget(m_Impl->m_GridPatternWidget);

      viewSplitter->addWidget(workspaceWidget);

      QToolBar* drawElementsTools = new QToolBar(this);
      m_Impl->m_DrawElements = new QTreeView(this);
      m_Impl->m_DrawElements->setItemDelegate(new TileItemDelegate(m_Impl->m_DrawElements));
      drawElementsTools->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add Element", [this]
      {
        uint32_t curRowCount = m_Impl->m_CurrentObjectModel->rowCount(m_Impl->drawElementsIdx);
        m_Impl->m_CurrentObjectModel->insertRow(curRowCount, m_Impl->drawElementsIdx);
      });

      drawElementsTools->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Element", [this]
      {
        QModelIndex currentSelection = m_Impl->m_DrawElements->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          m_Impl->m_CurrentObjectModel->removeRow(currentSelection.row(), currentSelection.parent());
        }
      });

      viewSplitter->addWidget(drawElementsTools);
      viewSplitter->addWidget(m_Impl->m_DrawElements);

    }
		rootSplitter->addWidget(dataSplitter);
		rootSplitter->addWidget(viewSplitter);

		QVBoxLayout* layout = new QVBoxLayout(this);

		layout->addWidget(rootSplitter);

		setLayout(layout);

    m_Impl->UpdateDrawElements();
	}

  void TilingGroupEditor::Impl::OnPatternDataChanged()
  {
    m_Editor->ModifyResource();
    TilingGroup::PatternName const* patternName = m_GroupCollectionModel->GetNameFromIndex(m_CurrentPatternIdx);
    eXl_ASSERT(patternName != nullptr);
    m_Group->m_Patterns[*patternName] = m_CurrentEditedPattern;
  };

  void TilingGroupEditor::Impl::PatternSizeChanged()
  {
    uint32_t totSize = m_CurrentEditedPattern.patternSize.X() * m_CurrentEditedPattern.patternSize.Y();
    if (m_CurrentEditedPattern.pattern.size() != totSize)
    {
      m_CurrentEditedPattern.pattern.resize(totSize);
      std::fill(m_CurrentEditedPattern.pattern.begin(), m_CurrentEditedPattern.pattern.end(), TilingGroupLocConstraint::Undefined);

      if (m_CurrentObjectModel)
      {
        m_CurrentObjectModel->UpdateModel(DynObject(TilingPattern::GetType(), &m_CurrentEditedPattern));
      }

      RebuildPatternGridWidget();
      UpdateDrawElements();
      OnPatternDataChanged();
    }
  }

  void TilingGroupEditor::Impl::UpdateDrawElements()
  {
    TileItemDelegate* itemDelegate = static_cast<TileItemDelegate*>(m_DrawElements->itemDelegate());
    if (m_CurrentObjectModel)
    {
      m_DrawElements->setModel(m_CurrentObjectModel);
      m_DrawElements->setRootIndex(drawElementsIdx);
      if (m_DefaultTileSelector->GetTileCollection())
      {
        TupleType const* drawElementsType = TupleType::DynamicCast(TilingDrawElement::GetType());
        uint32_t fieldIdx;
        drawElementsType->GetFieldDetails("m_Name", fieldIdx);
        itemDelegate->Setup(m_CurrentObjectModel, m_DefaultTileSelector->GetTileCollection(), drawElementsType, fieldIdx);
      }
      else
      {
        itemDelegate->Clear();
      }
    }
    else
    {
      m_DrawElements->setModel(nullptr);
      itemDelegate->Clear();
    }
  }

  void TilingGroupEditor::Impl::UpdatePattern(QModelIndex iIndex)
  {
    if (iIndex.isValid())
    {
      TilingPattern const* pattern = m_GroupCollectionModel->GetObjectFromIndex(iIndex);
      eXl_ASSERT(pattern != nullptr);

      m_CurrentEditedPattern = *pattern;

      m_CurrentObjectModel = ObjectModel::CreateOrUpdateModel(m_GroupDataView, false, DynObject(TilingPattern::GetType(), &m_CurrentEditedPattern));

      m_Toolbox->m_Ui.anchorX->setValue(m_CurrentEditedPattern.anchor.X());
      m_Toolbox->m_Ui.anchorY->setValue(m_CurrentEditedPattern.anchor.Y());
      m_Toolbox->m_Ui.patternSizeX->setValue(m_CurrentEditedPattern.patternSize.X());
      m_Toolbox->m_Ui.patternSizeY->setValue(m_CurrentEditedPattern.patternSize.Y());

      uint32_t rows = m_CurrentObjectModel->rowCount(QModelIndex());
      for (uint32_t i = 0; i < rows; ++i)
      {
        QModelIndex index = m_CurrentObjectModel->index(i, 0, QModelIndex());
        ConstDynObject const& field = m_CurrentObjectModel->GetObjectFromIndex(index);
        if (field.GetBuffer() == &m_CurrentEditedPattern.patternSize)
        {
          patternSizeIdx = index;
        }
        if (field.GetBuffer() == &m_CurrentEditedPattern.pattern)
        {
          patternIdx = index;
        }
        if (field.GetBuffer() == &m_CurrentEditedPattern.anchor)
        {
          anchorIdx = index;
        }
        if (field.GetBuffer() == &m_CurrentEditedPattern.drawElement)
        {
          drawElementsIdx = index;
        }
      }

      QObject::connect(m_GroupDataView->model(), &QAbstractItemModel::dataChanged, [this](QModelIndex const& iDataIndex)
      {
        if (iDataIndex == patternSizeIdx)
        {
          PatternSizeChanged();
        }
        OnPatternDataChanged();
      });

      QObject::connect(m_GroupDataView->model(), &QAbstractItemModel::rowsInserted, [this] { OnPatternDataChanged(); });
      QObject::connect(m_GroupDataView->model(), &QAbstractItemModel::rowsRemoved, [this] { OnPatternDataChanged(); });
      RebuildPatternGridWidget();
      UpdateDrawElements();
    }
  }

  void SetIconFromConstraint(QPushButton* iButton, TilingGroupLocConstraint iConstraint)
  {
    switch (iConstraint)
    {
    case TilingGroupLocConstraint::Undefined:
      iButton->setIcon(iButton->style()->standardIcon(QStyle::SP_CustomBase));
      break;
    case TilingGroupLocConstraint::SameGroup:
      iButton->setIcon(iButton->style()->standardIcon(QStyle::SP_DialogApplyButton));
      break;
    case TilingGroupLocConstraint::OtherGroup:
      iButton->setIcon(iButton->style()->standardIcon(QStyle::SP_MessageBoxCritical));
      break;
    }
  }

  void TilingGroupEditor::Impl::RebuildPatternGridWidget()
  {
    if (QLayout* prevLayout = m_GridPatternWidget->layout())
    {
      qDeleteAll(m_GridPatternWidget->children());
    }

    QGridLayout* layout = new QGridLayout(m_GridPatternWidget);
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    Vector2i size = m_CurrentEditedPattern.patternSize;
    for (int32_t y = 0; y < size.Y(); ++y)
    {
      for (int32_t x = 0; x < size.X(); ++x)
      {
        uint32_t offset = x + y * size.X();
        TilingGroupLocConstraint constraint = m_CurrentEditedPattern.pattern[offset];

        int32_t revY = (size.Y() - 1) - y;
        QPushButton* gridElement = new QPushButton(m_GridPatternWidget);
        layout->addWidget(gridElement, revY, x, 1, 1, Qt::AlignLeft | Qt::AlignTop);
        gridElement->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        SetIconFromConstraint(gridElement, constraint);

        QObject::connect(gridElement, &QPushButton::pressed, [this, gridElement, offset]() 
        {
          TilingGroupLocConstraint constraint = m_CurrentEditedPattern.pattern[offset];
          constraint = TilingGroupLocConstraint(((uint32_t)constraint + 1) % 3);
          m_CurrentEditedPattern.pattern[offset] = constraint;
          SetIconFromConstraint(gridElement, constraint);
          
          if (m_CurrentObjectModel)
          {
            QModelIndex itemIdx = m_CurrentObjectModel->index(offset, 1, patternIdx);
            emit m_CurrentObjectModel->dataChanged(itemIdx, itemIdx);
          }

          OnPatternDataChanged();
        });
      }
    }
  }
}