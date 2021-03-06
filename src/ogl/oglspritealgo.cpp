/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/oglspritealgo.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/oglutils.hpp>

#include "ogldefaultVS.inl"
#include "ogldefaultPS.inl"
#include "oglhq2x.inl"
#include "oglhq4x.inl"
#include "ogllinePS.inl"

#include "oglmeshVS.inl"
#include "oglmeshPS.inl"
#include "oglmeshNormalPS.inl"

#include "oglskyboxVS.inl"
#include "oglskyboxPS.inl"

#include "oglscreenquadVS.inl"
#include "oglirradiancePS.inl"
#include "oglspecularradiancePS.inl"

#include <ogl/renderer/oglinclude.hpp>


#include <math/math.hpp>
#include <core/type/dynobject.hpp>
#include <core/type/tupletypestruct.hpp>
#include <core/type/typemanager.hpp>

namespace eXl
{
  namespace
  {
  }

  struct dummyStruct
  {
    Mat4 dummyMat;
  };

  void OGLBaseAlgo::Init(OGLSemanticManager& iManager)
  {
    iManager.RegisterAttribute(GetPosAttrib(), OGLType::FLOAT32,3);
    iManager.RegisterAttribute(GetTexCoordAttrib(), OGLType::FLOAT32,2);

    TupleType const* type = TupleType::DynamicCast(CameraMatrix::GetType());;
      
    iManager.RegisterUniformData(GetCameraUniform(), type);

    List<FieldDesc> fieldList;
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("worldMatrix"),&dummyStruct::dummyMat));
    type = TupleTypeStruct::Create(fieldList);

    iManager.RegisterUniformData(GetWorldMatUniform() ,type);

    OGLSamplerDesc samplerDesc;
    samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
    samplerDesc.maxFilter = OGLMagFilter::LINEAR;
    samplerDesc.minFilter = OGLMinFilter::LINEAR;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetDiffuseTexture() ,samplerDesc);
  }

  AttributeName OGLBaseAlgo::GetPosAttrib()
  {
    static AttributeName s_Name("iPosition");
    return s_Name;
  }

  AttributeName OGLBaseAlgo::GetTexCoordAttrib()
  {
    static AttributeName s_Name("iTexCoord");
    return s_Name;
  }

  UniformName OGLBaseAlgo::GetWorldMatUniform()
  {
    static UniformName s_Name("worldMatrix");
    return s_Name;
  }

  UniformName OGLBaseAlgo::GetCameraUniform()
  {
    static UniformName s_Name("Camera");
    return s_Name;
  }

  TextureName OGLBaseAlgo::GetDiffuseTexture()
  {
    static TextureName s_Name("iDiffuseTexture");
    return s_Name;
  }

  OGLCompiledProgram const* OGLSpriteAlgo::CreateSpriteProgram(OGLSemanticManager& iSemantics, bool iFiltered)
  {
#ifdef EXL_WITH_OGL
    if (!iFiltered)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, defaultVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, defaultUPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader, defaultFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

      OGLProgramInterface sprTechDesc;
      sprTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
      sprTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
      sprTechDesc.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture());
      sprTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
      sprTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());
      sprTechDesc.AddUniform(OGLSpriteAlgo::GetSpriteColorUniform());

      return sprTechDesc.Compile(iSemantics, defaultProgram);
    }
    else
    {
      GLuint hq4xVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, hq4xVS);
      GLuint unfilteredFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, hq4xPS);

      GLuint unfilteredProgramId = OGLUtils::LinkProgram(hq4xVShader, unfilteredFShader);

      glDeleteShader(hq4xVShader);
      glDeleteShader(unfilteredFShader);

      OGLProgram* unfilteredProgram = eXl_NEW OGLProgram(unfilteredProgramId);

      OGLProgramInterface uSprTechDesc;
      uSprTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
      uSprTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
      uSprTechDesc.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture());
      uSprTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
      uSprTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());
      uSprTechDesc.AddUniform(OGLSpriteAlgo::GetSpriteColorUniform());

      return uSprTechDesc.Compile(iSemantics, unfilteredProgram);
    }
