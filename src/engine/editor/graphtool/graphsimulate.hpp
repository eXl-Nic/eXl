#pragma once

#include <QWidget>
#include "graphdata.hpp"

namespace eXl
{
  class GraphEditor;
  class GraphSimulateWidget : public QWidget
  {
  public:
    GraphSimulateWidget(GraphEditor* iParent, RewriteSystem& iSys);
    ~GraphSimulateWidget();
    void SetSelectedRule(String const&);

  protected:

    struct Impl;
    UniquePtr<Impl> m_Impl;
  };
}