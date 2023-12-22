#pragma once

#include <QWidget>
#include <engine/map/graphdata.hpp>

namespace eXl
{
  class GraphEditor;
  class GraphSimulateWidget : public QWidget
  {
  public:
    GraphSimulateWidget(GraphEditor* iParent, RewriteSystemRsc& iSys);
    ~GraphSimulateWidget();
    void SetSelectedRule(String const&);

  protected:

    struct Impl;
    UniquePtr<Impl> m_Impl;
  };
}