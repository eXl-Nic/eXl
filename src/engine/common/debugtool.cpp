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