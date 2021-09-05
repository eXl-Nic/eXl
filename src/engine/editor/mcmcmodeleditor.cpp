#include "mcmcmodeleditor.hpp"
#include "editordef.hpp"
//#include "editorstate.hpp"
#include "miniaturecache.hpp"

#include <engine/map/mcmcmodelrsc.hpp>

#include <gen/mcmcsynthesis.hpp>

#include <QBoxLayout>
#include <QComboBox>
#include <QFileDialog>
#include <QPushButton>
#include <QSplitter>
#include <QListWidget>
#include <QTableView>
#include <QLabel>

namespace eXl
{
  ResourceEditorHandler& MCMCModelEditor::GetEditorHandler()
  {
    static EditorHandler_T<MCMCModelRsc, MCMCModelEditor> s_Handler;
    return s_Handler;
  }

  struct MCMCModelEditor::Impl
  {
    Impl(MCMCModelEditor*, MCMCModelRsc*);

    void GenerateDefaultBehaviourSource();

    MCMCModelEditor* m_Editor;
    MCMCModelRsc* m_Model;
    QListWidget* m_ElementsView;
    QListWidget* m_InteractionCollectionView;
    QTableView* m_ElementData;
    QLabel* m_ImageView;
    MiniatureCache m_Cache;
  };

  void MCMCModelEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  MCMCModelEditor::MCMCModelEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl(this, MCMCModelRsc::DynamicCast(iDoc->GetResource())))
  {
  }

  MCMCModelEditor::Impl::Impl(MCMCModelEditor* iEditor, MCMCModelRsc* iModel)
    : m_Editor(iEditor)
    , m_Model(iModel)
  {
    QVBoxLayout* layout = new QVBoxLayout(m_Editor);

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, m_Editor);
    QSplitter* viewSplitter = new QSplitter(Qt::Vertical, m_Editor);
    QSplitter* dataSplitter = new QSplitter(Qt::Vertical, m_Editor);

    QWidget* interactionCollection = new QWidget(m_Editor);
    QVBoxLayout* interactionCollectionLayout = new QVBoxLayout(m_Editor);

    m_ElementsView = new QListWidget(m_Editor);
    m_InteractionCollectionView = new QListWidget(m_Editor);
    m_ElementData = new QTableView(m_Editor);
    
    MCMC2D::LearnedModel* model = m_Model->m_Model.get();
    if (model != nullptr)
    {
      auto const& elements = model->GetElements();
      MCMC2D::InputBuilder builder(elements, model->IsToroidal());

      for (uint32_t i = 0; i<m_Model->m_ElementsVector.size(); ++i)
      {
        auto rscKey = m_Model->m_ElementsVector[i];
        QListWidgetItem* item = new QListWidgetItem(m_ElementsView);
        item->setText(StringUtil::FromInt(i + 1).c_str());
        item->setData(Qt::DecorationRole, m_Cache.GetMiniature(rscKey.m_Resource, rscKey.m_Subobject));
        m_ElementsView->addItem(item);
      }

      Vector<QListWidgetItem*> items;
      items.resize(builder.oneHotSize);

      for (int i = 0; i < elements.size(); ++i)
      {
        int startElem = model->IsToroidal() ? 0 : -1;
        for (int j = startElem; j < static_cast<int>(elements.size() - i); ++j)
        {
          QListWidgetItem* item = new QListWidgetItem();
          if (j < 0)
          {
            item->setText((String("Wall <-> ") + StringUtil::FromInt(i + 1)).c_str());
          }
          else
          {
            item->setText((StringUtil::FromInt(i + 1) + String(" <-> ") + StringUtil::FromInt(j + i + 1)).c_str());
          }
          items[builder.BuildOneHot(i, j, 0.0)] = item;
        }
      }
      for (auto item : items)
      {
        m_InteractionCollectionView->addItem(item);
      }
    }

    QObject::connect(m_InteractionCollectionView->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_ImageView->clear();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            MCMC2D::LearnedModel* model = m_Model->m_Model.get();
            if(model)
            { 
              Image dbgImg = MCMC2D::DrawDbgImg(model, iSelected.indexes()[0].row());
              QImage convImage;
              eXlImageToQImage(dbgImg, convImage);
              m_ImageView->setPixmap(QPixmap::fromImage(convImage));
            }
          }
        }
      });

    interactionCollectionLayout->addWidget(m_InteractionCollectionView);
    interactionCollectionLayout->addWidget(m_ElementsView);
    interactionCollection->setLayout(interactionCollectionLayout);

    dataSplitter->addWidget(interactionCollection);
    dataSplitter->addWidget(m_ElementData);

    m_ImageView = new QLabel(m_Editor);
    viewSplitter->addWidget(m_ImageView);

    rootSplitter->addWidget(dataSplitter);
    rootSplitter->addWidget(viewSplitter);

    layout->addWidget(rootSplitter);
    m_Editor->setLayout(layout);
  }
}