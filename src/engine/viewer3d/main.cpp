
#include <engine/common/world.hpp>
#include <engine/common/app.hpp>
#include <core/input.hpp>
#include <core/path.hpp>
#include <core/image/image.hpp>
#include <core/image/imagestreamer.hpp>
#include <core/type/typemanager.hpp>

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/game/character.hpp>

#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/oglmeshalgo.hpp>
#include <ogl/ogltexturecache.hpp>
#include <ogl/oglutils.hpp>

#include <math/mathtools.hpp>

#include <imgui.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include "gfxutils.hpp"

namespace eXl
{
  class World;
  class Random;

  class TextureRefMaterial : public MaterialInfo
  {
  public:

    TextureRefMaterial()
    {
    }

    uint32_t Push(OGLDisplayList& iList) override
    {
      if (m_Texture)
      {
        iList.PushData(m_Texture->GetShaderData(OGLBaseAlgo::GetDiffuseTexture()));

        return 1;
      }
      return 0;
    }

    IntrusivePtr<OGLTextureCacheEntry> m_Texture;
  };


  class MaterialArray : public MaterialInfo
  {
  public:

    uint32_t Push(OGLDisplayList& iList) override
    {
      uint32_t totalData = 0;
      for (auto& matPtr : m_Materials)
      {
        totalData += matPtr->Push(iList);
      }

      return totalData;
    }

    Vector<IntrusivePtr<MaterialInfo>> m_Materials;
  };

