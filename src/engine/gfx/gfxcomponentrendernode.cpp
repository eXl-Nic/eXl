/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include "gfxcomponentrendernode.hpp"

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/oglutils.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>
#include <core/type/tagtype.hpp>


namespace eXl
{

  void GfxComponentRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
  }

  void GfxComponentRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {
    iList.SetDepth(true, true);
    m_ToRender.Iterate([&](GfxComponent& comp, Components::Handle)
      {
        comp.Push(iList);
      });
  }

  GfxRenderNode::TransformUpdateCallback GfxComponentRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
      {
        Components::Handle objHandle = m_ObjectToComp[iObjects->GetId()];
        if (auto* obj = m_ToRender.TryGet(objHandle))
        {
          obj->SetTransform(**iTransforms);
        }
      }
    };
  }

  GfxRenderNode::UpdateCallback GfxComponentRenderNode::GetDeleteCallback()
  {
    return [this](ObjectHandle const* iObjects, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects)
      {
        RemoveObject(*iObjects);
      }
    };
  }

  GfxComponent* GfxComponentRenderNode::GetComponent(ObjectHandle iObject)
  {
    if(iObject.GetId() < m_ObjectToComp.size())
    {
      return &m_ToRender.Get(m_ObjectToComp[iObject.GetId()]);
    }
    return nullptr;
  }

  void GfxComponentRenderNode::AddObject(ObjectHandle iObject)
  {
    eXl_ASSERT(m_Sys != nullptr);
    Components::Handle compHandle = m_ToRender.Alloc();
    GfxComponent& newComp = m_ToRender.Get(compHandle);
    newComp.m_Object = iObject;

    if (m_Sys->GetTransforms().HasTransform(iObject))
    {
      newComp.m_Transform = m_Sys->GetTransforms().GetWorldTransform(iObject);
    }
    else
    {
      newComp.m_Transform = Identity<Mat4>();
    }

    while (m_ObjectToComp.size() <= iObject.GetId())
    {
      m_ObjectToComp.push_back(Components::Handle());
    }
    m_ObjectToComp[iObject.GetId()] = compHandle;

    GfxRenderNode::AddObject(iObject);
  }

  void GfxComponentRenderNode::RemoveObject(ObjectHandle iObject)
  {
    eXl_ASSERT(iObject.GetId() < m_ObjectToComp.size() && m_ToRender.IsValid(m_ObjectToComp[iObject.GetId()]));
    m_ToRender.Release(m_ObjectToComp[iObject.GetId()]);
  }
}