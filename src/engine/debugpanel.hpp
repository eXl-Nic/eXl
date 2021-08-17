#pragma once 

#include <dunatk/common/world.hpp>
#include <dunatk/common/menumanager.hpp>

namespace eXl
{
  struct DebugPanel : public MenuManager::Panel
  {
    bool display[DebugTool::NumSystems];
    World& m_World;

    DebugPanel(World& iWorld)
      : m_World(iWorld)
    {
      for(auto& doDisp : display)
      {
        doDisp = true;
      }
    }

    void Display() override
    {
      ImGui::Begin("Debug Object");
      auto const& dbgInfo = DebugTool::GetDebugInfo();
      if(m_World.IsObjectValid(dbgInfo.m_DebugObj))
      {
        for(uint32_t i = 0; i<DebugTool::NumSystems; ++i)
        {
          ImGui::PushID(i);

          ImGui::Text("%s", DebugTool::GetSystemName((DebugTool::System)i));

          ImGui::PushID(0);
          bool doDisplay = display[i];
          ImGui::Checkbox("Display", &doDisplay);
          display[i] = doDisplay;
          ImGui::PopID();

          ImGui::PushID(1);
          if(ImGui::Button("Break"))
          {
            DebugTool::BreakIn((DebugTool::System)i);
          }
          ImGui::PopID();



          if(display[i])
          {
            ImGui::Separator();
            ImGui::Text("%s", dbgInfo.m_SysDbg[i].c_str());
          }
          ImGui::PopID();
        }

        if(ImGui::Button("Clear"))
        {
          DebugTool::SelectObject(ObjectHandle());
        }
      }
      else
      {
        ImGui::Text("No object selected");
      }
      ImGui::End();
    }
  };

  //DebugPanel s_DebugPanel;

  struct ImGuiLogState
  {
    static const uint32_t s_numEntries = 1024;

    ImGuiLogState()
    {
      m_Color[0] = Vector3f::ONE;
      m_Color[1] = Vector3f(0.7, 0.7, 0.0);
      m_Color[2] = Vector3f(0.7, 0.0, 0.0);
    }

    void write(const char* tolog, unsigned int num)
    {
      m_Entries[m_Begin] = tolog;
      m_LogLevel[m_Begin] = num;

      ++m_Begin;

      if(m_End != s_numEntries)
      {
        ++m_End;
      }

      if(m_Begin == s_numEntries)
      {
        m_Begin = 0;
      }
    }

    void Display()
    {
      for(uint32_t i = 0; i< m_End; ++i)
      {
        uint32_t curEntry = (m_Begin + i) % m_End;
        Vector3f const& color = m_Color[m_LogLevel[curEntry]];
        ImGui::TextColored(ImVec4(color.X(), color.Y(), color.Z(), 1.0), "%s", m_Entries[curEntry].c_str());
      }
    }

    String m_Entries[s_numEntries];
    uint32_t m_LogLevel[s_numEntries];
    Vector3f m_Color[3];
    uint32_t m_Begin = 0;
    uint32_t m_End = 0;

  };

  class ImGuiLogOutput : public Log_Manager::LogOutput
  {

  public:
    ImGuiLogOutput(ImGuiLogState& iState, unsigned int iNum)
      : m_State(iState)
      , m_Level(iNum)
    {

    }

    void write(const char* tolog)
    {
      m_State.write(tolog, m_Level);
    }

    ImGuiLogState& m_State;
    unsigned int m_Level;

  };
}


