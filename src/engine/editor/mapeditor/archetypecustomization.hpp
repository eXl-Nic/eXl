#pragma once

#include <QAbstractItemModel>
#include <vector>
#include <core/type/dynobject.hpp>

#include <engine/map/map.hpp>

class QAbstractItemView;

namespace eXl
{
  class DynObject;
  class CustomizationModel;

  class ArchetypeCustomizationModel : public QAbstractItemModel
  {
    Q_OBJECT
  public:

    static void ClearModelFromView(QAbstractItemView* iView);
    static ArchetypeCustomizationModel* CreateOrUpdateModel(QAbstractItemView* iView, Archetype const* iArchetype, CustomizationData const& iData);

    ArchetypeCustomizationModel(QObject* iParent, Archetype const* iArchetype, CustomizationData const& iData);

    void UpdateModel(Archetype const* iArchetype, CustomizationData const& iData);

    CustomizationData GetCustomization();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

  protected:

    struct TreeItem
    {
      TreeItem(TreeItem* iParent) : parent(iParent)
      {}
      TreeItem(TreeItem const& ) = delete;
      TreeItem& operator=(TreeItem const&) = delete;
      TreeItem* parent;
      QModelIndex srcIndex;
      Name objectName;

      struct Deleter
      {
        Deleter(ArchetypeCustomizationModel& iModel)
          : m_Model(iModel)
        {}
        Deleter(Deleter const& iOther)
          : m_Model(iOther.m_Model)
        {}
        Deleter(Deleter&& iOther)
          : m_Model(iOther.m_Model)
        {}
        Deleter& operator=(Deleter const& iOther)
        {
          this->~Deleter(); new (this)Deleter(iOther.m_Model); return *this;
        }
        Deleter& operator=(Deleter&& iOther)
        {
          this->~Deleter(); new (this)Deleter(iOther.m_Model); return *this;
        }

        void operator()(TreeItem* iItem) const;

        ArchetypeCustomizationModel& m_Model;
      };

      Vector<std::unique_ptr<TreeItem, Deleter>> children;
    };

    void AddCallbacks(TreeItem* iRoot, Name iName, CustomizationModel* iModel);

    //QModelIndex IndexOf(TreeItem* iNode) const;
    QModelIndex SourceIndex(QModelIndex iIndex, CustomizationModel*& oModel) const;

    std::unique_ptr<TreeItem, TreeItem::Deleter> BuildTree(QModelIndex iIndex, TreeItem* iParent, Name iName, CustomizationModel* iModel);

    bool IsValidField(QModelIndex iIndex) const;

    Archetype const* m_Archetype;
    UnorderedMap<ComponentName, std::unique_ptr<CustomizationModel>> m_Components;
    UnorderedMap<PropertySheetName, std::unique_ptr<CustomizationModel>> m_Properties;

    QHash<QModelIndex, TreeItem*> m_SourceIndexMap;
    
    TreeItem m_Root;
    TreeItem* m_ComponentRoot;
    TreeItem* m_PropertiesRoot;
  };
}