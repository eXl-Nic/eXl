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
  class GfxSystem;
  class Transforms;
  class OGLDisplayList;

  namespace DebugTool
  {
    class Drawer;
  }

  using GfxRenderNodeHandle = ObjectTableHandle_Base;

  class GfxRenderNode : public RttiObject
  {
    DECLARE_RTTI(GfxRenderNode, RttiObject);
  public:
    bool IsInitialized() const { return m_Sys != nullptr; }
    World& GetWorld() const;
  protected:
    friend GfxSystem;
    using UpdateCallback = std::function<void(ObjectHandle const*, size_t)>;
    using TransformUpdateCallback = std::function<void(ObjectHandle const*, Mat4 const**, size_t)>;

    virtual void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
    {
      m_Sys = &iSys;
      m_NodeHandle = iHandle;
    }

    virtual TransformUpdateCallback GetTransformUpdateCallback()
    {
      return TransformUpdateCallback();
    }

    virtual UpdateCallback GetDeleteCallback()
    {
      return UpdateCallback();
    }

    virtual void Push(OGLDisplayList& iList, float iDelta) = 0;

    void AddObject(ObjectHandle iObject);

    GfxSystem* m_Sys = nullptr;
    GfxRenderNodeHandle m_NodeHandle;
  };

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
      Vec2i viewportSize;
      Vec3 basis[3] = { UnitX<Vec3>(), UnitY<Vec3>(), UnitZ<Vec3>()};
      Vec3 pos = Zero<Vec3>();
      Vec4 backgroundColor = Zero<Vec4>();
      Projection projection = Perspective;
      float fov = Mathf::Pi() / 2.0; // Vertical FOV

      // How much units of the world should we display in our viewport (vertically)
      // For ortho, it is exactly how much of the world we will see
      // For perspective, it is how much we will see at nearPlane distance.
      float displayedSize = 1.0;
    };

    static void StaticInit();

    GfxSystem(Transforms& iTransforms);
    ~GfxSystem();

    void EnableDebugDraw();
    void DisableDebugDraw();

    DebugTool::Drawer* GetDebugDrawer();

    OGLSemanticManager& GetSemanticManager();

    GfxRenderNodeHandle GetDebugDrawerHandle();
    GfxRenderNodeHandle GetSpriteHandle();

    OGLCompiledProgram const* GetSpriteProgram();
    OGLCompiledProgram const* GetLineProgram();

    GfxRenderNodeHandle AddRenderNode(UniquePtr<GfxRenderNode> iNode);
    GfxRenderNode* GetRenderNode(GfxRenderNodeHandle iNodeHandle);

    void SynchronizeTransforms();

    GfxComponent& CreateComponent(ObjectHandle iObject);
    GfxComponent* GetComponent(ObjectHandle iObject);

		void CreateSpriteComponent(ObjectHandle iObject);

    void DeleteComponent(ObjectHandle iObject) override;

    void SetView(ViewInfo const& iInfo);
    CameraMatrix const& GetCurrentCamera();

    Vec2i GetViewportSize() const;

    void ScreenToWorld(Vec2i const& iScreenPos, Vec3& oWorldPos, Vec3& oViewDir);

    Vec2i WorldToScreen(Vec3 const& iWorldPos);

    void RenderFrame(float iDelta);

    Impl& GetImpl() { return *m_Impl; }
    Transforms& GetTransforms();
    using WorldSystem::GetWorld;

  protected:
    friend GfxRenderNode;
    UniquePtr<Impl> m_Impl;
  };

  inline World& GfxRenderNode::GetWorld() const { return m_Sys->GetWorld(); }
}
