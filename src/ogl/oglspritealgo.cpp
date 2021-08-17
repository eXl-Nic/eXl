/**

  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ogl/oglspritealgo.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogltechnique.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/oglutils.hpp>

#include "ogldefaultVS.inl"
#include "ogldefaultPS.inl"
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
    unsigned int s_PosAttrib      = 1<<31;
    unsigned int s_TexAttrib      = 1<<31;
    unsigned int s_WorldMatUnif   = 1<<31;
    unsigned int s_CameraUnif     = 1<<31;
    unsigned int s_DiffuseTexture = 1<<31;
    unsigned int s_UnfilteredTexture = 1<<31;
    unsigned int s_SpriteColor    = 1<<31;
    unsigned int s_LineColor      = 1<<31;

    OGLCompiledTechnique const* s_SpriteTech = NULL;
    OGLCompiledTechnique const* s_USpriteTech = NULL;
    OGLCompiledTechnique const* s_FontTech = NULL;

    OGLCompiledTechnique const* s_LineTech = NULL;
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

      s_PosAttrib = OGLSemanticManager::RegisterAttribute("iPosition",OGLType::FLOAT32,3);
      s_TexAttrib = OGLSemanticManager::RegisterAttribute("iTexCoord",OGLType::FLOAT32,2);

      TupleType const* type = TupleType::DynamicCast(CameraMatrix::GetType());;
      
      s_CameraUnif = OGLSemanticManager::RegisterUniformData("Camera",type);

      List<FieldDesc> fieldList;
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("worldMatrix"),&dummyStruct::dummyMat));
      type = TupleTypeStruct::Create(fieldList);

      s_WorldMatUnif = OGLSemanticManager::RegisterUniformData("WorldMatrix",type);

      OGLSamplerDesc samplerDesc;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_DiffuseTexture = OGLSemanticManager::RegisterTexture("iDiffuseTexture",samplerDesc);
    }
  }

  unsigned int OGLBaseAlgo::GetPosAttrib()
  {
    return s_PosAttrib;
  }

  unsigned int OGLBaseAlgo::GetTexCoordAttrib()
  {
    return s_TexAttrib;
  }

  unsigned int OGLBaseAlgo::GetWorldMatUniform()
  {
    return s_WorldMatUnif;
  }

  unsigned int OGLBaseAlgo::GetCameraUniform()
  {
    return s_CameraUnif;
  }

  unsigned int OGLBaseAlgo::GetDiffuseTexture()
  {
    return s_DiffuseTexture;
  }

  OGLCompiledTechnique const* OGLSpriteAlgo::GetSpriteTechnique(bool iFiltered)
  {
    if(iFiltered)
      return s_SpriteTech;
    else
      return s_USpriteTech;
  }


  OGLCompiledTechnique const* OGLSpriteAlgo::GetFontTechnique()
  {
    return s_FontTech;
  }

  unsigned int OGLSpriteAlgo::GetSpriteColorUniform()
  {
    return s_SpriteColor;
  }

  unsigned int OGLSpriteAlgo::GetUnfilteredTexture()
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
      samplerDesc.maxFilter = OGLMagFilter::NEAREST;
      samplerDesc.minFilter = OGLMinFilter::NEAREST;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_UnfilteredTexture = OGLSemanticManager::RegisterTexture("iUnfilteredTexture",samplerDesc);
    }
    if(s_SpriteTech == NULL)
    {
      GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER,defaultVS);
      GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,defaultPS);
      GLuint unfilteredFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,defaultUPS);
      GLuint fontFShader    = OGLUtils::CompileShader(GL_FRAGMENT_SHADER,fontPS);

      GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader,defaultFShader);
      GLuint unfilteredProgramId = OGLUtils::LinkProgram(defaultVShader,unfilteredFShader);

      GLuint fontProgramId    = OGLUtils::LinkProgram(defaultVShader,fontFShader);

      glDeleteShader(defaultVShader);
      glDeleteShader(defaultFShader);
      glDeleteShader(fontFShader);

      OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);
      OGLProgram* unfilteredProgram = eXl_NEW OGLProgram(unfilteredProgramId);
      OGLProgram* fontProgram    = eXl_NEW OGLProgram(fontProgramId);

      {
        OGLTechnique sprTechDesc;
        sprTechDesc.AddAttrib(s_PosAttrib);
        sprTechDesc.AddAttrib(s_TexAttrib);
        sprTechDesc.AddTexture(s_DiffuseTexture);
        sprTechDesc.AddUniform(s_CameraUnif);
        sprTechDesc.AddUniform(s_WorldMatUnif);
        sprTechDesc.AddUniform(s_SpriteColor);

        s_SpriteTech = sprTechDesc.Compile(defaultProgram);
      }

      {
        OGLTechnique uSprTechDesc;
        uSprTechDesc.AddAttrib(s_PosAttrib);
        uSprTechDesc.AddAttrib(s_TexAttrib);
        uSprTechDesc.AddTexture(s_UnfilteredTexture);
        uSprTechDesc.AddUniform(s_CameraUnif);
        uSprTechDesc.AddUniform(s_WorldMatUnif);
        uSprTechDesc.AddUniform(s_SpriteColor);

        s_USpriteTech = uSprTechDesc.Compile(unfilteredProgram);
      }

      {
        OGLTechnique fontTechDesc;
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

  OGLCompiledTechnique const* OGLLineAlgo::GetTechnique()
  {
    return s_LineTech;
  }

  unsigned int OGLLineAlgo::GetColor()
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

      OGLTechnique sprTechDesc;
      sprTechDesc.AddAttrib(s_PosAttrib);
      sprTechDesc.AddTexture(s_DiffuseTexture);
      sprTechDesc.AddUniform(s_CameraUnif);
      sprTechDesc.AddUniform(s_WorldMatUnif);
      sprTechDesc.AddUniform(s_LineColor);

      s_LineTech = sprTechDesc.Compile(defaultProgram);

    }
  }

  unsigned int s_LightInfo      = 1<<31;
  unsigned int s_MaterialInfo   = 1<<31;
  unsigned int s_NormalAttrib   = 1<<31;
  unsigned int s_IrradianceMap  = 1<<31;
  unsigned int s_SpecularEnvMap = 1<<31;
  unsigned int s_EnvBrdfLut     = 1<<31;

  OGLCompiledTechnique const* s_MeshTech = NULL;
  OGLCompiledTechnique const* s_MeshNormalTech = NULL;

  void OGLMeshAlgo::Init()
  {
    if(s_NormalAttrib == 1<<31)
    {
      s_NormalAttrib = OGLSemanticManager::RegisterAttribute("iNormal",OGLType::FLOAT32,3);
    }

    if(s_IrradianceMap == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_IrradianceMap = OGLSemanticManager::RegisterTexture("iIrradianceMap", samplerDesc);
    }

    if(s_SpecularEnvMap == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
      samplerDesc.maxFilter = OGLMagFilter::LINEAR;
      samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
      samplerDesc.wrapX = OGLWrapMode::REPEAT;
      samplerDesc.wrapY = OGLWrapMode::REPEAT;

      s_SpecularEnvMap = OGLSemanticManager::RegisterTexture("iSpecularEnvMap", samplerDesc);
    }

    if(s_EnvBrdfLut == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
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

      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iDiffuseColor"),&MaterialInfo::m_DiffuseColor));
      fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iBRDFParameters"),&MaterialInfo::m_BRDFParameters));

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

      OGLTechnique meshTechDesc;
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

  unsigned int OGLMeshAlgo::GetIrradianceMap()
  {
    return s_IrradianceMap;
  }

  unsigned int OGLMeshAlgo::GetSpecularMap()
  {
    return s_SpecularEnvMap;
  }

  unsigned int OGLMeshAlgo::GetEnvBrdfLUT()
  {
    return s_EnvBrdfLut;
  }

  unsigned int OGLMeshAlgo::GetNormalAttrib()
  {
    return s_NormalAttrib;
  }

  OGLCompiledTechnique const* OGLMeshAlgo::GetMeshTechnique()
  {
    return s_MeshTech;
  }

  OGLCompiledTechnique const* OGLMeshAlgo::GetMeshNormalTechnique()
  {
    return s_MeshNormalTech;
  }

  unsigned int OGLMeshAlgo::GetLightInfo()
  {
    return s_LightInfo;
  }

  unsigned int OGLMeshAlgo::GetMaterialInfo()
  {
    return s_MaterialInfo;
  }

  unsigned int s_SkyTexture    = 1<<31;
  
  OGLCompiledTechnique const* s_SkyTech = NULL;

  void OGLSkyAlgo::Init()
  {

    if(s_SkyTexture == 1<<31)
    {
      OGLSamplerDesc samplerDesc;
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

      OGLTechnique skyTechDesc;
      skyTechDesc.AddAttrib(s_PosAttrib);
      skyTechDesc.AddAttrib(s_NormalAttrib);
      skyTechDesc.AddTexture(s_SkyTexture);
      skyTechDesc.AddUniform(s_CameraUnif);

      s_SkyTech = skyTechDesc.Compile(defaultProgram);
    }
  }

  unsigned int OGLSkyAlgo::GetSkyTexture()
  {
    return s_SkyTexture;
  }

  OGLCompiledTechnique const* OGLSkyAlgo::GetSkyTechnique()
  {
    return s_SkyTech;
  }

  unsigned int s_IrradianceAlgoInfo = 1<<31;

  OGLCompiledTechnique const* s_IrradianceMapTech = NULL;
  OGLCompiledTechnique const* s_SpecularMapTech = NULL;
  OGLCompiledTechnique const* s_EnvBrdfTech = NULL;

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

      OGLTechnique irradianceTechDesc;
      irradianceTechDesc.AddTexture(s_SkyTexture);
      irradianceTechDesc.AddUniform(s_IrradianceAlgoInfo);

      s_IrradianceMapTech = irradianceTechDesc.Compile(defaultProgram);
      s_SpecularMapTech = irradianceTechDesc.Compile(specularProgram);
      s_EnvBrdfTech = irradianceTechDesc.Compile(envBProgram);
    }
  }

  unsigned int OGLIrradianceMapAlgo::GetAlgoInfo()
  {
    return s_IrradianceAlgoInfo;
  }

  void OGLIrradianceMapAlgo::ShutdownAPI()
  {

  }

  OGLCompiledTechnique const* OGLIrradianceMapAlgo::GetIrradianceMapTechnique()
  {
    return s_IrradianceMapTech;
  }

  OGLCompiledTechnique const* OGLIrradianceMapAlgo::GetSpecularMapTechnique()
  {
    return s_SpecularMapTech;
  }

  OGLCompiledTechnique const* OGLIrradianceMapAlgo::GetEnvBrdfTechnique()
  {
    return s_EnvBrdfTech;
  }

}
