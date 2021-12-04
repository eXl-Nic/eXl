#include "objectdelegate.hpp"
#include "editordef.hpp"
#include "dynobjecteditor.hpp"
//#include <core/type/resourcehandletype.hpp>
#include <core/type/enumtype.hpp>
#include "eXl_Editor/enum_editor.h"
#include "eXl_Editor/array_editor.h"
#include "eXl_Editor/arrayitem_editor.h"

#include <QItemEditorFactory>

#include <core/type/dynobject.hpp>
#include <core/type/arraytype.hpp>

#include <QtCore/QVariant>
#include <QPainter>

#include <core/stream/jsonstreamer.hpp>

#include <sstream>

namespace eXl
{
  typedef std::map<Type const*, ObjectDelegate::ObjectHandler*> EditorMap;
  static EditorMap s_EditorMap;

  static Type const* s_UIntId      = TypeManager::GetType<unsigned int>();
  static Type const* s_IntId       = TypeManager::GetType<int>();
  static Type const* s_UCharId     = TypeManager::GetType<unsigned char>();
  static Type const* s_FloatId     = TypeManager::GetType<float>();
  static Type const* s_StringId    = TypeManager::GetType<String>();

  ObjectDelegate::ObjectDelegate(QWidget* iParent)
    :QStyledItemDelegate(iParent)
  {
  }

  void ObjectDelegate::RegisterType(ObjectHandler* iHandler, Type const* iType)
  {
    s_EditorMap[iType]=iHandler;
  }

  void ObjectDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    QVariant editorData;
    QVariant data = index.data(Qt::DisplayRole);
    if (!data.isValid())
    {
      return QStyledItemDelegate::paint(painter, option, index);
    }
    ConstDynObject const* obj = NULL;
    if (data.type() >= QVariant::UserType)
    {
      obj = data.value<ConstDynObject const*>();
      DynObjToQVariant(*obj, editorData);
    }
    else
    {
      editorData.swap(data);
    }

