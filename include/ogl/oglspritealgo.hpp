/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <math/matrix4.hpp>
#include <math/vector4.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>

namespace eXl
{
  class OGLProgram;
  class OGLCompiledProgram;

  struct EXL_OGL_API CameraMatrix
  {
    EXL_REFLECT;

    Matrix4f viewMatrix;
    Matrix4f viewInverseMatrix;
    Matrix4f projMatrix;
  };

  class EXL_OGL_API OGLBaseAlgo
  {
  public:

    static void Init(OGLSemanticManager& iManager);

    static AttributeName GetPosAttrib();
    static AttributeName GetTexCoordAttrib();

    static UniformName GetWorldMatUniform();
    static UniformName GetCameraUniform();
    static TextureName GetDiffuseTexture();
  };

  class EXL_OGL_API OGLLineAlgo
  {
  public:

    static void Init(OGLSemanticManager& iManager);

    static OGLCompiledProgram const* CreateProgram(OGLSemanticManager& iSemantics);

    static UniformName GetColor();
  };

  struct EXL_OGL_API SpriteColor
  {
    EXL_REFLECT;

    Vector4f tint;
		Vector2f tcOffset = Vector2f::ZERO;
		Vector2f tcScaling = Vector2f::ONE;
    Vector2f texSize = Vector2f::ONE;
    Vector2f imageSize;
    float alphaMult = 1.0;
  };

  class EXL_OGL_API OGLSpriteAlgo
  {
  public:

    static void Init(OGLSemanticManager& iManager);

    static void ShutdownAPI();

    static OGLCompiledProgram const* CreateSpriteProgram(OGLSemanticManager& iSemantics, bool iFiltered = true);

    static OGLCompiledProgram const* CreateFontProgram(OGLSemanticManager& iSemantics);

    static UniformName GetSpriteColorUniform();

    static TextureName GetUnfilteredTexture();
  };

#if 0
  struct LightInfo
  {
    //Vector3f m_Position;
    Vector3f m_Direction;
    Vector3f m_Color;
  };

  struct MeshMaterialInfo
  {
    Vector4f m_BRDFParameters;
    Vector3f m_DiffuseColor;
  };

  class EXL_OGL_API OGLMeshAlgo
  {
  public:

    static void Init();

    static void ShutdownAPI();

    static uint32_t GetNormalAttrib();

    static OGLCompiledProgram const* GetMeshProgram();
    static OGLCompiledProgram const* GetMeshNormalProgram();

    static uint32_t GetLightInfo();

    static uint32_t GetSpecularMap();

    static uint32_t GetEnvBrdfLUT();

    static uint32_t GetIrradianceMap();

    static uint32_t GetMaterialInfo();

  };

  class EXL_OGL_API OGLSkyAlgo
  {
  public:

    static void Init();

    static void ShutdownAPI();

    static uint32_t GetSkyTexture();

    static OGLCompiledProgram const* GetSkyProgram();

  };

  class EXL_OGL_API OGLIrradianceMapAlgo
  {
  public:

    struct AlgoData
    {
      inline AlgoData()
        :phiComputationRange(0.0, 2.0 * Mathf::PI, 0.025)
        ,thetaComputationRange(0.0, Mathf::PI / 2.0, 0.05)
      {

      }

      Vector3f phiComputationRange;
      Vector3f thetaComputationRange;
      int face;
    };

    static void Init();

    static void ShutdownAPI();

    static uint32_t GetAlgoInfo();

    static OGLCompiledProgram const* GetIrradianceMapProgram();
    static OGLCompiledProgram const* GetSpecularMapProgram();
    static OGLCompiledProgram const* GetEnvBrdfProgram();

  };
#endif
}
