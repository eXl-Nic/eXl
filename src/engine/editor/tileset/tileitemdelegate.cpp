#include "tileitemdelegate.hpp"
#include "tilecollectionmodel.hpp"

#include <editor/objectmodel.hpp>

#include <QComboBox>
#include <QSortFilterProxyModel>

namespace eXl
{
  TileItemDelegate::TileItemDelegate(QWidget *parent)
    : ObjectDelegate(parent)
  {

  }

  void TileItemDelegate::Setup(ObjectModel *iModel, TileCollectionModel* iTileset, Type const* iParentType, uint32_t iFieldNum)
  {
    m_Model = iModel;
    m_Tileset = iTileset;
    m_ParentType = iParentType;
    m_FieldNum = iFieldNum;

    m_FilteredModel = new QSortFilterProxyModel(m_Tileset);
    m_FilteredModel->setSourceModel(m_Tileset);
    m_FilteredModel->sort(0);
  }

  void TileItemDelegate::Clear()
  {
    m_Model = nullptr;
    delete m_FilteredModel;
    m_FilteredModel = nullptr;

    if (m_Tileset != nullptr && children().contains(m_Tileset))
    {
      delete m_Tileset;
    }

    m_Tileset = nullptr;
    m_ParentType = nullptr;
  }

  bool TileItemDelegate::IsOverridenField(QModelIndex iIndex) const
  {
    if (m_Model == nullptr)
    {
      return false;
    }
    QModelIndex parentIdx = iIndex.parent();
    //if (parentIdx.isValid())
    {
      ConstDynObject const& field = m_Model->GetObjectFromIndex(iIndex);
      ConstDynObject const& parentField = m_Model->GetObjectFromIndex(parentIdx);
      if (parentField.GetType() == m_ParentType)
      {
        ConstDynObject testField;
        if (parentField.GetField(m_FieldNum, testField))
        {
          if (testField.GetBuffer() == field.GetBuffer())
          {
            return true;
          }
        }
      }
    }
    return false;
  }

  QWidget* TileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    if (!IsOverridenField(index))
    {
      return ObjectDelegate::createEditor(parent, option, index);
    }

    QComboBox* combo = new QComboBox(parent);
    QObject::connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, combo](int i) 
    {
      emit const_cast<TileItemDelegate*>(this)->commitData(combo); 
    });

    return combo;
  }

  void TileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
  {
    if (!IsOverridenField(index))
    {
      return ObjectDelegate::setEditorData(editor, index);
    }

    ConstDynObject const& tileNameObj = m_Model->GetObjectFromIndex(index);
    TileName tileName = *tileNameObj.CastBuffer<TileName>();

    QComboBox* combo = (QComboBox*)editor;
    combo->setModel(m_FilteredModel);
    QModelIndex tileIndex = m_Tileset->GetIndexFromName(tileName);
    if (tileIndex.isValid())
    {
      combo->setCurrentIndex(m_FilteredModel->mapFromSource(tileIndex).row());
    }
    else
    {
      if (m_Tileset->rowCount(QModelIndex()) > 0)
      {
        combo->setCurrentIndex(0);
      }
    }
  }

  void TileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
  {
    if (!IsOverridenField(index))
    {
      return ObjectDelegate::setModelData(editor, model, index);
    }

    QComboBox* combo = (QComboBox*)editor;
    QModelIndex tileIndex = m_FilteredModel->index(combo->currentIndex(), 0, QModelIndex());
    TileName const* tileName = m_Tileset->GetTileNameFromIndex(m_FilteredModel->mapToSource(tileIndex));
    if (tileName != nullptr)
    {
      model->setData(index, QVariant::fromValue(QString::fromUtf8(tileName->get().c_str())));
    }
  }

  void TileItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
  {
    //if (!IsOverridenField(index))
    {
      return ObjectDelegate::updateEditorGeometry(editor, option, index);
    }
  }

}