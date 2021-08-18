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

namespace eXl
{
  class OGLProgram;
  class OGLCompiledTechnique;

  struct CameraMatrix
  {
    EXL_REFLECT;

    Matrix4f viewMatrix;
    Matrix4f viewInverseMatrix;
    Matrix4f projMatrix;
  };

  class EXL_OGL_API OGLBaseAlgo
  {
  public:

    static void Init();

    static unsigned int GetPosAttrib();

    static unsigned int GetTexCoordAttrib();

    static unsigned int GetWorldMatUniform();

    static unsigned int GetCameraUniform();

    static unsigned int GetDiffuseTexture();
  };

  class EXL_OGL_API OGLLineAlgo
  {
  public:

    static void Init();

    static OGLCompiledTechnique const* GetTechnique();

    static unsigned int GetColor();
  };

  struct SpriteColor
  {
    EXL_REFLECT;

    Vector4f tint;
		Vector2f tcOffset = Vector2f::ZERO;
		Vector2f tcScaling = Vector2f::ONE;
    Vector2f texSize = Vector2f::ONE;
    float alphaMult = 1.0;
  };

  class EXL_OGL_API OGLSpriteAlgo
  {
  public:

    static void Init();

    static void ShutdownAPI();

    static OGLCompiledTechnique const* GetSpriteTechnique(bool iFiltered = true);

    static OGLCompiledTechnique const* GetFontTechnique();

    static unsigned int GetSpriteColorUniform();

    static unsigned int GetUnfilteredTexture();
  };


  struct LightInfo
  {
    //Vector3f m_Position;
    Vector3f m_Direction;
    Vector3f m_Color;
  };

  struct MaterialInfo
  {
    Vector4f m_BRDFParameters;
    Vector3f m_DiffuseColor;
  };

  class EXL_OGL_API OGLMeshAlgo
  {
  public:

    static void Init();

    static void ShutdownAPI();

    static unsigned int GetNormalAttrib();

    static OGLCompiledTechnique const* GetMeshTechnique();
    static OGLCompiledTechnique const* GetMeshNormalTechnique();

    static unsigned int GetLightInfo();

    static unsigned int GetSpecularMap();

    static unsigned int GetEnvBrdfLUT();

    static unsigned int GetIrradianceMap();

    static unsigned int GetMaterialInfo();

  };

  class EXL_OGL_API OGLSkyAlgo
  {
  public:

    static void Init();

    static void ShutdownAPI();

    static unsigned int GetSkyTexture();

    static OGLCompiledTechnique const* GetSkyTechnique();

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

    static unsigned int GetAlgoInfo();

    static OGLCompiledTechnique const* GetIrradianceMapTechnique();
    static OGLCompiledTechnique const* GetSpecularMapTechnique();
    static OGLCompiledTechnique const* GetEnvBrdfTechnique();

  };
}
