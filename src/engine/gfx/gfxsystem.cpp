/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>

#include <engine/gfx/bounds3d.hpp>

#include "gfxdebugdrawer.hpp"
#include "gfxcomponentrendernode.hpp"
#include "gfxspriterendernode.hpp"

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>

#include <ogl/oglutils.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>
#include <core/type/tagtype.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(GfxSystem);
  IMPLEMENT_RTTI(GfxRenderNode);

  void GfxSystem::ViewInfo::ComputeFrustumPlanes(Plane(&oPlanes)[6]) const
  {
    float hwRatio = float(viewportSize.y) / viewportSize.x;

    if (projection == Perspective)
    {
      float nearPlane = displayedSize * 0.5 / tan(fov * 0.5);
      float farPlane = displayedSize * 1000;

      float halfWidth = tan(fov * 0.5) * nearPlane;

      Vec3 nearPlaneCenter = pos - basis[2] * nearPlane;
      Vec3 farPlaneCenter = pos - basis[2] * farPlane;

      Vec3 nearPlaneRect[4] =
      {
        normalize(Vec3(-halfWidth, -halfWidth * hwRatio, nearPlane)),
        normalize(Vec3(-halfWidth,  halfWidth * hwRatio, nearPlane)),
        normalize(Vec3(halfWidth,  halfWidth * hwRatio, nearPlane)),
        normalize(Vec3(halfWidth, -halfWidth * hwRatio, nearPlane)),
      };

      for (auto& pt : nearPlaneRect)
      {
        pt = basis[0] * pt.x
          + basis[1] * pt.y
          + basis[2] * pt.z;
      }

      oPlanes[0] = Plane::FromPointAndNormal(cross(nearPlaneRect[1], nearPlaneRect[0]), pos);
      oPlanes[1] = Plane::FromPointAndNormal(cross(nearPlaneRect[3], nearPlaneRect[2]), pos);
      oPlanes[2] = Plane::FromPointAndNormal(cross(nearPlaneRect[0], nearPlaneRect[3]), pos);
      oPlanes[3] = Plane::FromPointAndNormal(cross(nearPlaneRect[2], nearPlaneRect[1]), pos);
      oPlanes[4] = Plane::FromPointAndNormal( basis[2], nearPlaneCenter);
      oPlanes[5] = Plane::FromPointAndNormal(-basis[2], farPlaneCenter);
    }
    else
    {
      float nearPlane = 0.0001;
      float farPlane = displayedSize * 100.0;

      Vec3 nearPlaneCenter = pos - basis[2] * nearPlane;
      Vec3 farPlaneCenter = pos - basis[2] * farPlane;

      Vec3 clipSpaceCenter = (nearPlaneCenter + farPlaneCenter) * 0.5f;

      oPlanes[0] = Plane::FromPointAndNormal( basis[0], clipSpaceCenter + displayedSize * 0.5 * basis[0]);
      oPlanes[1] = Plane::FromPointAndNormal(-basis[0], clipSpaceCenter - displayedSize * 0.5 * basis[0]);
      oPlanes[2] = Plane::FromPointAndNormal( basis[1], clipSpaceCenter + displayedSize * 0.5 * hwRatio * basis[1]);
      oPlanes[3] = Plane::FromPointAndNormal(-basis[1], clipSpaceCenter - displayedSize * 0.5 * hwRatio * basis[1]);
      oPlanes[4] = Plane::FromPointAndNormal( basis[2], nearPlaneCenter);
      oPlanes[5] = Plane::FromPointAndNormal(-basis[2], farPlaneCenter);
    }
  }

  void GfxSystem::ViewInfo::GetFurstumCornerDir(Vec3(&oDirs)[4]) const
  {
    float hwRatio = float(viewportSize.y) / viewportSize.x;

    if (projection == Perspective)
    {
      float nearPlane = displayedSize * 0.5 / tan(fov * 0.5);
      float farPlane = displayedSize * 1000;

      float halfWidth = tan(fov * 0.5) * nearPlane;

      oDirs[0] = normalize(Vec3(-halfWidth, -halfWidth * hwRatio, nearPlane));
      oDirs[1] = normalize(Vec3(-halfWidth,  halfWidth * hwRatio, nearPlane));
      oDirs[2] = normalize(Vec3( halfWidth,  halfWidth * hwRatio, nearPlane));
      oDirs[3] = normalize(Vec3( halfWidth, -halfWidth * hwRatio, nearPlane));

      for (auto& pt : oDirs)
      {
        pt = basis[0] * pt.x
          + basis[1] * pt.y
          + basis[2] * pt.z;
      }
    }
    else
    {
      oDirs[0] = -basis[2];
      oDirs[1] = oDirs[2] = oDirs[3];
    }
  }

  void GfxSystem::ViewInfo::DrawFrustum(DebugTool::Drawer& iDrawer) const
  {
    float hwRatio = float(viewportSize.y) / viewportSize.x;
    if (projection == Perspective)
    {
      float nearPlane = displayedSize * 0.5 / tan(fov * 0.5);
      float farPlane = displayedSize * 1000;

      float halfWidth = tan(fov * 0.5) * nearPlane;

      Vec3 nearPlaneCenter = pos - basis[2] * nearPlane;
      Vec3 farPlaneCenter = pos - basis[2] * farPlane;

      Vec3 nearPlaneRect[4] =
      {
        Vec3(-halfWidth, -halfWidth * hwRatio, -nearPlane),
        Vec3(-halfWidth,  halfWidth * hwRatio, -nearPlane),
        Vec3(halfWidth,  halfWidth * hwRatio, -nearPlane),
        Vec3(halfWidth, -halfWidth * hwRatio, -nearPlane),
      };

      Vec3 farPlaneRect[4] =
      {
        (nearPlaneRect[0] / nearPlane) * farPlane,
        (nearPlaneRect[1] / nearPlane) * farPlane,
        (nearPlaneRect[2] / nearPlane) * farPlane,
        (nearPlaneRect[3] / nearPlane) * farPlane,
      };

      for (auto& pt : nearPlaneRect)
      {
        pt = basis[0] * pt.x
          + basis[1] * pt.y
          + basis[2] * pt.z;
      }

      for (auto& pt : farPlaneRect)
      {
        pt = basis[0] * pt.x
          + basis[1] * pt.y
          + basis[2] * pt.z;
      }

      for (uint32_t i = 0; i < 4; ++i)
      {
        iDrawer.DrawLine(pos + nearPlaneRect[i], pos + farPlaneRect[i], Vec4(0, 1, 0, 1));
      }

      for (uint32_t i = 0; i < 4; ++i)
      {
        iDrawer.DrawLine(pos + nearPlaneRect[i], pos + nearPlaneRect[(i + 1) % 4], Vec4(0, 1, 0, 1));
        iDrawer.DrawLine(pos + farPlaneRect[i], pos + farPlaneRect[(i + 1) % 4], Vec4(0, 1, 0, 1));
      }

    }
    else
    {
    }
  }

  class GfxSystem::Impl
  {
  public:
    Impl(Transforms& iTrans)
      : m_Transforms(iTrans)
    {
      
    }

    Vec2i m_ViewportSize;
    float m_NearP;
    float m_FarP;
    Vec4 m_ClearColor;
    float m_ClearDepth;
    CameraMatrix m_Camera;

    GfxDebugDrawer* m_DebugDrawer;
    GfxComponentRenderNode* m_ComponentsNode;
    GfxSpriteRenderNode* m_SpriteNode;

    IntrusivePtr<OGLBuffer> m_CameraBuffer;

    Transforms& m_Transforms;

    OGLSemanticManager m_Semantics;
    struct RenderNodeEntry
    {
      UniquePtr<GfxRenderNode> m_Node;
      GfxRenderNode::TransformUpdateCallback m_OnTransform;
      GfxRenderNode::UpdateCallback m_OnDelete;
      Vector<ObjectHandle> m_UpdateArray;
      Vector<Mat4 const*> m_TransUpdateArray;
    };

    using RenderNodes = ObjectTable<RenderNodeEntry>;
    using RenderNodeHandle = RenderNodes::Handle;
    RenderNodes m_Nodes;

    RenderNodeHandle m_DebugDrawerHandle;
    RenderNodeHandle m_SpriteHandle;

    Vector<RenderNodeHandle> m_ObjectToNode;
  };

  OGLSemanticManager& GfxSystem::GetSemanticManager()
  {
    return m_Impl->m_Semantics;
  }

  void GfxSystem::ScreenToWorld(Vec2i const& iScreenPos, Vec3& oWorldPos, Vec3& oViewDir)
  {
    Vec2 screenSpacePos(( 2.0 * float(iScreenPos.x) / m_Impl->m_ViewportSize.x - 1.0), 
      ((1.0 - 2.0 * float(iScreenPos.y) / m_Impl->m_ViewportSize.y)));
    Vec4 screenPos(screenSpacePos.x, screenSpacePos.y, m_Impl->m_NearP, 1.0);
    Vec4 screenPosF(screenSpacePos.x, screenSpacePos.y, m_Impl->m_NearP + (m_Impl->m_FarP - m_Impl->m_NearP) * 0.1, 1.0);

    Mat4 invMat = inverse(m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix);

    Vec4 worldPt = invMat * screenPos;
    Vec4 worldPtF = invMat * screenPosF;

    oWorldPos = worldPt;
    oViewDir = normalize(Vec3(worldPtF) - oWorldPos);
  }

  Vec2i GfxSystem::WorldToScreen(Vec3 const& iWorldPos)
  {
    Vec4 worldPos(iWorldPos.x, iWorldPos.y, iWorldPos.z, 1.0);

    Vec4 projectedPt = m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix * worldPos;

    return Vec2i(((projectedPt.x * 0.5) + 0.5) * m_Impl->m_ViewportSize.x
      , (0.5 - ((projectedPt.y * 0.5))) * m_Impl->m_ViewportSize.y);
  }

  void GfxSystem::StaticInit()
  {
    static bool s_StaticInitDone = false;
    if (!s_StaticInitDone)
    {
      OGLUtils::Init();
      OGLProgramInterface::InitStaticData();

			s_StaticInitDone = true;
    }
  }

  GfxSystem::GfxSystem(Transforms& iTransforms)
    : m_Impl(new Impl(iTransforms))
  {
    OGLBaseAlgo::Init(m_Impl->m_Semantics);
    m_Impl->m_DebugDrawer = eXl_NEW GfxDebugDrawer;
    m_Impl->m_DebugDrawerHandle = Impl::RenderNodeHandle(AddRenderNode(UniquePtr<GfxRenderNode>(m_Impl->m_DebugDrawer)));

    m_Impl->m_ComponentsNode = eXl_NEW GfxComponentRenderNode;
    AddRenderNode(UniquePtr<GfxRenderNode>(m_Impl->m_ComponentsNode));

    m_Impl->m_SpriteNode = eXl_NEW GfxSpriteRenderNode;
    m_Impl->m_SpriteHandle = Impl::RenderNodeHandle(AddRenderNode(UniquePtr<GfxRenderNode>(m_Impl->m_SpriteNode)));
  }

  GfxRenderNodeHandle GfxSystem::GetDebugDrawerHandle()
  {
    return m_Impl->m_DebugDrawerHandle;
  }

  GfxRenderNodeHandle GfxSystem::GetSpriteHandle()
  {
    return m_Impl->m_SpriteHandle;
  }

  OGLCompiledProgram const* GfxSystem::GetSpriteProgram()
  {
    return m_Impl->m_SpriteNode->GetSpriteProgram();
  }

  OGLCompiledProgram const* GfxSystem::GetLineProgram()
  {
    return m_Impl->m_DebugDrawer->GetLineProgram();
  }

  GfxSystem::~GfxSystem() = default;

  Transforms& GfxSystem::GetTransforms()
  {
    return m_Impl->m_Transforms;
  }

  DebugTool::Drawer* GfxSystem::GetDebugDrawer()
  {
    return m_Impl->m_DebugDrawer;
  }

  void GfxSystem::EnableDebugDraw()
  {
    DebugTool::SetDrawer(m_Impl->m_DebugDrawer);
  }

  void GfxSystem::DisableDebugDraw()
  {
    DebugTool::SetDrawer(nullptr);
  }

  GfxComponent& GfxSystem::CreateComponent(ObjectHandle iObject)
  {
    m_Impl->m_ComponentsNode->AddObject(iObject);
    return *m_Impl->m_ComponentsNode->GetComponent(iObject);
  }

  GfxComponent* GfxSystem::GetComponent(ObjectHandle iObject)
  {
    if (GetWorld().IsObjectValid(iObject))
    {
      return m_Impl->m_ComponentsNode->GetComponent(iObject);
    }
    return nullptr;
  }

	void GfxSystem::CreateSpriteComponent(ObjectHandle iObject)
	{
		eXl_ASSERT(GetWorld().IsObjectValid(iObject));
    m_Impl->m_SpriteNode->AddObject(iObject);
	}

	//GfxSpriteComponent* GfxSystem::GetSpriteComponent(ObjectHandle iObject)
	//{
  //  if (GetWorld().IsObjectValid(iObject))
  //  {
  //    return m_Impl->m_SpriteNode->GetComponent(iObject);
  //  }
  //  return nullptr;
	//}

  void GfxSystem::DeleteComponent(ObjectHandle iObject)
  {
    if(m_Impl->m_ObjectToNode.size() > iObject.GetId()
      && m_Impl->m_ObjectToNode[iObject.GetId()].IsAssigned())
    {
      auto const& nodeEntry = m_Impl->m_Nodes.Get(m_Impl->m_ObjectToNode[iObject.GetId()]);
      if (nodeEntry.m_OnDelete)
      {
        nodeEntry.m_OnDelete(&iObject, 1);
      }
      m_Impl->m_ObjectToNode[iObject.GetId()] = Impl::RenderNodeHandle();
    }

		ComponentManager::DeleteComponent(iObject);
  }

  Vec2i GfxSystem::GetViewportSize() const
  {
    return m_Impl->m_ViewportSize;
  }

  void GfxSystem::SynchronizeTransforms()
  {
    m_Impl->m_Transforms.IterateOverDirtyTransforms([this](Mat4 const& iMat, ObjectHandle iObj)
    {
      if (m_Impl->m_ObjectToNode.size() > iObj.GetId()
        && m_Impl->m_ObjectToNode[iObj.GetId()].IsAssigned())
      {
        auto& nodeEntry = m_Impl->m_Nodes.Get(m_Impl->m_ObjectToNode[iObj.GetId()]);
        if (nodeEntry.m_OnTransform)
        {
          nodeEntry.m_UpdateArray.push_back(iObj);
          nodeEntry.m_TransUpdateArray.push_back(&iMat);
        }
      }
    });

    m_Impl->m_Nodes.Iterate([](Impl::RenderNodeHandle, Impl::RenderNodeEntry& iNode)
      {
        if (!iNode.m_UpdateArray.empty())
        {
          iNode.m_OnTransform(iNode.m_UpdateArray.data(), iNode.m_TransUpdateArray.data(), iNode.m_UpdateArray.size());
          iNode.m_UpdateArray.clear();
          iNode.m_TransUpdateArray.clear();
        }
      });
  }

  CameraMatrix const& GfxSystem::GetCurrentCamera()
  {
    return m_Impl->m_Camera;
  }

  void GfxSystem::SetView(ViewInfo const& iInfo)
  {
    m_Impl->m_ViewportSize = iInfo.viewportSize;
    m_Impl->m_Camera.projMatrix = Zero<Mat4>();
    m_Impl->m_Camera.viewInverseMatrix = Identity<Mat4>();

    float screenRatio = float(m_Impl->m_ViewportSize.x) / float(m_Impl->m_ViewportSize.y);
    bool ortho = iInfo.projection == Orthographic;

    if (ortho)
    {
      m_Impl->m_NearP = 0.0001;
      m_Impl->m_FarP = iInfo.displayedSize * 100.0;

      m_Impl->m_Camera.projMatrix[0][0] = 2.0 / (iInfo.displayedSize * screenRatio);
      m_Impl->m_Camera.projMatrix[1][1] = 2.0 / iInfo.displayedSize;
      m_Impl->m_Camera.projMatrix[2][2] = -2.0 / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix[3][2] = -(m_Impl->m_FarP + m_Impl->m_NearP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix[3][3] = 1.0;
    }
    else
    {
      m_Impl->m_NearP = iInfo.displayedSize * 0.5 / tan(iInfo.fov * 0.5);
      m_Impl->m_FarP = iInfo.displayedSize * 1000;

      m_Impl->m_Camera.projMatrix[0][0] = 2.0 * m_Impl->m_NearP / screenRatio;
      m_Impl->m_Camera.projMatrix[1][1] = 2.0 * m_Impl->m_NearP;
      m_Impl->m_Camera.projMatrix[2][2] = -1.0* (m_Impl->m_NearP + m_Impl->m_FarP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix[3][2] = -2.0 * m_Impl->m_NearP * m_Impl->m_FarP / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix[2][3] = -1.0;
    }

    Vec3 basisX = iInfo.basis[0];
    Vec3 basisY = iInfo.basis[1];
    Vec3 basisZ = iInfo.basis[2];

    m_Impl->m_Camera.viewInverseMatrix[0] = Vec4(basisX, 0);
    m_Impl->m_Camera.viewInverseMatrix[1] = Vec4(basisY, 0);
    m_Impl->m_Camera.viewInverseMatrix[2] = Vec4(basisZ, 0);
    m_Impl->m_Camera.viewInverseMatrix[3] = Vec4(iInfo.pos, 1);

    m_Impl->m_Camera.viewMatrix = inverse(m_Impl->m_Camera.viewInverseMatrix);

    // Will work for ortho, but not persp.
    m_Impl->m_DebugDrawer->m_CurrentScreenSize = iInfo.displayedSize;

    m_Impl->m_ClearColor = iInfo.backgroundColor;
    //m_Impl->m_ClearDepth = 1.0;
  }

  void GfxSystem::RenderFrame(float iDelta)
  {
    if (!m_Impl->m_CameraBuffer)
    {
      m_Impl->m_CameraBuffer = OGLBuffer::CreateBuffer(OGLBufferUsage::UNIFORM_BUFFER, CameraMatrix::GetType()->GetSize(), nullptr);
    }

    m_Impl->m_Nodes.Iterate([&](Impl::RenderNodeHandle iNodeHandle, Impl::RenderNodeEntry& iNode)
      {
        if (iNode.m_Node->m_Sys == nullptr)
        {
          iNode.m_Node->Init(*this, iNodeHandle);
        }
      });

    OGLDisplayList list(GetSemanticManager());

    list.SetDefaultViewport(Zero<Vec2i>(), m_Impl->m_ViewportSize);
    list.SetDefaultDepth(true, true);
    list.SetDefaultScissor(Vec2i(0,0),Vec2i(-1,-1));
    list.SetDefaultBlend(true, OGLBlend::SRC_ALPHA, OGLBlend::ONE_MINUS_SRC_ALPHA);

    list.InitForPush();

    list.Clear(0,true,true, m_Impl->m_ClearColor);

    OGLShaderData camData;
    //camData.AddData(OGLBaseAlgo::GetCameraUniform(), &m_Impl->m_Camera);
    m_Impl->m_CameraBuffer->SetData(0, sizeof(m_Impl->m_Camera), &m_Impl->m_Camera);
    camData.SetDataBuffer(OGLBaseAlgo::GetCameraUniform(), m_Impl->m_CameraBuffer);

    list.PushData(&camData);

    List<GfxComponent*> toDelete;

    m_Impl->m_Nodes.Iterate([&](Impl::RenderNodeHandle iHandle, Impl::RenderNodeEntry& iNode)
    {
        if (iHandle == m_Impl->m_DebugDrawerHandle)
        {
          return;
        }
        iNode.m_Node->Push(list, iDelta);
    });

    m_Impl->m_Nodes.Get(m_Impl->m_DebugDrawerHandle).m_Node->Push(list, iDelta);

    OGLRenderContext renderContext(GetSemanticManager());
    list.Render(&renderContext);
  }

  GfxRenderNodeHandle GfxSystem::AddRenderNode(UniquePtr<GfxRenderNode> iNode)
  {
    if (!iNode)
    {
      return GfxRenderNodeHandle();
    }

    Impl::RenderNodeHandle nodeHandle = m_Impl->m_Nodes.Alloc();
    auto& entry = m_Impl->m_Nodes.Get(nodeHandle);

    entry.m_Node = std::move(iNode);
    entry.m_OnTransform = entry.m_Node->GetTransformUpdateCallback();
    entry.m_OnDelete = entry.m_Node->GetDeleteCallback();

    return nodeHandle;
  }

  GfxRenderNode* GfxSystem::GetRenderNode(GfxRenderNodeHandle iNodeHandle)
  {
    Impl::RenderNodeEntry* entry = m_Impl->m_Nodes.TryGet(Impl::RenderNodeHandle(iNodeHandle));
    if (entry != nullptr)
    {
      return entry->m_Node.get();
    }

    return nullptr;
  }

  void GfxRenderNode::AddObject(ObjectHandle iObject)
  {
    eXl_ASSERT_REPAIR_RET(m_Sys != nullptr, void()); 
    eXl_ASSERT_REPAIR_RET(m_Sys->GetWorld().IsObjectValid(iObject), void());

    auto& nodeAssocArray = m_Sys->GetImpl().m_ObjectToNode;
    while (nodeAssocArray.size() <= iObject.GetId())
    {
      nodeAssocArray.push_back(GfxSystem::Impl::RenderNodeHandle());
    }
    GfxSystem::Impl::RenderNodeHandle& entry = nodeAssocArray[iObject.GetId()];
    eXl_ASSERT_REPAIR_RET(!entry.IsAssigned() || entry == m_NodeHandle, void());
    
    entry = GfxSystem::Impl::RenderNodeHandle(m_NodeHandle);
    m_Sys->ComponentManager::CreateComponent(iObject);
  }
}