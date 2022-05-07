/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include "gfxspriterendernode.hpp"

#include <ogl/renderer/ogldisplaylist.hpp>
#include <engine/common/transforms.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(GfxSpriteRenderNode);

  void GfxSpriteRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
    OGLSpriteAlgo::Init(iSys.GetSemanticManager());
    m_SpriteData.emplace(GetWorld());
    m_Renderer.emplace(iSys, *GetSpriteComponentView(GetWorld()), *m_SpriteData->GetView().GetDenseView());
  }
  
  void GfxSpriteRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {
    m_Renderer->PrepareSprites();
    iList.SetDepth(true, true);
    iList.SetProgram(m_Renderer->m_SpriteProgram.get());
    m_SpriteData->Iterate([&](ObjectHandle object, GfxSpriteData& data)
      {
        if (data.m_Texture != nullptr)
        {
          m_Renderer->TickAnimation(object, data, iDelta);
          data.Push(iList, 0x0100);
        }
      });
  }

  GfxRenderNode::TransformUpdateCallback GfxSpriteRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum)
    {
      m_Renderer->UpdateTransforms(iObjects, iTransforms, iNum);
    };
  }

  GfxRenderNode::UpdateCallback GfxSpriteRenderNode::GetDeleteCallback()
  {
    return [this](ObjectHandle const* iObjects, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects)
      {
        RemoveObject(*iObjects);
      }
    };
  }

  void GfxSpriteRenderNode::SetSpriteDirty(ObjectHandle iObj)
  {
    m_Renderer->m_DirtyComponents.insert(iObj);
  }

  void GfxSpriteRenderNode::AddObject(ObjectHandle iObject)
  {
    GameDataView<GfxSpriteComponent::Desc> const* spriteDescView = GetSpriteComponentView(GetWorld());
    if (spriteDescView->Get(iObject) == nullptr)
    {
      //Ensure a description exists.
      GetSpriteComponentView(GetWorld())->GetOrCreate(iObject);
    }

    m_Renderer->m_DirtyComponents.insert(iObject);
    GfxRenderNode::AddObject(iObject);
  }

  void GfxSpriteRenderNode::RemoveObject(ObjectHandle iObject)
  {
    m_Renderer->m_SpriteData.Erase(iObject);
    m_Renderer->m_DirtyComponents.erase(iObject);
  }
}