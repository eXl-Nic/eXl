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
