/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfx3dscene.hpp>
#include <engine/gfx/model.hpp>
#include <engine/gfx/gfx3dutils.hpp>
#include <engine/common/data_tables/multi.hpp>
#include <engine/common/transforms.hpp>

#include <ogl/oglmeshalgo.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>

#include <core/image/imagestreamer.hpp>

namespace eXl
{
  struct Gfx3DSceneEntry 
  {
    IntrusivePtr<Model> m_Model;
    bool m_Hidden = false;
  };

  struct RenderData
  {
    Mat4 transMat = Identity<Mat4>();
    CameraMatrix neutralCam;
    IntrusivePtr<OGLBuffer> m_NeutralCamBuffer;
    MeshMaterialInfo matInfo;
    LightInfo lightInfo;
    OGLShaderData skyCam;
    OGLShaderData matSph;
    OGLShaderData envData;
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
    void HideObject(ObjectHandle iObj, bool iHide);
    Optional<IBLCompute> m_IBLCompute;

    RenderData m_RndData;

    IntrusivePtr<OGLTexture> m_White;
    IntrusivePtr<OGLTexture> m_SkyBox;
    IntrusivePtr<OGLTexture> m_EnvBrdfLUT;
    IntrusivePtr<OGLTexture> m_IrradianceMap;

    Vector<IntrusivePtr<Geometry>> m_Geoms;
    Vector<uint32_t> m_Draws;


    OGLCompiledProgram const* m_MeshProg;
    OGLCompiledProgram const* m_MeshNormalProg;
    OGLCompiledProgram const* m_SkyBoxProg;

    OGLVAssembly m_SkyBoxVtx;
    //OGLVAssembly m_SphereAss;
    //unsigned int m_NumIdxSphere;

  protected:

    Optional<MultiDataStorage<Gfx3DSceneEntry, BoundingSphere, Mat4, OGLShaderData>> m_SceneData;

    void RemoveObject(ObjectHandle);
  };

  IMPLEMENT_RTTI(Gfx3DSceneRenderNode);

  void Gfx3DSceneRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
    m_SceneData.emplace(iSys.GetWorld());

    OGLMeshAlgo::Init(iSys.GetSemanticManager());
    OGLSkyAlgo::Init(iSys.GetSemanticManager());
    OGLIrradianceMapAlgo::Init(iSys.GetSemanticManager());

    m_IBLCompute.emplace(iSys.GetSemanticManager());

    MakeBox(m_SkyBoxVtx, Vec3(50.0, 50.0, 50.0));
    //m_NumIdxSphere = MakeSphere(m_SphereAss, 10.0);

    m_MeshProg = OGLMeshAlgo::CreateMeshProgram(iSys.GetSemanticManager());
    m_MeshNormalProg = OGLMeshAlgo::CreateMeshNormalProgram(iSys.GetSemanticManager());
    m_SkyBoxProg = OGLSkyAlgo::CreateSkyProgram(iSys.GetSemanticManager());

    m_White = Tileset::GetWhiteTexture()->GetTexture("White");
    m_RndData.m_NeutralCamBuffer = OGLBuffer::CreateBuffer(OGLBufferUsage::UNIFORM_BUFFER, sizeof(CameraMatrix), nullptr);
    m_RndData.transMat[3][2] = -1;

    float ior = 1.0;
    float roughness = 0.1;
    bool metallic = false;
    bool light = true;

    m_RndData.matInfo.m_DiffuseColor = Vec3(212.0 / 255.0, 175.0 / 255.0, 55.0 / 255.0);
    //matInfo.m_DiffuseColor = Vector3f::ONE;
    m_RndData.matInfo.m_BRDFParameters = Vec4(ior, roughness, metallic, 1.0);

