#pragma once

#include <math/math.hpp>

namespace eXl
{
  class OGLSemanticManager;
  class OGLCompiledProgram;
  struct OGLVAssembly;
  class OGLTexture;

  void MakeBox(OGLVAssembly& oAssembly, Vec3 const& iSize);

  unsigned int MakeSphere(OGLVAssembly& oAssembly, float iRadius);

  void GetViewProjMat(Vec2 const& iTheta, Vec2 const& iOffset, float iZoom, float iScreenRatio,
    Vec3 iSceneSize,
    Mat4& oProj, Mat4& oView);

  void GetCameraViewProjMat(Vec3 const (&iBasis)[3], Vec3 const& iPos, float iScreenRatio,
    Mat4& oProj, Mat4& oView,
    float fov = Mathd::Pi() / 4.0, float displayedSize = 100.0);

  class IBLCompute
  {
  public:
    IBLCompute(OGLSemanticManager& iManager);

    IntrusivePtr<OGLTexture> MakeIrradianceCubemap(OGLSemanticManager& iManager, OGLTexture* iCubeMap);
    void MakeSpecularMipmap(OGLSemanticManager& iManager, OGLTexture* iCubeMap);
    IntrusivePtr<OGLTexture> MakeEnvBrdfMap(OGLSemanticManager& iManager, Vec2i iSize);

  protected:
    OGLCompiledProgram const* m_IrradianceMapProgram;
    OGLCompiledProgram const* m_SpecularMapProgram;
    OGLCompiledProgram const* m_EnvBRDFProgram;

  };

}