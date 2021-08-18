/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <engine/common/object.hpp>
#include <engine/common/world.hpp>
#include <core/coredef.hpp>
#include <ogl/oglspritealgo.hpp>

namespace eXl
{
  class GfxComponent;
	class GfxSpriteComponent;
  class GfxSystem;
  class Transforms;
  class OGLTextureLoader;

  namespace DebugTool
  {
    class Drawer;
  }

  class EXL_ENGINE_API GfxSystem : public ComponentManager
  {
    DECLARE_RTTI(GfxSystem, ComponentManager);
  public:

    class Impl;

    enum Projection
    {
      Perspective,
      Orthographic
    };

    struct EXL_ENGINE_API ViewInfo
    {
      Vector2i viewportSize;
      Vector3f basis[3] = { Vector3f::UNIT_X, Vector3f::UNIT_Y, Vector3f::UNIT_Z};
      Vector3f pos;
      Vector4f backgroundColor = Vector4f::ZERO;
      Projection projection = Perspective;
      float fov = Mathf::PI / 2.0; // Vertical FOV

      // How much units of the world should we display in our viewport (vertically)
      // For ortho, it is exactly how much of the world we will see
      // For perspective, it is how much we will see at nearPlane distance.
      float displayedSize = 1.0;
    };

    static void StaticInit();

    GfxSystem(Transforms& iTransforms);

    void EnableDebugDraw();
    void DisableDebugDraw();

    DebugTool::Drawer* GetDebugDrawer();

    void SynchronizeTransforms();

    GfxComponent& CreateComponent(ObjectHandle iObject);
    GfxComponent* GetComponent(ObjectHandle iObject);

		GfxSpriteComponent& CreateSpriteComponent(ObjectHandle iObject);
		GfxSpriteComponent* GetSpriteComponent(ObjectHandle iObject);

    void DeleteComponent(ObjectHandle iObject) override;

    void SetView(ViewInfo const& iInfo);
    CameraMatrix const& GetCurrentCamera();

    Vector2i GetViewportSize() const;

    void ScreenToWorld(Vector2i const& iScreenPos, Vector3f& oWorldPos, Vector3f& oViewDir);

    Vector2i WorldToScreen(Vector3f const& iWorldPos);

    void RenderFrame(float iDelta);

		void SetSpriteDirty(GfxSpriteComponent& iSpriteComp);

  protected:

    Impl* m_Impl;
  };
}