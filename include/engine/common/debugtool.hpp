#pragma once

#include <engine/common/world.hpp>
#include <math/vector3.hpp>
#include <math/vector4.hpp>
#include <math/aabb2d.hpp>

#include <iostream>

namespace eXl
{
  namespace DebugTool
  {
    class Drawer
    {
    public:
      virtual void DrawLine(const Vector3f& iFrom, const Vector3f& iTo, const Vector4f& iColor, bool iScreenSpace = false) = 0;
      virtual void DrawBox(AABB2Df const& iBox, const Vector4f& iColor, bool iScreenSpace = false) = 0;
    };

    enum System
    {
      Transforms,
      Gfx,
      Physics,
      Nav_Pathfinding,
      Nav_Avoidance,

      NumSystems
    };

    struct Context
    {
      Context(std::stringstream& iStream, bool iDebug, System iSys)
        : m_PrintString(iStream)
        , m_DebugObj(iDebug)
        , m_System(iSys)
      {}

      ~Context();

      std::ostream& Print()
      {
        return m_PrintString;
      }

      explicit operator bool()
      {
        return m_DebugObj;
      }

    protected:
      std::stringstream& m_PrintString;
      bool m_DebugObj;
      System m_System;
    };

    struct DebugInfo
    {
      ObjectHandle m_DebugObj;
      std::string m_SysDbg[NumSystems];
    };

    Context BeginDebug(System iSys, ObjectHandle iHandle);

    void EndDebug(System iSys);

    EXL_ENGINE_API void SetDrawer(Drawer*);

    EXL_ENGINE_API const char* GetSystemName(System iSys);

    EXL_ENGINE_API void BreakIn(System iSys);

    EXL_ENGINE_API void SelectObject(ObjectHandle iHandle);

    EXL_ENGINE_API DebugInfo const& GetDebugInfo();

    EXL_ENGINE_API Drawer* GetDrawer();
  }
}