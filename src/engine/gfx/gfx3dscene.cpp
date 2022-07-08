/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfx3dscene.hpp>
#include <engine/gfx/model.hpp>
#include <engine/common/gamedata.hpp>
#include <engine/common/transforms.hpp>

namespace eXl
{
  struct Gfx3DSceneEntry 
  {
    IntrusivePtr<Model> m_Model;
    BoundingSphere m_WorldSphere;
  };

  class Gfx3DSceneRenderNode : public GfxRenderNode
  {
    DECLARE_RTTI(Gfx3DSceneRenderNode, GfxRenderNode);
  public:

    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override;
    void Push(OGLDisplayList& iList, float iDelta) override;
    TransformUpdateCallback GetTransformUpdateCallback() override;
    UpdateCallback GetDeleteCallback() override;

    void AddObject(ObjectHandle, IntrusivePtr<Model> iModel);

    //OGLCompiledProgram const* Get3DProgram() { return m_Renderer->m_SpriteProgram.get(); }

  protected:

    void RemoveObject(ObjectHandle);

    Optional<DenseGameDataStorage<Gfx3DSceneEntry>> m_SceneData;
  };

  void Gfx3DScene::Initialize(GfxSystem& iSys)
  {
    m_Sys = &iSys;
    m_Handle = iSys.AddRenderNode(std::make_unique<Gfx3DSceneRenderNode>());
  }

  void Gfx3DScene::Create3DModel(ObjectHandle iObj, IntrusivePtr<Model> iModel)
  {
    Gfx3DSceneRenderNode* renderNode = Gfx3DSceneRenderNode::DynamicCast(m_Sys->GetRenderNode(m_Handle));
    renderNode->AddObject(iObj, iModel);
  }

  void Gfx3DSceneRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
    m_SceneData.emplace(iSys.GetWorld());
  }

  void Gfx3DSceneRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {

  }

  GfxRenderNode::TransformUpdateCallback Gfx3DSceneRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
      {
        if (Gfx3DSceneEntry* entry = m_SceneData->Get(*iObjects))
        {
          entry->m_WorldSphere = BoundingSphere::FromBox((**iTransforms) * entry->m_Model->GetBoundingBox());
        }
      }
    };
  }

  GfxRenderNode::UpdateCallback Gfx3DSceneRenderNode::GetDeleteCallback()
  {
    return [this](ObjectHandle const* iObjects, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects)
      {
        RemoveObject(*iObjects);
      }
    };
  }

  void Gfx3DSceneRenderNode::AddObject(ObjectHandle iObj, IntrusivePtr<Model> iModel)
  {
    if(iModel)
    {
      Gfx3DSceneEntry& entry = m_SceneData->GetOrCreate(iObj);
      if (entry.m_Model == nullptr)
      {
        entry.m_Model = iModel;
        Transforms& trans = *m_Sys->GetWorld().GetSystem<Transforms>();
        Mat4 const& objMat = trans.GetWorldTransform(iObj);
        entry.m_WorldSphere = BoundingSphere::FromBox( objMat * entry.m_Model->GetBoundingBox() );
      }

      GfxRenderNode::AddObject(iObj);
    }
  }

  void Gfx3DSceneRenderNode::RemoveObject(ObjectHandle iObj)
  {
    m_SceneData->Erase(iObj);
  }
}

