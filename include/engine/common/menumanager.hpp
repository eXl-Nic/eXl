#pragma once

#include <engine/enginelib.hpp>

#include <core/string.hpp>
#include <core/containers.hpp>
#include <core/heapobject.hpp>
#include <functional>

namespace eXl
{
  class EXL_ENGINE_API MenuManager
  {
    struct Impl;
  public:

    MenuManager();
    ~MenuManager();

    typedef void* MenuId;

    class Panel : public HeapObject
    {
    public:
      virtual ~Panel() {}

      virtual void Display() = 0;
    };

    typedef std::function<void()> Command;
    typedef std::function<Panel*()> PanelFactory;

    class EXL_ENGINE_API MenuBuilder
    {
    public:

      MenuBuilder& AddCommand(String const& iCommandName, Command&& iCommand);

      MenuBuilder& AddOpenPanelCommand(String const& iPanelIdentifier, PanelFactory&& iPanelFactory);

      MenuId EndMenu();

    private:

      MenuBuilder(MenuManager& iManager, String const& iName);

      friend MenuManager;
      MenuManager& m_Manager;
      String m_Name;
      Map<uint32_t, std::pair<String, Command>> m_Commands;
      Map<uint32_t, std::pair<String, PanelFactory>> m_Panels;
      uint32_t m_EntryCounter = 0;
    };

    MenuBuilder AddMenu(String const& iMenuName);
    void RemoveMenu(MenuId);

    void DisplayMenus();
    void DisplayPanels();

  private:

    MenuId CreateMenu(MenuBuilder& Builder);

    std::unique_ptr<Impl> m_Impl;
  };
}