#else
    return nullptr;
#endif
  }

  OGLCompiledProgram const* OGLSpriteAlgo::CreateFontProgram(OGLSemanticManager& iSemantics)
  {
#ifdef EXL_WITH_OGL
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, defaultVS);
    GLuint fontFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, fontPS);
    GLuint fontProgramId = OGLUtils::LinkProgram(defaultVShader, fontFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(fontFShader);

    OGLProgram* fontProgram = eXl_NEW OGLProgram(fontProgramId);

    OGLProgramInterface fontTechDesc;
    fontTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
    fontTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
    fontTechDesc.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture());
    fontTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
    fontTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());
    fontTechDesc.AddUniform(OGLSpriteAlgo::GetSpriteColorUniform());

    return fontTechDesc.Compile(iSemantics, fontProgram);
#else
    return nullptr;
#endif
  }

  UniformName OGLSpriteAlgo::GetSpriteColorUniform()
  {
    static UniformName s_Name("SpriteColor");
    return s_Name;
  }

  TextureName OGLSpriteAlgo::GetUnfilteredTexture()
  {
    static TextureName s_Name("iUnfilteredTexture");
    return s_Name;
  }

  void OGLSpriteAlgo::Init(OGLSemanticManager& iManager)
  {
    TupleType const* type = TupleType::DynamicCast(SpriteColor::GetType());

    iManager.RegisterUniformData(GetSpriteColorUniform(), type);

    OGLSamplerDesc samplerDesc;
    samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
    samplerDesc.maxFilter = OGLMagFilter::NEAREST;
    samplerDesc.minFilter = OGLMinFilter::NEAREST;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetUnfilteredTexture(), samplerDesc);
  }

#if 0
  void OGLSpriteAlgo::ShutdownAPI()
  {
    if(s_SpriteTech)
    {
      if(OGLProgram const* progS = s_SpriteTech->GetProgram())
      {
        GLuint progNameS = progS->GetProgName();
        eXl_DELETE progS;
        glDeleteProgram(progNameS);
      }
      eXl_DELETE s_SpriteTech;
      s_SpriteTech = NULL;
    }
    
    if(s_FontTech)
    {
      if(OGLProgram const* progF = s_FontTech->GetProgram())
      {
        GLuint progNameF = progF->GetProgName();
        eXl_DELETE progF;
        glDeleteProgram(progNameF);
      }
      eXl_DELETE s_FontTech;
      s_FontTech = NULL; 
    }

    if(s_USpriteTech)
    {
      if(OGLProgram const* progUS = s_USpriteTech->GetProgram())
      {
        GLuint progNameUS = progUS->GetProgName();
        eXl_DELETE progUS;
        glDeleteProgram(progNameUS);
      }

      eXl_DELETE s_USpriteTech;
      s_USpriteTech = NULL;
    }
  }
#endif

  struct dummyColor
  {
    Vec4 m_Color;
  };

  OGLCompiledProgram const* OGLLineAlgo::CreateProgram(OGLSemanticManager& iSemantics)
  {
#ifdef EXL_WITH_OGL
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, defaultVS);
    GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, linePS);

    GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader, defaultFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(defaultFShader);

    OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

    OGLProgramInterface lineTechDesc;
    lineTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
    lineTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
    lineTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
    lineTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());
    lineTechDesc.AddUniform(OGLLineAlgo::GetColor());

    return lineTechDesc.Compile(iSemantics, defaultProgram);
#else
    return nullptr;
#endif
  }

  UniformName OGLLineAlgo::GetColor()
  {
    static UniformName s_Name("LineColor");
    return s_Name;
  }


  void OGLLineAlgo::Init(OGLSemanticManager& iManager)
  {
    List<FieldDesc> fieldList;
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("color"),&dummyColor::m_Color));
    TupleType const* type = TupleTypeStruct::Create(fieldList);
    iManager.RegisterUniformData(GetColor(), type);
  }

