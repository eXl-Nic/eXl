/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/debugtool.hpp>

namespace eXl
{
  namespace DebugTool
  {
    namespace
    {
      class DummyDrawer : public Drawer
      {
      public:
        void DrawLine(const Vector3f& iFrom, const Vector3f& iTo, const Vector4f& iColor, bool iScreenSpace = false)override
        {}
        void DrawBox(AABB2Df const& iBox, const Vector4f& iColor, bool iScreenSpace = false) override
        {}
        void DrawConvex(Vector<Vector2f> const& iConvex, const Vector4f& iColor, bool iScreenSpace = false) override
        {}
      };

      struct DebugToolState
      {
        DebugToolState()
          : m_Drawer(&m_DummyDrawer)
        {
          for(auto& doBreak : m_BreakIn)
          {
            doBreak = false;
          }
        }

        bool m_BreakIn[NumSystems];
        ObjectHandle m_SelectedObject;

        DebugInfo m_CurInfo;
        std::stringstream m_OutStream;
        DummyDrawer m_DummyDrawer;
        Drawer* m_Drawer = nullptr;
      };
    }

    DebugToolState s_State;

    Context::~Context()
    {
      if(m_DebugObj)
      {
        EndDebug(m_System);
      }
    }

    const char* GetSystemName(System iSys)
    {
      static const char* s_SysNames[] = 
      {
        "Transforms",
        "Gfx",
        "Physics",
        "Navigation - Pathfinding",
        "Navigation - Obstacle Avoidance",
      };

      return s_SysNames[iSys];
    }

    void BreakIn(System iSys)
    {
      s_State.m_BreakIn[iSys] = true;
    }

    void SelectObject(ObjectHandle iHandle)
    {
      s_State.m_SelectedObject = iHandle;
      s_State.m_CurInfo.m_DebugObj = iHandle;
    }

    Context BeginDebug(System iSys, ObjectHandle iHandle)
    {
      s_State.m_OutStream = std::stringstream();
      if(iHandle == s_State.m_SelectedObject)
      {
        if(s_State.m_BreakIn[iSys])
        {
#ifndef __ANDROID__
          __debugbreak();
#endif
          s_State.m_BreakIn[iSys] = false;
        }
        
        return Context(s_State.m_OutStream, true, iSys);
      }

      return Context(s_State.m_OutStream, false, iSys);
    }

    void EndDebug(System iSys)
    {
      s_State.m_CurInfo.m_SysDbg[iSys] = s_State.m_OutStream.str();
    }

    DebugInfo const& GetDebugInfo()
    {
      return s_State.m_CurInfo;
    }

    void SetDrawer(Drawer* iDrawer)
    {
      if (iDrawer == nullptr)
      {
        s_State.m_Drawer = &s_State.m_DummyDrawer;
      }
      else
      {
        s_State.m_Drawer = iDrawer;
      }
      
    }

    Drawer* GetDrawer()
    {
      return s_State.m_Drawer;
    }
  }
}