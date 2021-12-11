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
    uint32_t s_PosAttrib      = 1<<31;
    uint32_t s_TexAttrib      = 1<<31;
    uint32_t s_WorldMatUnif   = 1<<31;
    uint32_t s_CameraUnif     = 1<<31;
    uint32_t s_DiffuseTexture = 1<<31;
    uint32_t s_UnfilteredTexture = 1<<31;
    uint32_t s_SpriteColor    = 1<<31;
    uint32_t s_LineColor      = 1<<31;

    OGLCompiledProgram const* s_SpriteTech = NULL;
    OGLCompiledProgram const* s_USpriteTech = NULL;
    OGLCompiledProgram const* s_FontTech = NULL;

    OGLCompiledProgram const* s_LineTech = NULL;
  }

  struct dummyStruct
  {
    Matrix4f dummyMat;
  };

  void OGLBaseAlgo::Init()
  {
    if(s_PosAttrib == 1<<31)
    {
#ifndef __ANDROID__
      glewInit();
#endif

      //LOG_INFO << "GL version : " << (char*)glGetString(GL_VERSION) << "\n";

      s_PosAttrib = OGLSemanticManager::RegisterAttribute("iPosition",OGLType::FLOAT32,3);
      s_TexAttrib = OGLSemanticManager::RegisterAttribute("iTexCoord",OGLType::FLOAT32,2);

      TupleType const* type = TupleType::DynamicCast(CameraMatrix::GetType());;
      
      s_CameraUnif = OGLSemanticManager::RegisterUniformData("Camera",type);

      List<FieldDesc> fieldList;
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("worldMatrix"),&dummyStruct::dummyMat));
      type = TupleTypeStruct::Create(fieldList);

      s_WorldMatUnif = OGLSemanticManager::RegisterUniformData("WorldMatrix",type);

      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_DiffuseTexture = OGLSemanticManager::RegisterTexture("iDiffuseTexture",samplerDesc);
    }
  }

  uint32_t OGLBaseAlgo::GetPosAttrib()
  {
    return s_PosAttrib;
  }

  uint32_t OGLBaseAlgo::GetTexCoordAttrib()
  {
    return s_TexAttrib;
  }

  uint32_t OGLBaseAlgo::GetWorldMatUniform()
  {
    return s_WorldMatUnif;
  }

  uint32_t OGLBaseAlgo::GetCameraUniform()
  {
    return s_CameraUnif;
  }

  uint32_t OGLBaseAlgo::GetDiffuseTexture()
  {
    return s_DiffuseTexture;
  }

  OGLCompiledProgram const* OGLSpriteAlgo::GetSpriteProgram(bool iFiltered)
  {
    if(iFiltered)
      return s_SpriteTech;
    else
      return s_USpriteTech;
  }


  OGLCompiledProgram const* OGLSpriteAlgo::GetFontProgram()
  {
    return s_FontTech;
  }

  uint32_t OGLSpriteAlgo::GetSpriteColorUniform()
  {
    return s_SpriteColor;
  }

  uint32_t OGLSpriteAlgo::GetUnfilteredTexture()
  {
    return s_UnfilteredTexture;
  }

  void OGLSpriteAlgo::Init()
  {
    if(s_SpriteColor == 1<<31)
    {
      TupleType const* type = TupleType::DynamicCast(SpriteColor::GetType());

      s_SpriteColor = OGLSemanticManager::RegisterUniformData("SpriteColor",type);

      OGLSamplerDesc samplerDesc;
      samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
      samplerDesc.maxFilter = OGLMagFilter::NEAREST;
      samplerDesc.minFilter = OGLMinFilter::NEAREST;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_UnfilteredTexture = OGLSemanticManager::RegisterTexture("iUnfilteredTexture",samplerDesc);
    }
    if(s_SpriteTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, defaultVS);
      GLuint hq4xVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, hq4xVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, defaultPS);
      GLuint unfilteredFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, hq4xPS);
      GLuint fontFShader    = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,fontPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);
      GLuint unfilteredProgramId = OGLUtils::LinkProgram(hq4xVShader,unfilteredFShader);

      GLuint fontProgramId    = OGLUtils::LinkProgram(defaultVShader,fontFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);
      glDeleteShader(fontFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);
      OGLProgram* unfilteredProgram = eXl_NEW OGLProgram(unfilteredProgramId);
      OGLProgram* fontProgram    = eXl_NEW OGLProgram(fontProgramId);

      {
        OGLProgramInterface sprTechDesc;
        sprTechDesc.AddAttrib(s_PosAttrib);
        sprTechDesc.AddAttrib(s_TexAttrib);
        sprTechDesc.AddTexture(s_DiffuseTexture);
        sprTechDesc.AddUniform(s_CameraUnif);
        sprTechDesc.AddUniform(s_WorldMatUnif);
        sprTechDesc.AddUniform(s_SpriteColor);

        s_SpriteTech = sprTechDesc.Compile(defaultProgram);
      }

      {
        OGLProgramInterface uSprTechDesc;
        uSprTechDesc.AddAttrib(s_PosAttrib);
        uSprTechDesc.AddAttrib(s_TexAttrib);
        uSprTechDesc.AddTexture(s_UnfilteredTexture);
        uSprTechDesc.AddUniform(s_CameraUnif);
        uSprTechDesc.AddUniform(s_WorldMatUnif);
        uSprTechDesc.AddUniform(s_SpriteColor);

        s_USpriteTech = uSprTechDesc.Compile(unfilteredProgram);
      }

      {
        OGLProgramInterface fontTechDesc;
        fontTechDesc.AddAttrib(s_PosAttrib);
        fontTechDesc.AddAttrib(s_TexAttrib);
        fontTechDesc.AddTexture(s_UnfilteredTexture);
        fontTechDesc.AddUniform(s_CameraUnif);
        fontTechDesc.AddUniform(s_WorldMatUnif);
        fontTechDesc.AddUniform(s_SpriteColor);

        s_FontTech = fontTechDesc.Compile(fontProgram);
      }

      //glUseProgram(m_Impl->defaultProgram->GetProgName());

    }
  }

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

  struct dummyColor
  {
    Vector4f m_Color;
  };

  OGLCompiledProgram const* OGLLineAlgo::GetProgram()
  {
    return s_LineTech;
  }

  uint32_t OGLLineAlgo::GetColor()
  {
    return s_LineColor;
  }


  void OGLLineAlgo::Init()
  {
    if(s_LineColor == 1<<31)
    {
      List<FieldDesc> fieldList;
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("color"),&dummyColor::m_Color));
      TupleType const* type = TupleTypeStruct::Create(fieldList);
      s_LineColor = OGLSemanticManager::RegisterUniformData("LineColor",type);
    }


    if(s_LineTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER,defaultVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,linePS);
      
      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);


      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

      OGLProgramInterface sprTechDesc;
      sprTechDesc.AddAttrib(s_PosAttrib);
      sprTechDesc.AddTexture(s_DiffuseTexture);
      sprTechDesc.AddUniform(s_CameraUnif);
      sprTechDesc.AddUniform(s_WorldMatUnif);
      sprTechDesc.AddUniform(s_LineColor);

      s_LineTech = sprTechDesc.Compile(defaultProgram);

    }
  }

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

}
