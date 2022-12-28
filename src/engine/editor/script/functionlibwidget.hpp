#pragma once

#include <QWidget>
#include <QListWidget>
#include <engine/script/luafunctionlibrary.hpp>

namespace eXl 
{
  class LuaFunctionLibrarySelector : public QWidget
  {
    Q_OBJECT
  public:

    LuaFunctionLibrarySelector(QWidget* iParent, Vector<ResourceHandle<LuaFunctionLibrary>> const & iInit);

    Vector<ResourceHandle<LuaFunctionLibrary>> GetList();

  Q_SIGNALS:
    void onListChanged();

  protected:
    QListWidget* m_List;
  };

}