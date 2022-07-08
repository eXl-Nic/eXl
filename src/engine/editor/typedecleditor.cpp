#include "typedecleditor.hpp"

#include <core/type/typemanager.hpp>

#include <QTabWidget>
#include <QTableWidget>
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
#include <QLineEdit>

namespace eXl
{
  TypeDeclEditor::TypeDeclEditor(QWidget* iParent)
    : QWidget(iParent)
  {
    Vector<Type const*> types = TypeManager::GetCoreTypes();
    std::sort(types.begin(), types.end(), [](Type const* const& iType1, Type const* const& iType2)
      {
        return iType1->GetDisplayName() < iType2->GetDisplayName();
      }
    );

    for (auto type : types)
    {
      m_TypeNames.append(QString::fromUtf8(type->GetName().c_str()));
    }

    for (auto type : types)
    {
      m_TypeDisplayNames.append(QString::fromUtf8(type->GetDisplayName().c_str()));
    }
  }

  TypeDeclEditor* TypeDeclEditor::Create(QWidget* iParent)
  {
    TypeDeclEditor* editor = new TypeDeclEditor(iParent);
    QVBoxLayout* propDataLayout = new QVBoxLayout(editor);
    editor->setLayout(propDataLayout);
    editor->Build(propDataLayout);

    return editor;
  }

  void TypeDeclEditor::Build(QLayout* iLayout)
  {
    m_PropDataView = new QTableWidget(this);

    QToolBar* propDataTool = new QToolBar(this);

    propDataTool->addAction(parentWidget()->style()->standardIcon(QStyle::SP_FileIcon), "Add New Field", [this]
      {
        Project::Field newField;
        newField.m_Name = "NewField";
        newField.m_TypeName = "uint32_t";
        newField.m_IsArray = false;
        m_Decl.m_Fields.push_back(newField);
        UpdateEditedProperty();
        OnDeclChange();
      });

    propDataTool->addAction(parentWidget()->style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Field", [this]
      {
        QTableWidgetItem* currentSelection = m_PropDataView->currentItem();
        if (currentSelection != nullptr)
        {
          int32_t row = currentSelection->row();
          m_Decl.m_Fields.erase(m_Decl.m_Fields.begin() + row);
          UpdateEditedProperty();
          OnDeclChange();
        }
      });


    iLayout->addWidget(propDataTool);
    iLayout->addWidget(m_PropDataView);
  }

  void TypeDeclEditor::SetDecl(Project::TypeDecl const& iDecl)
  {
    Clear();
    m_Decl = iDecl;
    UpdateEditedProperty();
  }

  void TypeDeclEditor::Clear()
  {
    QObject::disconnect(m_PropDataChangeCb);
    m_PropDataView->clear();
  }

  void TypeDeclEditor::UpdateEditedProperty()
  {
    Clear();
    m_PropDataView->setRowCount(m_Decl.m_Fields.size());
    m_PropDataView->setColumnCount(3);

    auto onSheetDataChanged = [this](QModelIndex const& iIndex)
    {
      QTableWidgetItem* item = m_PropDataView->item(iIndex.row(), iIndex.column());
      switch (iIndex.column())
      {
      case 0:
      {
        TypeFieldName newFieldName = item->text().toUtf8();
        Project::Field& fieldData = m_Decl.m_Fields[iIndex.row()];
        fieldData.m_Name = newFieldName;
      }
      break;
      case 1:
      {
        QComboBox* typeSelector = static_cast<QComboBox*>(m_PropDataView->cellWidget(iIndex.row(), iIndex.column()));
        m_Decl.m_Fields[iIndex.row()].m_TypeName = m_TypeNames[typeSelector->currentIndex()].toUtf8();
      }
      break;
      case 2:
      {
        TypeFieldName fieldName = item->data(Qt::UserRole).toString().toUtf8();
        m_Decl.m_Fields[iIndex.row()].m_IsArray = item->checkState() == Qt::Checked;
      }
      break;
      }
      OnDeclChange();
    };
    
    
    for (uint32_t numRow = 0; numRow < m_Decl.m_Fields.size(); ++numRow)
    {
      auto const& field = m_Decl.m_Fields[numRow];
      QTableWidgetItem* nameItem = new QTableWidgetItem;
      QString fieldName = QString::fromUtf8(field.m_Name.c_str());
      nameItem->setData(Qt::UserRole, QVariant(fieldName));
      nameItem->setText(fieldName);
      m_PropDataView->setItem(numRow, 0, nameItem);
      
      QString typeNameStr = QString::fromUtf8(field.m_TypeName.c_str());

      bool foundType = false;
      int32_t selType = 0;
      QComboBox* typeCombo = new QComboBox(m_PropDataView);
      for(int32_t i = 0; i<m_TypeNames.size(); ++i)
      {
        typeCombo->addItem(m_TypeDisplayNames[i]);
        if (m_TypeNames[i] == typeNameStr)
        {
          foundType = true;
        }
        if (!foundType)
        {
          ++selType;
        }
      }
      if (selType < m_TypeNames.size())
      {
        typeCombo->setCurrentIndex(selType);
      }
      m_PropDataView->setCellWidget(numRow, 1, typeCombo);
      QObject::connect(typeCombo, (void (QComboBox::*)(int ))&QComboBox::currentIndexChanged, 
        [onSheetDataChanged, index = m_PropDataView->model()->index(numRow, 1)](int)
        { onSheetDataChanged(index); });

      QTableWidgetItem* isArrayItem = new QTableWidgetItem;
      isArrayItem->setData(Qt::UserRole, QVariant(fieldName));
      isArrayItem->setCheckState(field.m_IsArray ? Qt::Checked : Qt::Unchecked);
      m_PropDataView->setItem(numRow, 2, isArrayItem);
    }
    
    m_PropDataChangeCb = QObject::connect(m_PropDataView->model(), &QAbstractItemModel::dataChanged, [onSheetDataChanged](QModelIndex const& iDataIndex)
    {
      onSheetDataChanged(iDataIndex);
    });

    //QObject::connect(m_Impl->m_PropDataView->model(), &QAbstractItemModel::rowsInserted, onSheetDataChanged);
    //QObject::connect(m_Impl->m_PropDataView->model(), &QAbstractItemModel::rowsRemoved, onSheetDataChanged);
  }
}