  class GfxMeshRenderNode : public GfxRenderNode
  {
    DECLARE_RTTI(GfxMeshRenderNode, GfxRenderNode);
    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override
    {
      GfxRenderNode::Init(iSys, iHandle);

      OGLMeshAlgo::Init(iSys.GetSemanticManager());
      OGLSkyAlgo::Init(iSys.GetSemanticManager());
      OGLIrradianceMapAlgo::Init(iSys.GetSemanticManager());

      m_IBLCompute.emplace(iSys.GetSemanticManager());

      Vector<Image*> skyBoxPlanes;
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\posx.jpg"));
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\negx.jpg"));
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\posy.jpg"));
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\negy.jpg"));
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\posz.jpg"));
      skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMap\\negz.jpg"));


      m_SkyBox = OGLTextureLoader::CreateCubeMap(skyBoxPlanes.data(), true);
      m_EnvBrdfLUT = m_IBLCompute->MakeEnvBrdfMap(iSys.GetSemanticManager(), Vector2i::ONE * 256);

      FILE* tstIrr = fopen("D:\\cubeMap\\Irr_0.png", "r");

      if (tstIrr == NULL)
      {
        m_IrradianceMap = m_IBLCompute->MakeIrradianceCubemap(iSys.GetSemanticManager(), m_SkyBox.get());
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
        m_IrradianceMap = OGLTextureLoader::CreateCubeMap(irrBoxPlanes.data(), true);
      }

      m_IBLCompute->MakeSpecularMipmap(iSys.GetSemanticManager(), m_SkyBox.get());

      MakeBox(m_SkyBoxVtx, Vector3f(50.0, 50.0, 50.0));
      m_NumIdxSphere = MakeSphere(m_SphereAss, 10.0);

      m_MeshProg = OGLMeshAlgo::CreateMeshProgram(iSys.GetSemanticManager());
      m_MeshNormalProg = OGLMeshAlgo::CreateMeshNormalProgram(iSys.GetSemanticManager());
      m_SkyBoxProg = OGLSkyAlgo::CreateSkyProgram(iSys.GetSemanticManager());

      m_White = OGLTextureLoader::Create(Image::Size(4, 4), OGLTextureLoader::RGBA8);
      {
        unsigned int pixels[16];
        memset(pixels, 0xFF, 16 * sizeof(unsigned int));
        m_White->Update(AABB2Di(Vector2i::ZERO, Vector2i::ONE * 4), OGLTextureElementType::BYTE, OGLTextureFormat::RGBA, pixels);
      }
      m_RndData.m_NeutralCamBuffer = OGLBuffer::CreateBuffer(OGLBufferUsage::UNIFORM_BUFFER, sizeof(CameraMatrix), nullptr);
      MathTools::GetPosition(m_RndData.transMat).Z() = -1;

      float ior = 0.0;
      float roughness = 0.1;
      bool metallic = true;
      bool light = true;

      m_RndData.matInfo.m_DiffuseColor = Vector3f(212.0 / 255.0, 175.0 / 255.0, 55.0 / 255.0);
      //matInfo.m_DiffuseColor = Vector3f::ONE;
      m_RndData.matInfo.m_BRDFParameters = Vector4f(ior, roughness, metallic, 1.0);

      m_RndData.matSph.AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_RndData.transMat);
      m_RndData.matSph.AddData(OGLMeshAlgo::GetMaterialInfo(), &m_RndData.matInfo);
      m_RndData.matSph.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), m_White.get());

      m_RndData.envData.AddTexture(OGLMeshAlgo::GetIrradianceMap(), m_IrradianceMap.get());
      m_RndData.envData.AddTexture(OGLMeshAlgo::GetSpecularMap(), m_SkyBox.get());
      m_RndData.envData.AddTexture(OGLMeshAlgo::GetEnvBrdfLUT(), m_EnvBrdfLUT.get());

      m_RndData.skyCam.SetDataBuffer(OGLBaseAlgo::GetCameraUniform(), m_RndData.m_NeutralCamBuffer);
      m_RndData.skyCam.AddTexture(OGLSkyAlgo::GetSkyTexture(), m_SkyBox.get());

      m_RndData.lightInfo.m_Color = Vector3f::ONE * 0.0;
      m_RndData.lightInfo.m_Direction = Vector3f(-1, -1, -1);
      m_RndData.lightInfo.m_Direction.Normalize();
      m_RndData.envData.AddData(OGLMeshAlgo::GetLightInfo(), &m_RndData.lightInfo);

      Vector3f startPos(-100, -100);

      for (uint32_t i = 0; i < 10; ++i)
      {
        for (uint32_t j = 0; j < 10; ++j)
        {
          m_Trans.push_back(Matrix4f::FromPosition(Vector3f(i, j) * 2 + startPos));
        }
      }

      m_PosData.resize(m_Trans.size());

      for (uint32_t i = 0; i < m_Trans.size(); ++i)
      {
        m_PosData[i].AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_Trans[i]);
      }
    }

    TransformUpdateCallback GetTransformUpdateCallback() override
    {
      return TransformUpdateCallback();
    }

    UpdateCallback GetDeleteCallback() override
    {
      return UpdateCallback();
    }

    struct RenderData
    {
      Matrix4f transMat = Matrix4f::IDENTITY;
      CameraMatrix neutralCam;
      IntrusivePtr<OGLBuffer> m_NeutralCamBuffer;
      MeshMaterialInfo matInfo;
      LightInfo lightInfo;
      OGLShaderData skyCam;
      OGLShaderData matSph;
      OGLShaderData envData;
    };

    void Push(OGLDisplayList& iList, float iDelta) override
    {
      //m_IBLCompute->MakeSpecularMipmap(m_Sys->GetSemanticManager(), m_SkyBox.get());

      bool displayNormal = false;

      Matrix4f neutralView = m_Sys->GetCurrentCamera().viewMatrix;
      MathTools::GetPosition(neutralView) = Vector3f::ZERO;

      m_RndData.neutralCam.viewMatrix = neutralView;
      m_RndData.neutralCam.viewInverseMatrix = neutralView.Inverse();
      m_RndData.neutralCam.projMatrix = m_Sys->GetCurrentCamera().projMatrix;
      m_RndData.m_NeutralCamBuffer->SetData(0, sizeof(CameraMatrix), &m_RndData.neutralCam);
      
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

      for (uint32_t t = 0; t < m_Trans.size(); ++t)
      {
        iList.PushData(&m_PosData[t]);
        for (uint32_t i = 0; i < m_Draws.size(); ++i)
        {
          iList.SetVAssembly(&m_Geoms[i]->m_Assembly);

          iList.PushDraw(2, m_Geoms[i]->m_Command, m_Draws[i], 0, 0);
        }
        iList.PopData();
      }

      iList.PopData();
      iList.PopData();
    }

    Optional<IBLCompute> m_IBLCompute;

    RenderData m_RndData;

    IntrusivePtr<OGLTexture> m_SkyBox;
    IntrusivePtr<OGLTexture> m_EnvBrdfLUT;
    IntrusivePtr<OGLTexture> m_IrradianceMap;

    IntrusivePtr<OGLTexture> m_White;

    Vector<IntrusivePtr<GeometryInfo>> m_Geoms;
    Vector<uint32_t> m_Draws;

    Vector<OGLShaderData> m_PosData;
    Vector<Matrix4f> m_Trans;

    OGLCompiledProgram const* m_MeshProg;
    OGLCompiledProgram const* m_MeshNormalProg;
    OGLCompiledProgram const* m_SkyBoxProg;

    OGLVAssembly m_SkyBoxVtx;
    OGLVAssembly m_SphereAss;
    unsigned int m_NumIdxSphere;
  };

  IMPLEMENT_RTTI(GfxMeshRenderNode)

  class ViewerApp : public Scenario
  {
  public:
    ViewerApp()
    {}


    void PreInit(World& iWorld) override
    {
      m_MeshRenderNode = iWorld.GetSystem<GfxSystem>()->AddRenderNode(std::make_unique<GfxMeshRenderNode>());
    }

    void Init(World& iWorld) override;

    void Step(World& iWorld, float iDelta);

    void ProcessInputs(World& iWorld);

    GfxRenderNodeHandle m_MeshRenderNode;
    uint32_t dirMask = 0;
    bool keyChanged = false;
  };

  class ViewerAppPanel : public MenuManager::Panel
  {
  public:
    ViewerAppPanel(World& iWorld, ViewerApp& iScenario)
      : m_World(iWorld)
      , m_Scenario(iScenario)
    {

    }

  protected:
    void Display() override
    {
      
    }
    World& m_World;
    ViewerApp& m_Scenario;
  };

  OGLTextureCache::LoadingCallback MakeLoadCB()
  {
    return [](TextureKey iKey)
    {
      Image* img = ImageStreamer::Load(String(iKey.get()));
      auto ret = IntrusivePtr<OGLTexture>(OGLTextureLoader::CreateFromImage(img, false));
      eXl_DELETE(img);
      return ret;
    };
  }

  void ViewerApp::Init(World& iWorld)
  {

    iWorld.AddTick(World::Stage::FrameStart, [this](World& iWorld, float iDelta)
      {
        Step(iWorld, iDelta);
      });
    Engine_Application& app = Engine_Application::GetAppl();

    app.GetMenuManager().AddMenu("Viewer")
      .AddOpenPanelCommand("Menu", [this, &iWorld] {return new ViewerAppPanel(iWorld, *this); })
      .EndMenu();

    unsigned int ppsteps = 
      aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
      aiProcess_JoinIdenticalVertices | // join identical vertices/ optimize indexing
      aiProcess_ValidateDataStructure | // perform a full validation of the loader's output
      aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
      aiProcess_RemoveRedundantMaterials | // remove redundant materials
      aiProcess_FindDegenerates | // remove degenerated polygons from the import
      aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
      aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
      aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
      aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
      aiProcess_LimitBoneWeights | // limit bone weights to 4 per vertex
      aiProcess_OptimizeMeshes | // join small meshes, if possible;
      aiProcess_SplitByBoneCount | // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
      aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
      aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
      aiProcess_Triangulate | // triangulate polygons with more than 3 edges
      aiProcess_SortByPType | // make 'clean' meshes which consist of a single typ of primitives
      0;


    //aiScene const* scene = aiImportFile("D:\\eXl_Game\\Ultimate Modular Ruins Pack - Aug 2021\\FBX\\Arch_Gothic.fbx", ppsteps);
    aiScene const* scene = aiImportFile("D:\\eXl_Game\\Ultimate Modular Ruins Pack - Aug 2021\\FBX\\Character_Animated.fbx", ppsteps);

    Vector<IntrusivePtr<GeometryInfo>> meshes;
    Vector<uint32_t> drawSize;
    meshes.reserve(scene->mNumMeshes);

    Vector<float> bufferData;
    Vector<uint32_t> idxData;
    for (uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
      aiMesh const& curMesh = *scene->mMeshes[meshIdx];

      uint32_t numV = curMesh.mNumVertices;

      uint32_t vertexFloats = (3 + 3 + 2);
      uint32_t vertexSize = sizeof(float) * vertexFloats;
      size_t totSize = numV * vertexSize;
      if (bufferData.size() < totSize)
      {
        bufferData.resize(totSize);
      }
      std::fill(bufferData.begin(), bufferData.end(), 0.0);

      if (curMesh.HasPositions())
      {
        float* posData = bufferData.data();
        for (uint32_t vtx = 0; vtx < numV; ++vtx)
        {
          posData[0] = curMesh.mVertices[vtx].x;
          posData[1] = curMesh.mVertices[vtx].y;
          posData[2] = curMesh.mVertices[vtx].z;
          posData += vertexFloats;
        }
      }

      if (curMesh.HasNormals())
      {
        float* normalData = bufferData.data() + 3;
        for (uint32_t vtx = 0; vtx < numV; ++vtx)
        {
          normalData[0] = curMesh.mNormals[vtx].x;
          normalData[1] = curMesh.mNormals[vtx].y;
          normalData[2] = curMesh.mNormals[vtx].z;
          normalData += vertexFloats;
        }
      }

      if (curMesh.HasTextureCoords(0))
      {
        float* tcData = bufferData.data() + 6;
        for (uint32_t vtx = 0; vtx < numV; ++vtx)
        {
          tcData[0] = curMesh.mTextureCoords[0][vtx].x;
          tcData[1] = curMesh.mTextureCoords[0][vtx].y;
          tcData += vertexFloats;
        }
      }
      IntrusivePtr<OGLBuffer> vBuffer (OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, totSize, bufferData.data()));

      auto geom = MakeRefCounted<GeometryInfo>();

      geom->m_Assembly.AddAttrib(vBuffer, OGLBaseAlgo::GetPosAttrib(), 3, vertexSize, 0);
      geom->m_Assembly.AddAttrib(vBuffer, OGLMeshAlgo::GetNormalAttrib(), 3, vertexSize, 3 * sizeof(float));
      geom->m_Assembly.AddAttrib(vBuffer, OGLBaseAlgo::GetTexCoordAttrib(), 2, vertexSize, 6 * sizeof(float));
      
      geom->m_Assembly.m_IOffset = 0;

      idxData.clear();
      if (curMesh.HasFaces() && curMesh.mPrimitiveTypes == aiPrimitiveType_TRIANGLE)
      {
        for (uint32_t faceIdx = 0; faceIdx < curMesh.mNumFaces; ++faceIdx)
        {
          aiFace& face = curMesh.mFaces[faceIdx];
          eXl_ASSERT(face.mNumIndices == 3);
          idxData.push_back(face.mIndices[0]);
          idxData.push_back(face.mIndices[1]);
          idxData.push_back(face.mIndices[2]);
        }
        IntrusivePtr<OGLBuffer> idxBuffer(OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, idxData.size() * sizeof(uint32_t), idxData.data()));
        geom->m_Assembly.m_IBuffer = idxBuffer;
      }

      switch (curMesh.mPrimitiveTypes)
      {
      case aiPrimitiveType_TRIANGLE:
        geom->m_Command = OGLDraw::TriangleList;
        break;
      default:
        geom->m_Command = OGLDraw::Point;
        break;
      }

      meshes.push_back(geom);
      drawSize.push_back(idxData.size());
    }

    GfxSystem& gfx = *iWorld.GetSystem<GfxSystem>();
    GfxMeshRenderNode& meshRender = *GfxMeshRenderNode::DynamicCast(gfx.GetRenderNode(m_MeshRenderNode));

    meshRender.m_Geoms = std::move(meshes);
    meshRender.m_Draws = std::move(drawSize);

    GetCamera().view.projection = GfxSystem::Perspective;
    GetCamera().view.displayedSize = 1.0;
  }

  void ViewerApp::Step(World& iWorld, float iDelta)
  {
    ProcessInputs(iWorld);
    
  }

  void ViewerApp::ProcessInputs(World& iWorld)
  {
    Engine_Application& app = Engine_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();
    CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();

    GfxSystem& gfxSys = *iWorld.GetSystem<GfxSystem>();
    Vector2i vptSize = gfxSys.GetViewportSize();

    boost::optional<bool> actionKeyUsed;

    Vector3f(&basis)[3] = GetCamera().view.basis;

    for (int i = 0; i < (int)iInputs.m_KeyEvts.size(); ++i)
    {
      KeyboardEvent& evt = iInputs.m_KeyEvts[i];
      if (!evt.pressed)
      {
        if (evt.key == K_SPACE)
        {
          dirMask &= ~(1 << 2);
          keyChanged = true;
        }
        if (evt.key == K_LCTRL)
        {
          dirMask &= ~(1 << 3);
          keyChanged = true;
        }
        if (evt.key == K_UP)
        {
          dirMask &= ~(1 << 5);
          keyChanged = true;
        }
        if (evt.key == K_DOWN)
        {
          dirMask &= ~(1 << 4);
          keyChanged = true;
        }
        if (evt.key == K_LEFT)
        {
          dirMask &= ~(1 << 1);
          keyChanged = true;
        }
        if (evt.key == K_RIGHT)
        {
          dirMask &= ~(1 << 0);
          keyChanged = true;
        }
      }
      else
      {
        if (evt.key == K_SPACE)
        {
          dirMask |= 1 << 2;
          keyChanged = true;
        }
        if (evt.key == K_LCTRL)
        {
          dirMask |= 1 << 3;
          keyChanged = true;
        }
        if (evt.key == K_UP)
        {
          dirMask |= 1 << 5;
          keyChanged = true;
        }
        if (evt.key == K_DOWN)
        {
          dirMask |= 1 << 4;
          keyChanged = true;
        }
        if (evt.key == K_LEFT)
        {
          dirMask |= 1 << 1;
          keyChanged = true;
        }
        if (evt.key == K_RIGHT)
        {
          dirMask |= 1 << 0;
          keyChanged = true;
        }
      }

      if (keyChanged)
      {
        iInputs.m_KeyEvts.erase(iInputs.m_KeyEvts.begin() + i);
        --i;
      }
    }

    uint32_t mask = dirMask;

    for (auto const& evt : iInputs.m_MouseMoveEvts)
    {
      if (evt.wheel)
      {
      //  if (evt.relY < 0)
      //  {
      //    mask |= 1 << 4;
      //    keyChanged = true;
      //  }
      //  else
      //  {
      //    mask |= 1 << 5;
      //    keyChanged = true;
      //  }
      }
      else
      {
        basis[2] = basis[2] + basis[0] * Mathf::Clamp(-0.005 * evt.relX, -1.0, 1.0) + basis[1] * Mathf::Clamp(0.005 * evt.relY, -1.0, 1.0);
        basis[2].Normalize();

        Vector3f upRef;
        if (Mathf::Abs(basis[2].Dot(Vector3f::UNIT_Y)) < (1.0 - Mathf::ZERO_TOLERANCE))
          upRef = Vector3f::UNIT_Y;
        else
          upRef = basis[1];

        basis[0] = upRef.Cross(basis[2]);
        basis[0].Normalize();
        basis[1] = basis[2].Cross(basis[0]);
        basis[1].Normalize();
      }
    }

    if (keyChanged)
    {
      static const Vector3f dirs[] =
      {
        Vector3f::UNIT_X * 1.0,
        Vector3f::UNIT_X * -1.0,
        Vector3f::UNIT_Y * 1.0,
        Vector3f::UNIT_Y * -1.0,
        Vector3f::UNIT_Z * 1.0,
        Vector3f::UNIT_Z * -1.0,
      };
      Vector3f dir;
      for (unsigned int i = 0; i < 6; ++i)
      {
        if (mask & (1 << i))
        {
          dir += dirs[i];
        }
      }

      controller.SetSpeed(GetCamera().cameraObj, 100.0);
      dir = dir.X() * basis[0] + dir.Y() * basis[1] + dir.Z() * basis[2];
      controller.SetCurDir(GetCamera().cameraObj, dir);
    }
  }
}

#include <engine/eXl_Main.hpp>

EXL_MAIN_WITH_SCENARIO_AND_PROJECT(ViewerApp, "eXl_ForestProject/ForestProject.eXlProject")