    if (editorData.isValid())
    {
      return QStyledItemDelegate::paint(painter, option, index);
    }
    else
    {
      Type const* dataType = obj->GetType();

      EnumType const* enumType = EnumType::DynamicCast(dataType);
      if (enumType != NULL)
      {
        TypeEnumName name;
        if (enumType->GetEnumName(*reinterpret_cast<int const*>(obj->GetBuffer()), name))
        {
          painter->drawText(option.rect.bottomLeft(), QString::fromUtf8(name.c_str()));
        }
        else
        {
          painter->drawText(option.rect.bottomLeft(), QString::fromUtf8("ERROR, INVALID ENUM VALUE"));
        }
      }
      else
      {
        EditorMap::iterator iter = s_EditorMap.find(dataType);
        if (iter != s_EditorMap.end())
        {
          iter->second->paint(painter, option, *obj);
        }
        else
        {
          std::stringstream sstream;
          JSONStreamer streamer(&sstream);
          streamer.Begin();
          dataType->Stream(obj->GetBuffer(),&streamer);
          streamer.End();

          painter->drawText(QPoint(0, 0), QString::fromUtf8(sstream.str().c_str()));
        }
      }
    }
    //QString text;
    //QRect displayRect;
    //QVariant value = index.data(Qt::DisplayRole);
    //if (value.isValid() && !value.isNull()) 
    //{
    //  text = value.toString();
    //  displayRect = textRectangle(painter, d->textLayoutBounds(option), option.font, text);
    //}
  }

  void ObjectDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const
  {
    QStyledItemDelegate::updateEditorGeometry(editor,option,index);
    //editor->setGeometry(option.rect);
  }

  QSize ObjectDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    return QStyledItemDelegate::sizeHint(option,index);
  }

  QWidget* ObjectDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
  {
    QVariant editorData;
    QVariant data = index.data(Qt::EditRole);
    if (!data.isValid())
    {
      return nullptr;
    }

    ConstDynObject const* obj = NULL;
    if(data.type() >= QVariant::UserType)
    {
      if (data.canConvert<ArrayItem>())
      {
        ArrayItem item = data.value<ArrayItem>();

        ArrayItem_Editor* editor = new ArrayItem_Editor(item.index, parent);

        QObject::connect(editor, &ArrayItem_Editor::onRemoveItem, [index]()
        {
          QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
          model->removeRow(index.row(), index.parent());
        });

        QObject::connect(editor, &ArrayItem_Editor::onInsertItem, [index]()
        {
          QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
          model->insertRow(index.row(), index.parent());
        });

        return editor;
      }
      else if (data.canConvert<ConstDynObject const*>())
      {
        obj = data.value<ConstDynObject const*>();
        DynObjToQVariant(*obj, editorData);
      }
    }
    else
    {
      editorData.swap(data);
    }

    if(editorData.isValid())
    {
      QWidget* editor = QStyledItemDelegate::createEditor(parent,option,index);
      connect(editor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
      return editor;
    }
    else
    {
      QWidget* editor = nullptr;
      Type const* dataType = obj->GetType();

      EnumType const* enumType = EnumType::DynamicCast(dataType);
      ArrayType const* arrayType = ArrayType::DynamicCast(dataType);
      if(enumType != nullptr)
      {
        editor = Enum_Editor::Construct(parent);
        connect(editor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
      }

      if (arrayType != nullptr)
      {
        editor = new Array_Editor(parent);
        QObject::connect((Array_Editor*)editor, &Array_Editor::onAddItem, [index]()
        {
          QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
          QModelIndex arrayIndex = model->index(index.row(), 0, index.parent());
          model->insertRow(model->rowCount(arrayIndex), arrayIndex);
        });

        QObject::connect((Array_Editor*)editor, &Array_Editor::onEmptyArray, [index]()
        {
          QAbstractItemModel* model = const_cast<QAbstractItemModel*>(index.model());
          QModelIndex arrayIndex = model->index(index.row(), 0, index.parent());
          model->removeRows(0, model->rowCount(arrayIndex), arrayIndex);
        });
      }

      if(editor == nullptr)
      {
        EditorMap::iterator iter = s_EditorMap.find(dataType);
        if(iter != s_EditorMap.end())
        {
          editor = iter->second->CreateEditor(parent);
        }
        connect(editor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));
      }

      return editor;
    }
  }

  void ObjectDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
  {
    QVariant editorData;
    QVariant data = index.data(Qt::EditRole);
    ConstDynObject const* obj = NULL;
    if(data.type() >= QVariant::UserType)
    {
      if (data.canConvert<ArrayItem>())
      {
        return;
      }
      else if (data.canConvert<ConstDynObject const*>())
      {
        obj = data.value<ConstDynObject const*>();
        DynObjToQVariant(*obj, editorData);
      }
    }
    else
    {
      editorData.swap(data);
    }

    if(editorData.isValid())
    {
      QByteArray n = editor->metaObject()->userProperty().name();
      if (!n.isEmpty()) 
      {
        editor->setProperty(n, editorData);
      }
    }
    else
    {
      //DynObject objCopy(obj);
      editor->setProperty("eXlObject", qVariantFromValue(obj));
    }
  }

  void ObjectDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
  {
    QVariant editorData;
    QVariant data = index.data(Qt::EditRole);
    ConstDynObject const* obj = NULL;
    if(data.type() >= QVariant::UserType)
    {
      if (data.canConvert<ArrayItem>())
      {
        return ;
      }
      else if (data.canConvert<ConstDynObject const*>())
      {
        obj = data.value<ConstDynObject const*>();
        DynObjToQVariant(*obj, editorData);
      }
    }
    else
    {
      editorData.swap(data);
    }

    if(editorData.isValid())
    {
      return QStyledItemDelegate::setModelData(editor,model,index);
    }
    else
    {
      Type const* dataType = obj->GetType();
      if (ArrayType::DynamicCast(dataType))
      {
        return ;
      }
      QVariant data = editor->property("eXlObject");
      DynObject objCopy(data.value<ConstDynObject const*>());
      model->setData(index,qVariantFromValue(objCopy),Qt::EditRole);
    }
  }

  void ObjectDelegate::commitAndCloseEditor()
  {
    QWidget* editor = qobject_cast<QWidget*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
  }
}