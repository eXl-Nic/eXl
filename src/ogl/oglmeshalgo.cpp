/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/oglmeshalgo.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/oglutils.hpp>

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

  void OGLMeshAlgo::Init(OGLSemanticManager& iManager)
  {
    iManager.RegisterAttribute(GetNormalAttrib() ,OGLType::FLOAT32,3);
   
    OGLSamplerDesc samplerDesc;
    samplerDesc.samplerType = OGLTextureType::TEXTURE_CUBE_MAP;
    samplerDesc.maxFilter = OGLMagFilter::LINEAR;
    samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetIrradianceMap(), samplerDesc);
  
    samplerDesc.samplerType = OGLTextureType::TEXTURE_CUBE_MAP;
    samplerDesc.maxFilter = OGLMagFilter::LINEAR;
    samplerDesc.minFilter = OGLMinFilter::LINEAR_MIPMAP_LINEAR;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetSpecularMap(), samplerDesc);

    samplerDesc.samplerType = OGLTextureType::TEXTURE_2D;
    samplerDesc.maxFilter = OGLMagFilter::LINEAR;
    samplerDesc.minFilter = OGLMinFilter::LINEAR;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetEnvBrdfLUT(), samplerDesc);

    List<FieldDesc> fieldList;
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iLightDir"),&LightInfo::m_Direction));
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iLightColor"),&LightInfo::m_Color));

    TupleType const* type = TupleTypeStruct::Create(fieldList);

    iManager.RegisterUniformData("LightInfo", type);

    fieldList.clear();

    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iDiffuseColor"),&MeshMaterialInfo::m_DiffuseColor));
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iBRDFParameters"),&MeshMaterialInfo::m_BRDFParameters));

    type = TupleTypeStruct::Create(fieldList);

    iManager.RegisterUniformData("MaterialInfo",type);
  }

  TextureName OGLMeshAlgo::GetIrradianceMap()
  {
    static TextureName s_Name("iIrradianceMap");
    return s_Name;
  }

  TextureName OGLMeshAlgo::GetSpecularMap()
  {
    static TextureName s_Name("iSpecularEnvMap");
    return s_Name;
  }

  TextureName OGLMeshAlgo::GetEnvBrdfLUT()
  {
    static TextureName s_Name("iEnvBrdfLUT");
    return s_Name;
  }

  AttributeName OGLMeshAlgo::GetNormalAttrib()
  {
    static AttributeName s_Name("iNormal");
    return s_Name;
  }

  OGLCompiledProgram const* OGLMeshAlgo::CreateMeshProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, meshVS);
    GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, meshPS);

    GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader, defaultFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(defaultFShader);

    OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

    OGLProgramInterface meshTechDesc;
    meshTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
    meshTechDesc.AddAttrib(GetNormalAttrib());
    meshTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
    meshTechDesc.AddTexture(OGLBaseAlgo::GetDiffuseTexture());
    meshTechDesc.AddTexture(GetIrradianceMap());
    meshTechDesc.AddTexture(GetSpecularMap());
    meshTechDesc.AddTexture(GetEnvBrdfLUT());
    meshTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
    meshTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());
    meshTechDesc.AddUniform(GetLightInfo());
    meshTechDesc.AddUniform(GetMaterialInfo());

    return meshTechDesc.Compile(iManager, defaultProgram);
  }

  OGLCompiledProgram const* OGLMeshAlgo::CreateMeshNormalProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, meshVS);
    GLuint normalFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, meshNormalPS);

    GLuint normalProgramId = OGLUtils::LinkProgram(defaultVShader, normalFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(normalFShader);

    OGLProgram* normalProgram = eXl_NEW OGLProgram(normalProgramId);

    OGLProgramInterface meshTechDesc;
    meshTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
    meshTechDesc.AddAttrib(GetNormalAttrib());
    //meshTechDesc.AddAttrib(OGLBaseAlgo::GetTexCoordAttrib());
    //meshTechDesc.AddTexture(OGLBaseAlgo::GetDiffuseTexture());
    meshTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());
    meshTechDesc.AddUniform(OGLBaseAlgo::GetWorldMatUniform());

    return meshTechDesc.Compile(iManager, normalProgram);
  }

  UniformName OGLMeshAlgo::GetLightInfo()
  {
    static UniformName s_Name("LightInfo");
    return s_Name;
  }

  UniformName OGLMeshAlgo::GetMaterialInfo()
  {
    static UniformName s_Name("MaterialInfo");
    return s_Name;
  }

  void OGLSkyAlgo::Init(OGLSemanticManager& iManager)
  {
    OGLSamplerDesc samplerDesc;
    samplerDesc.samplerType = OGLTextureType::TEXTURE_CUBE_MAP;
    samplerDesc.maxFilter = OGLMagFilter::LINEAR;
    samplerDesc.minFilter = OGLMinFilter::LINEAR;
    samplerDesc.wrapX = OGLWrapMode::REPEAT;
    samplerDesc.wrapY = OGLWrapMode::REPEAT;

    iManager.RegisterTexture(GetSkyTexture(), samplerDesc);

  }

  TextureName OGLSkyAlgo::GetSkyTexture()
  {
    static TextureName s_Name("iSkyBox");
    return s_Name;
  }

  OGLCompiledProgram const* OGLSkyAlgo::CreateSkyProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, skyBoxVS);
    GLuint defaultFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, skyBoxPS);

    GLuint defaultProgramId = OGLUtils::LinkProgram(defaultVShader, defaultFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(defaultFShader);

    OGLProgram* defaultProgram = eXl_NEW OGLProgram(defaultProgramId);

    OGLProgramInterface skyTechDesc;
    skyTechDesc.AddAttrib(OGLBaseAlgo::GetPosAttrib());
    skyTechDesc.AddAttrib(OGLMeshAlgo::GetNormalAttrib());
    skyTechDesc.AddTexture(GetSkyTexture());
    skyTechDesc.AddUniform(OGLBaseAlgo::GetCameraUniform());

    return skyTechDesc.Compile(iManager, defaultProgram);
  }

  void OGLIrradianceMapAlgo::Init(OGLSemanticManager& iManager)
  {
    List<FieldDesc> fieldList;
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iPhiComputationRange"),&AlgoData::phiComputationRange));
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iThetaComputationRange"),&AlgoData::thetaComputationRange));
    fieldList.push_back(FieldDesc::MakeField(TypeFieldName("iCubeFace"),&AlgoData::face));

    TupleType const* type = TupleTypeStruct::Create(fieldList);

    iManager.RegisterUniformData(GetAlgoInfo() ,type);
  }

  UniformName OGLIrradianceMapAlgo::GetAlgoInfo()
  {
    static UniformName s_Name("IrradianceAlgoData");
    return s_Name;
  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::CreateIrradianceMapProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, screenQuadVS);
    GLuint irrMapFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, irradianceMapPS);

    GLuint irrMapProgramId = OGLUtils::LinkProgram(defaultVShader, irrMapFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(irrMapFShader);

    OGLProgram* irrMapProgram = eXl_NEW OGLProgram(irrMapProgramId);

    OGLProgramInterface irradianceTechDesc;
    irradianceTechDesc.AddTexture(OGLSkyAlgo::GetSkyTexture());
    irradianceTechDesc.AddUniform(GetAlgoInfo());

    return irradianceTechDesc.Compile(iManager, irrMapProgram);
  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::CreateSpecularMapProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, screenQuadVS);
    GLuint specularFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, specularMapPS);

    GLuint specularProgramId = OGLUtils::LinkProgram(defaultVShader, specularFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(specularFShader);

    OGLProgram* specularProgram = eXl_NEW OGLProgram(specularProgramId);

    OGLProgramInterface irradianceTechDesc;
    irradianceTechDesc.AddTexture(OGLSkyAlgo::GetSkyTexture());
    irradianceTechDesc.AddUniform(GetAlgoInfo());

    return irradianceTechDesc.Compile(iManager, specularProgram);
  }

  OGLCompiledProgram const* OGLIrradianceMapAlgo::CreateEnvBrdfProgram(OGLSemanticManager& iManager)
  {
    GLuint defaultVShader = OGLUtils::CompileShader(GL_VERTEX_SHADER, screenQuadVS);
    GLuint envBrdfFShader = OGLUtils::CompileShader(GL_FRAGMENT_SHADER, envBrdfMapPS);

    GLuint envBProgramId = OGLUtils::LinkProgram(defaultVShader, envBrdfFShader);

    glDeleteShader(defaultVShader);
    glDeleteShader(envBrdfFShader);

    OGLProgram* envBProgram = eXl_NEW OGLProgram(envBProgramId);

    OGLProgramInterface irradianceTechDesc;
    irradianceTechDesc.AddTexture(OGLSkyAlgo::GetSkyTexture());
    irradianceTechDesc.AddUniform(GetAlgoInfo());

    return irradianceTechDesc.Compile(iManager, envBProgram);
  }

}
