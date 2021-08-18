/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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