    m_RndData.matSph.AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_RndData.transMat);
    m_RndData.matSph.AddData(OGLMeshAlgo::GetMaterialInfo(), &m_RndData.matInfo);
    m_RndData.matSph.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), m_White.get());

    m_RndData.lightInfo.m_Color = One<Vec3>() * 0.0;
    m_RndData.lightInfo.m_Direction = normalize(Vec3(-1, -1, -1));
  }

  void Gfx3DSceneRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {
    bool displayNormal = false;

    Mat4 neutralView = m_Sys->GetCurrentCamera().viewMatrix;
    neutralView[3] = Vec4(Zero<Vec3>(), 1);

    m_RndData.envData.Clear();

    m_RndData.envData.AddData(OGLMeshAlgo::GetLightInfo(), &m_RndData.lightInfo);
    m_RndData.envData.AddTexture(OGLMeshAlgo::GetIrradianceMap(), m_IrradianceMap.get());
    m_RndData.envData.AddTexture(OGLMeshAlgo::GetSpecularMap(), m_SkyBox.get());
    m_RndData.envData.AddTexture(OGLMeshAlgo::GetEnvBrdfLUT(), m_EnvBrdfLUT.get());

    m_RndData.neutralCam.viewMatrix = neutralView;
    m_RndData.neutralCam.viewInverseMatrix = inverse(neutralView);
    m_RndData.neutralCam.projMatrix = m_Sys->GetCurrentCamera().projMatrix;
    m_RndData.m_NeutralCamBuffer->SetData(0, sizeof(CameraMatrix), &m_RndData.neutralCam);

    m_RndData.skyCam.Clear();
    m_RndData.skyCam.SetDataBuffer(OGLBaseAlgo::GetCameraUniform(), m_RndData.m_NeutralCamBuffer);
    m_RndData.skyCam.AddTexture(OGLSkyAlgo::GetSkyTexture(), m_SkyBox.get());

    iList.SetDepth(false, false);

    iList.PushData(&m_RndData.skyCam);
    iList.SetVAssembly(&m_SkyBoxVtx);
    iList.SetProgram(m_SkyBoxProg);
    iList.PushDraw(1, OGLDraw::TriangleList, 36, 0, 0);
    iList.PopData();

    iList.SetDepth(true, true);

    iList.PushData(&m_RndData.envData);

    //tempList.PushData(&idMatrixData);
    iList.PushData(&m_RndData.matSph);

    OGLCompiledProgram const* meshTech = displayNormal ? m_MeshNormalProg : m_MeshProg;

    iList.SetProgram(meshTech);

    //iList.SetVAssembly(&m_SphereAss);

    //iList.PushDraw(2, OGLDraw::TriangleList, m_NumIdxSphere, 0, 0);

    m_SceneData->Iterate([&iList](ObjectHandle
      , Gfx3DSceneEntry const& iEntry
      , BoundingSphere const& iSphere
      , Mat4 const&
      , OGLShaderData const& iData) 
      {
        if (iEntry.m_Hidden == false)
        {
          iList.PushData(&iData);
          iEntry.m_Model->Draw(iList);
          iList.PopData();
        }
      });

    iList.PopData();
    iList.PopData();
  }

  GfxRenderNode::TransformUpdateCallback Gfx3DSceneRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
      {
        auto entryTuple = m_SceneData->Get(*iObjects);
        if (entryTuple)
        {
          auto [entry, sphere, mat, data] = *entryTuple;
          sphere = BoundingSphere::FromBox((**iTransforms) * entry.m_Model->GetBoundingBox());
          mat = (**iTransforms);
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
      auto [entry, sphere, mat, data] = m_SceneData->GetOrCreate(iObj);
      if (entry.m_Model == nullptr)
      {
        entry.m_Model = iModel;
        Transforms& trans = *m_Sys->GetWorld().GetSystem<Transforms>();
        mat = trans.GetWorldTransform(iObj);
        sphere = BoundingSphere::FromBox( mat * entry.m_Model->GetBoundingBox() );
        data.AddData(OGLBaseAlgo::GetWorldMatUniform(), &mat);
      }

      GfxRenderNode::AddObject(iObj);
    }
  }

  void Gfx3DSceneRenderNode::RemoveObject(ObjectHandle iObj)
  {
    m_SceneData->Erase(iObj);
  }

  void Gfx3DSceneRenderNode::HideObject(ObjectHandle iObj, bool iHide)
  {
    if (Gfx3DSceneEntry* entry = m_SceneData->GetView<0>().Get(iObj))
    {
      entry->m_Hidden = iHide;
    }
  }

  void Gfx3DScene::Initialize(GfxSystem& iSys)
  {
    m_Sys = &iSys;
    m_Handle = iSys.AddRenderNode(std::make_unique<Gfx3DSceneRenderNode>());
  }

  void Gfx3DScene::Add3DModel(ObjectHandle iObj, IntrusivePtr<Model> iModel)
  {
    Gfx3DSceneRenderNode* renderNode = Gfx3DSceneRenderNode::DynamicCast(m_Sys->GetRenderNode(m_Handle));
    renderNode->AddObject(iObj, iModel);
  }

  void Gfx3DScene::Hide3DModel(ObjectHandle iObj, bool iHide)
  {
    Gfx3DSceneRenderNode* renderNode = Gfx3DSceneRenderNode::DynamicCast(m_Sys->GetRenderNode(m_Handle));
    renderNode->HideObject(iObj, iHide);
  }

  void Gfx3DScene::SetSkybox(Vector<Image*> const& iPlanes)
  {
    Gfx3DSceneRenderNode* renderNode = Gfx3DSceneRenderNode::DynamicCast(m_Sys->GetRenderNode(m_Handle));

    renderNode->m_SkyBox = OGLTextureLoader::CreateCubeMap(iPlanes.data(), true);
    renderNode->m_EnvBrdfLUT = renderNode->m_IBLCompute->MakeEnvBrdfMap(m_Sys->GetSemanticManager(), One<Vec2i>() * 256);

    FILE* tstIrr = fopen("D:\\cubeMap\\Irr_0.png", "r");

    if (tstIrr == NULL)
    {
      renderNode->m_IrradianceMap = renderNode->m_IBLCompute->MakeIrradianceCubemap(m_Sys->GetSemanticManager(), renderNode->m_SkyBox.get());
    }
    else
    {
      fclose(tstIrr);
      std::vector<Image*> irrBoxPlanes;
      for (unsigned int i = 0; i < 6; ++i)
      {
        char buffer[512];
        sprintf(buffer, "D:\\cubeMap\\Irr_%i.png", i);
        irrBoxPlanes.push_back(ImageStreamer::Load(buffer));
      }
      renderNode->m_IrradianceMap = OGLTextureLoader::CreateCubeMap(irrBoxPlanes.data(), true);
    }

    renderNode->m_IBLCompute->MakeSpecularMipmap(m_Sys->GetSemanticManager(), renderNode->m_SkyBox.get());
  }
}

