#pragma once

#include <QWidget>

#include <core/resource/resource.hpp>
#include "projectresourcesmodel.hpp"

class QComboBox;
class QListView;
class QSortFilterProxyModel;

namespace eXl
{
  class ResourceSelectionWidget : public QWidget
  {
    Q_OBJECT
  public:

	  enum Widget
	  {
		  Combo,
		  List
	  };

    ResourceSelectionWidget(QWidget* iParent, ResourceLoaderName iName, Widget iConf, Resource::UUID const& iRsc = Resource::UUID());

    void ForceSelection(Resource::UUID const& iRsc);
    //void ResetModel();

    Resource::UUID GetSelectedResourceId() const { return m_Selected; }

  Q_SIGNALS:
    void onResourceChanged();

  protected:
    Resource::UUID m_Selected;
    ProjectResourcesModel::TypeFilter* m_Collection = nullptr;
    QSortFilterProxyModel* m_FilteredModel = nullptr;
    QComboBox* m_Selector = nullptr;
    QListView* m_List = nullptr;
  };

  template <typename T>
  class TResourceSelectionWidget : public ResourceSelectionWidget
  {
  public:
  
    TResourceSelectionWidget(QWidget* iParent, ResourceSelectionWidget::Widget iConf, ResourceHandle<T> const& iRsc = ResourceHandle<T>())
		  : ResourceSelectionWidget(iParent, T::StaticLoaderName(), iConf, iRsc.GetUUID())
	  {}
  
	  void ForceSelection(ResourceHandle<T> const& iRsc)
	  { ForceSelection(iRsc.GetUUID()); }
	  
    ResourceHandle<T> GetSelectedResource() const { ResourceHandle<T> handle; handle.SetUUID(m_Selected); return handle; }
  };
}