/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
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

  class GfxSystem::Impl
  {
  public:
    Impl(Transforms& iTrans)
      : m_Transforms(iTrans)
    {
      
    }

    Vector2i m_ViewportSize;
    float m_NearP;
    float m_FarP;
    Vector4f m_ClearColor;
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
      Vector<Matrix4f const*> m_TransUpdateArray;
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

  void GfxSystem::ScreenToWorld(Vector2i const& iScreenPos, Vector3f& oWorldPos, Vector3f& oViewDir)
  {
    Vector2f screenSpacePos(( 2.0 * float(iScreenPos.X()) / m_Impl->m_ViewportSize.X() - 1.0), 
      ((1.0 - 2.0 * float(iScreenPos.Y()) / m_Impl->m_ViewportSize.Y())));
    Vector4f screenPos(screenSpacePos.X(), screenSpacePos.Y(), m_Impl->m_NearP, 1.0);
    Vector4f screenPosF(screenSpacePos.X(), screenSpacePos.Y(), m_Impl->m_NearP + (m_Impl->m_FarP - m_Impl->m_NearP) * 0.1, 1.0);

    Matrix4f invMat = m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix;
    invMat = invMat.Inverse();

    Vector4f worldPt = invMat * screenPos;
    Vector4f worldPtF = invMat * screenPosF;

    oWorldPos = reinterpret_cast<Vector3f&>(worldPt);
    oViewDir = reinterpret_cast<Vector3f&>(worldPtF) - oWorldPos;
    oViewDir.Normalize();
  }

  Vector2i GfxSystem::WorldToScreen(Vector3f const& iWorldPos)
  {
    Vector4f worldPos(iWorldPos.X(), iWorldPos.Y(), iWorldPos.Z(), 1.0);

    Vector4f projectedPt = m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix * worldPos;

    return Vector2i(((projectedPt.X() * 0.5) + 0.5) * m_Impl->m_ViewportSize.X()
      , (0.5 - ((projectedPt.Y() * 0.5))) * m_Impl->m_ViewportSize.Y());
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

	GfxSpriteComponent& GfxSystem::CreateSpriteComponent(ObjectHandle iObject)
	{
		eXl_ASSERT(GetWorld().IsObjectValid(iObject));
    m_Impl->m_SpriteNode->AddObject(iObject);
		return *m_Impl->m_SpriteNode->GetComponent(iObject);
	}

	GfxSpriteComponent* GfxSystem::GetSpriteComponent(ObjectHandle iObject)
	{
    if (GetWorld().IsObjectValid(iObject))
    {
      return m_Impl->m_SpriteNode->GetComponent(iObject);
    }
    return nullptr;
	}

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

  Vector2i GfxSystem::GetViewportSize() const
  {
    return m_Impl->m_ViewportSize;
  }

  void GfxSystem::SynchronizeTransforms()
  {
    m_Impl->m_Transforms.IterateOverDirtyTransforms([this](Matrix4f const& iMat, ObjectHandle iObj)
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

    m_Impl->m_Nodes.Iterate([](Impl::RenderNodeEntry& iNode, Impl::RenderNodeHandle)
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
    m_Impl->m_Camera.projMatrix.MakeZero();
    m_Impl->m_Camera.viewInverseMatrix.MakeIdentity();

    float screenRatio = float(m_Impl->m_ViewportSize.X()) / float(m_Impl->m_ViewportSize.Y());
    bool ortho = iInfo.projection == Orthographic;

    if (ortho)
    {
      m_Impl->m_NearP = 0.0001;
      m_Impl->m_FarP = iInfo.displayedSize * 100.0;

      m_Impl->m_Camera.projMatrix.m_Data[0] = 2.0 / (iInfo.displayedSize * screenRatio);
      m_Impl->m_Camera.projMatrix.m_Data[5] = 2.0 / iInfo.displayedSize;
      m_Impl->m_Camera.projMatrix.m_Data[10] = -2.0 / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[14] = -(m_Impl->m_FarP + m_Impl->m_NearP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[15] = 1.0;
    }
    else
    {
      m_Impl->m_NearP = iInfo.displayedSize * 0.5 / tan(iInfo.fov * 0.5);
      m_Impl->m_FarP = iInfo.displayedSize * 1000;

      m_Impl->m_Camera.projMatrix.m_Data[0] = 2.0 * m_Impl->m_NearP / screenRatio;
      m_Impl->m_Camera.projMatrix.m_Data[5] = 2.0 * m_Impl->m_NearP;
      m_Impl->m_Camera.projMatrix.m_Data[10] = -1.0* (m_Impl->m_NearP + m_Impl->m_FarP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[14] = -2.0 * m_Impl->m_NearP * m_Impl->m_FarP / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[11] = -1.0;
    }

    Vector3f basisX = iInfo.basis[0];
    Vector3f basisY = iInfo.basis[1];
    Vector3f basisZ = iInfo.basis[2];

    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 0, &basisX, sizeof(Vector3f));
    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 4, &basisY, sizeof(Vector3f));
    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 8, &basisZ, sizeof(Vector3f));

    //Vector3f transPos = basisX * (-iInfo.pos.X()) + basisY * (-iInfo.pos.Y()) + basisZ * (-iInfo.pos.Z());
    m_Impl->m_Camera.viewInverseMatrix.m_Data[12] = iInfo.pos.X();
    m_Impl->m_Camera.viewInverseMatrix.m_Data[13] = iInfo.pos.Y();
    m_Impl->m_Camera.viewInverseMatrix.m_Data[14] = iInfo.pos.Z();

    m_Impl->m_Camera.viewMatrix = m_Impl->m_Camera.viewInverseMatrix.Inverse();

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

    m_Impl->m_Nodes.Iterate([&](Impl::RenderNodeEntry& iNode, Impl::RenderNodeHandle iNodeHandle)
      {
        if (iNode.m_Node->m_Sys == nullptr)
        {
          iNode.m_Node->Init(*this, iNodeHandle);
        }
      });

    OGLDisplayList list(GetSemanticManager());

    list.SetDefaultViewport(Vector2i::ZERO, m_Impl->m_ViewportSize);
    list.SetDefaultDepth(true, true);
    list.SetDefaultScissor(Vector2i(0,0),Vector2i(-1,-1));
    list.SetDefaultBlend(true, OGLBlend::SRC_ALPHA, OGLBlend::ONE_MINUS_SRC_ALPHA);

    list.InitForPush();

    list.Clear(0,true,true, m_Impl->m_ClearColor);

    OGLShaderData camData;
    //camData.AddData(OGLBaseAlgo::GetCameraUniform(), &m_Impl->m_Camera);
    m_Impl->m_CameraBuffer->SetData(0, sizeof(m_Impl->m_Camera), &m_Impl->m_Camera);
    camData.SetDataBuffer(OGLBaseAlgo::GetCameraUniform(), m_Impl->m_CameraBuffer);

    list.PushData(&camData);

    List<GfxComponent*> toDelete;

    m_Impl->m_Nodes.Iterate([&](Impl::RenderNodeEntry& iNode, Impl::RenderNodeHandle)
    {
      iNode.m_Node->Push(list, iDelta);
    });

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