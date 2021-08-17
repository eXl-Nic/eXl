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