#if 0

  uint32_t s_LightInfo      = 1<<31;
  uint32_t s_MaterialInfo   = 1<<31;
  uint32_t s_NormalAttrib   = 1<<31;
  uint32_t s_IrradianceMap  = 1<<31;
  uint32_t s_SpecularEnvMap = 1<<31;
  uint32_t s_EnvBrdfLut     = 1<<31;

  OGLCompiledProgram const* s_MeshTech = NULL;
  OGLCompiledProgram const* s_MeshNormalTech = NULL;

  void OGLMeshAlgo::Init()
  {
    if(s_NormalAttrib == 1<<31)
    {
      s_NormalAttrib = OGLSemanticManager::RegisterAttribute("iNormal",OGLType::FLOAT32,3);
    }

    if(s_IrradianceMap == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_IrradianceMap = OGLSemanticManager::RegisterTexture("iIrradianceMap", samplerDesc);
    }

    if(s_SpecularEnvMap == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_CUBE_MAP;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_SpecularEnvMap = OGLSemanticManager::RegisterTexture("iSpecularEnvMap", samplerDesc);
    }

    if(s_EnvBrdfLut == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_EnvBrdfLut = OGLSemanticManager::RegisterTexture("iEnvBrdfLUT", samplerDesc);
    }

    if(s_LightInfo == 1<<31)
    {
      List<FieldDesc> fieldList;
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iLightDir"),&LightInfo::m_Direction));
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iLightColor"),&LightInfo::m_Color));

      TupleType const* type = TupleTypeStruct::Create(fieldList);

      s_LightInfo = OGLSemanticManager::RegisterUniformData("LightInfo",type);

      fieldList.clear();

      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iDiffuseColor"),&MeshMaterialInfo::m_DiffuseColor));
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iBRDFParameters"),&MeshMaterialInfo::m_BRDFParameters));

      type = TupleTypeStruct::Create(fieldList);

      s_MaterialInfo = OGLSemanticManager::RegisterUniformData("MaterialInfo",type);

    }
    if(s_MeshTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER,meshVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,meshPS);
      GLuint normalFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,meshNormalPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);
      GLuint normalProgramId = OGLUtils::LinkProgram(defaultVShader,normalFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);
      glDeleteShader(normalFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);
      OGLProgram* normalProgram = eXl_NEW OGLProgram(normalProgramId);

      OGLProgramInterface meshTechDesc;
      meshTechDesc.AddAttrib(s_PosAttrib);
      meshTechDesc.AddAttrib(s_NormalAttrib);
      meshTechDesc.AddAttrib(s_TexAttrib);
      meshTechDesc.AddTexture(s_DiffuseTexture);
      meshTechDesc.AddTexture(s_IrradianceMap);
      meshTechDesc.AddTexture(s_SpecularEnvMap);
      meshTechDesc.AddTexture(s_EnvBrdfLut);
      meshTechDesc.AddUniform(s_CameraUnif);
      meshTechDesc.AddUniform(s_WorldMatUnif);
      meshTechDesc.AddUniform(s_LightInfo);
      meshTechDesc.AddUniform(s_MaterialInfo);

      s_MeshTech = meshTechDesc.Compile(defaultProgram);
      s_MeshNormalTech = meshTechDesc.Compile(normalProgram);
    }
  }

  uint32_t OGLMeshAlgo::GetIrradianceMap()
  {
    return s_IrradianceMap;
  }

  uint32_t OGLMeshAlgo::GetSpecularMap()
  {
    return s_SpecularEnvMap;
  }

  uint32_t OGLMeshAlgo::GetEnvBrdfLUT()
  {
    return s_EnvBrdfLut;
  }

  uint32_t OGLMeshAlgo::GetNormalAttrib()
  {
    return s_NormalAttrib;
  }

  OGLCompiledProgram const* OGLMeshAlgo::GetMeshProgram()
  {
    return s_MeshTech;
  }

  OGLCompiledProgram const* OGLMeshAlgo::GetMeshNormalProgram()
  {
    return s_MeshNormalTech;
  }

  uint32_t OGLMeshAlgo::GetLightInfo()
  {
    return s_LightInfo;
  }

  uint32_t OGLMeshAlgo::GetMaterialInfo()
  {
    return s_MaterialInfo;
  }

  uint32_t s_SkyTexture    = 1<<31;
  
  OGLCompiledProgram const* s_SkyTech = NULL;

  void OGLSkyAlgo::Init()
  {
    if(s_SkyTexture == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_CUBE_MAP;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_SkyTexture = OGLSemanticManager::RegisterTexture("iSkyBox", samplerDesc);
    }

    if(s_SkyTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER,skyBoxVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,skyBoxPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

      OGLProgramInterface skyTechDesc;
      skyTechDesc.AddAttrib(s_PosAttrib);
      skyTechDesc.AddAttrib(s_NormalAttrib);
      skyTechDesc.AddTexture(s_SkyTexture);
      skyTechDesc.AddUniform(s_CameraUnif);

      s_SkyTech = skyTechDesc.Compile(defaultProgram);
    }
  }

  uint32_t OGLSkyAlgo::GetSkyTexture()
  {
    return s_SkyTexture;
  }

  OGLCompiledProgram const* OGLSkyAlgo::GetSkyProgram()
  {
    return s_SkyTech;
  }

  uint32_t s_IrradianceAlgoInfo = 1<<31;

  OGLCompiledProgram const* s_IrradianceMapTech = NULL;
  OGLCompiledProgram const* s_SpecularMapTech = NULL;
  OGLCompiledProgram const* s_EnvBrdfTech = NULL;

  void OGLIrradianceMapAlgo::Init()
  {
    if(s_IrradianceAlgoInfo == 1<<31)
    {
      List<FieldDesc> fieldList;
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iPhiComputationRange"),&AlgoData::phiComputationRange));
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iThetaComputationRange"),&AlgoData::thetaComputationRange));
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iCubeFace"),&AlgoData::face));

      TupleType const* type = TupleTypeStruct::Create(fieldList);

      s_IrradianceAlgoInfo = OGLSemanticManager::RegisterUniformData("IrradianceAlgoData",type);
    }

    if(s_IrradianceMapTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER,screenQuadVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,irradianceMapPS);
      GLuint specularFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,specularMapPS);
      GLuint envBrdfFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,envBrdfMapPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);
      GLuint specularProgramId = OGLUtils::LinkProgram(defaultVShader,specularFShader);
      GLuint envBProgramId = OGLUtils::LinkProgram(defaultVShader,envBrdfFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);
      glDeleteShader(specularFShader);
      glDeleteShader(envBrdfFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);
      OGLProgram* specularProgram = eXl_NEW OGLProgram(specularProgramId);
      OGLProgram* envBProgram = eXl_NEW OGLProgram(envBProgramId);

      OGLProgramInterface irradianceTechDesc;
      irradianceTechDesc.AddTexture(s_SkyTexture);
      irradianceTechDesc.AddUniform(s_IrradianceAlgoInfo);

      s_IrradianceMapTech = irradianceTechDesc.Compile(defaultProgram);
      s_SpecularMapTech = irradianceTechDesc.Compile(specularProgram);
      s_EnvBrdfTech = irradianceTechDesc.Compile(envBProgram);
    }
  }

  uint32_t OGLIrradianceMapAlgo::GetAlgoInfo()
  {
    return s_IrradianceAlgoInfo;
  }

  void OGLIrradianceMapAlgo::ShutdownAPI()
  {

  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::GetIrradianceMapProgram()
  {
    return s_IrradianceMapTech;
  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::GetSpecularMapProgram()
  {
    return s_SpecularMapTech;
  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::GetEnvBrdfProgram()
  {
    return s_EnvBrdfTech;
  }

#endif
}
