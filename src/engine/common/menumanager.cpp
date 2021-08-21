/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/menumanager.hpp>
#include <core/heapobject.hpp>

#ifndef DISABLE_IMGUI
#define IMGUI_API __declspec(dllimport)
#include <imgui.h>
#endif

namespace eXl
{
  struct CommandEntry
  {
    String m_Name;
    MenuManager::Command m_Command;
    bool m_IsPanel = false;;
  };

  struct MenuEntry : public HeapObject
  {
    String m_Name;
    Vector<CommandEntry> m_Commands;
    Map<String, std::unique_ptr<MenuManager::Panel>> m_OpenedPanels;
  };

  struct MenuManager::Impl : public HeapObject
  {
    Set<MenuEntry*> m_Menus;
  };

  MenuManager::MenuManager() 
    : m_Impl(eXl_NEW Impl)
  {

  }

  MenuManager::~MenuManager()
  {

  }

  MenuManager::MenuBuilder& MenuManager::MenuBuilder::AddOpenPanelCommand(String const& iPanelIdentifier, PanelFactory&& iPanelFactory)
  {
    m_Panels.insert(std::make_pair(m_EntryCounter++, std::make_pair(iPanelIdentifier, std::move(iPanelFactory))));
    return *this;
  }

  MenuManager::MenuBuilder& MenuManager::MenuBuilder::AddCommand(String const& iCommandName, Command&& iCommand)
  {
    m_Commands.insert(std::make_pair(m_EntryCounter++, std::make_pair(iCommandName, std::move(iCommand))));
    return *this;
  }

  MenuManager::MenuId MenuManager::MenuBuilder::EndMenu()
  {
    return m_Manager.CreateMenu(*this);
  }
  
  MenuManager::MenuBuilder::MenuBuilder(MenuManager& iManager, String const& iName)
    : m_Manager(iManager)
    , m_Name(iName)
  {

  }

  MenuManager::MenuBuilder MenuManager::AddMenu(String const& iMenuName)
  {
    return MenuBuilder(*this, iMenuName);
  }

  void MenuManager::RemoveMenu(MenuId iId)
  {
    auto menuIter = m_Impl->m_Menus.find((MenuEntry*)iId);
    if (menuIter != m_Impl->m_Menus.end())
    {
      eXl_DELETE *menuIter;
      m_Impl->m_Menus.erase(menuIter);
    }
  }

  void MenuManager::DisplayMenus()
  {
    for (auto menuEntry : m_Impl->m_Menus)
    {
#ifndef DISABLE_IMGUI
      if (ImGui::BeginMenu(menuEntry->m_Name.c_str()))
      {
        for (auto command : menuEntry->m_Commands)
        {
          if (command.m_IsPanel)
          {
            auto openedPanelIter = menuEntry->m_OpenedPanels.find(command.m_Name);
            bool panelOpened = openedPanelIter != menuEntry->m_OpenedPanels.end();
            bool checkBoxState = panelOpened;
            ImGui::Checkbox(command.m_Name.c_str(), &checkBoxState);
            if (panelOpened != checkBoxState)
            {
              if (checkBoxState)
              {
                command.m_Command();
              }
              else
              {
                menuEntry->m_OpenedPanels.erase(openedPanelIter);
              }
            }
          }
          else
          {
            if (ImGui::MenuItem(command.m_Name.c_str()))
            {
              command.m_Command();
            }
          }
        }
        ImGui::EndMenu();
      }
#endif
    }
  }

  void MenuManager::DisplayPanels()
  {
    for (auto menuEntry : m_Impl->m_Menus)
    {
#ifndef DISABLE_IMGUI
      Vector<Map<String, std::unique_ptr<Panel>>::iterator> toDelete;
      for (auto panelIter = menuEntry->m_OpenedPanels.begin(); panelIter != menuEntry->m_OpenedPanels.end(); ++panelIter)
      {
        bool open = true;
        ImGui::Begin(panelIter->first.c_str(), &open);
        panelIter->second->Display();
        ImGui::End();

        if (!open)
        {
          toDelete.push_back(panelIter);
        }
      }
      for (auto panel : toDelete)
      {
        menuEntry->m_OpenedPanels.erase(panel);
      }
#endif
    }
  }

  MenuManager::MenuId MenuManager::CreateMenu(MenuBuilder& iBuilder)
  {
    MenuEntry* newMenu = eXl_NEW MenuEntry;
    newMenu->m_Name = std::move(iBuilder.m_Name);
    for (uint32_t i = 0; i < iBuilder.m_EntryCounter; ++i)
    {
      auto CommandIter = iBuilder.m_Commands.find(i);
      if (CommandIter != iBuilder.m_Commands.end())
      {
        CommandEntry Entry;
        Entry.m_Name = std::move(CommandIter->second.first);
        Entry.m_Command = std::move(CommandIter->second.second);
        newMenu->m_Commands.emplace_back(std::move(Entry));
      }

      auto PanelIter = iBuilder.m_Panels.find(i);
      if (PanelIter != iBuilder.m_Panels.end())
      {
        CommandEntry Entry;
        Entry.m_IsPanel = true;
        Entry.m_Name = std::move(PanelIter->second.first);
        Entry.m_Command = [newMenu, panelName = Entry.m_Name, factory = std::move(PanelIter->second.second)]()
        {
          newMenu->m_OpenedPanels.insert(std::make_pair(panelName, factory()));
        };
        newMenu->m_Commands.emplace_back(std::move(Entry));
      }
    }
    m_Impl->m_Menus.insert(newMenu);

    return newMenu;
  }
}