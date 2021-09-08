/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
      virtual void DrawConvex(Vector<Vector2f> const& iConvex, const Vector4f& iColor, bool iScreenSpace = false) = 0;
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