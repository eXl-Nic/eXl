#pragma once

#include <core/base/coredef.hpp>
#include <core/lua/luamanager.hpp>

#include <imgui.h>

namespace eXl
{
  struct LuaConsole
  {
    char             InputBuf[256];
    Vector<AString>  Items;
    bool             ScrollToBottom;
    Vector<AString>  History;
    int              HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    LuaWorld         LuaCtx;

    LuaConsole();

    ~LuaConsole();
  
    void ClearLog();

    void AddLog(const char* fmt, ...) IM_FMTARGS(2);

    void Draw(/*const char* title, bool* p_open*/);

    void ExecCommand(const char* command_line);

    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

    int TextEditCallback(ImGuiInputTextCallbackData* data);
  };
}