/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/bounds3d.hpp>

namespace eXl
{
  class Model;
  class Image;
  
  class EXL_ENGINE_API Gfx3DScene
  {
  public:

    void Initialize(GfxSystem& iSys);

    void Add3DModel(ObjectHandle iObj, IntrusivePtr<Model> iModel);
    void Hide3DModel(ObjectHandle iObj, bool iHide);

    void SetSkybox(Vector<Image*> const& iPlanes);

  protected:
    GfxSystem* m_Sys = nullptr;
    GfxRenderNodeHandle m_Handle;
  };

  class EXL_ENGINE_API OrbitCamera
  {
  public:
    OrbitCamera();
    OrbitCamera(GfxSystem::ViewInfo& ioInfo, Vec3 const& iPos, Vec3 const& iFocus, Vec3 iUpDir);

    // Moves camera alongside its local axis.
    void Update(GfxSystem::ViewInfo& ioInfo, Vec3 const& iLocalMovement);

    // Set the camera's focus on the given object
    void Reframe(GfxSystem::ViewInfo& ioInfo, BoundingSphere const& iSph);

    Vec3 const& GetFocus() const { return m_Focus; }

  protected:
    Vec3 m_Focus;
    Vec3 m_UpDir;
  };
}