#pragma once

#include <math/math.hpp>

namespace eXl
{
  class OGLSemanticManager;
  class OGLCompiledProgram;
  struct OGLVAssembly;
  class OGLTexture;

  void MakeBox(OGLVAssembly& oAssembly, Vector3f const& iSize);

  unsigned int MakeSphere(OGLVAssembly& oAssembly, float iRadius);

  void GetViewProjMat(Vector2f const& iTheta, Vector2f const& iOffset, float iZoom, float iScreenRatio,
    Vector3f iSceneSize,
    Matrix4f& oProj, Matrix4f& oView);

  void GetCameraViewProjMat(Vector3f const (&iBasis)[3], Vector3f const& iPos, float iScreenRatio, 
    Matrix4f& oProj, Matrix4f& oView, 
    float fov = Mathd::PI / 4.0, float displayedSize = 100.0);

  class IBLCompute
  {
  public:
    IBLCompute(OGLSemanticManager& iManager);

    IntrusivePtr<OGLTexture> MakeIrradianceCubemap(OGLSemanticManager& iManager, OGLTexture* iCubeMap);
    void MakeSpecularMipmap(OGLSemanticManager& iManager, OGLTexture* iCubeMap);
    IntrusivePtr<OGLTexture> MakeEnvBrdfMap(OGLSemanticManager& iManager, Vector2i iSize);

  protected:
    OGLCompiledProgram const* m_IrradianceMapProgram;
    OGLCompiledProgram const* m_SpecularMapProgram;
    OGLCompiledProgram const* m_EnvBRDFProgram;

  };
}