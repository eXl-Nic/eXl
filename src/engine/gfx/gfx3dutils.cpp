
#include <engine/gfx/gfx3dutils.hpp>

#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/oglrendercontext.hpp>

#include <ogl/oglutils.hpp>
#include <ogl/oglmeshalgo.hpp>


#include <core/image/imagestreamer.hpp>
#include <core/image/image.hpp>
#include <ogl/renderer/oglframebuffer.hpp>

namespace eXl
{

  IBLCompute::IBLCompute(OGLSemanticManager& iManager)
    : m_IrradianceMapProgram(OGLIrradianceMapAlgo::CreateIrradianceMapProgram(iManager))
    , m_SpecularMapProgram(OGLIrradianceMapAlgo::CreateSpecularMapProgram(iManager))
    , m_EnvBRDFProgram(OGLIrradianceMapAlgo::CreateEnvBrdfProgram(iManager))
  {

  }

  IntrusivePtr<OGLTexture> IBLCompute::MakeIrradianceCubemap(OGLSemanticManager& iManager, OGLTexture* iCubeMap)
  {
    Image::Size size = iCubeMap->GetSize();
    unsigned int const sizePix = 3 * size.x * size.y;
    OGLTextureLoader texLoader;

    IntrusivePtr<OGLTexture> renderTexture(OGLTextureLoader::Create(size, OGLTextureLoader::RGB32F));

    OGLFramebuffer framebuffer(size);
    framebuffer.AddColorAttachement(renderTexture.get());
    framebuffer.AddDepthStencilAttachement(NULL);

    OGLRenderContext ctx(iManager);

    std::vector<Image*> images;

    for(unsigned int i = 0 ; i<6; ++i)
    {
      Image* finalImage = new Image(NULL, size, Image::RGB, Image::Float, 4);
      float* accumValue = reinterpret_cast<float*>(finalImage->GetPixel(0,0));
      for(unsigned int pix = 0; pix<sizePix; ++pix)
      {
        accumValue[pix] = 0.0;
      }
      unsigned int const numSteps = 100;
      float stepTheta = 0.01;
      float stepPhi = 0.005;
      unsigned int const totSample = ((2.0 * glm::pi<float>()) / stepPhi) * ((glm::pi<float>() / 2) / stepTheta);
      float range = 2.0*glm::pi<float>() / numSteps;
      for(unsigned int step = 0 ; step<numSteps; ++step)
      {
        OGLIrradianceMapAlgo::AlgoData irrConfData;
        irrConfData.phiComputationRange = Vec3(range * step, range * (step + 1), stepPhi);
        irrConfData.thetaComputationRange.z = stepTheta;
        irrConfData.face = i;

        OGLShaderData skyData;
        skyData.AddTexture(OGLSkyAlgo::GetSkyTexture(), iCubeMap);
        skyData.AddData(OGLIrradianceMapAlgo::GetAlgoInfo(), &irrConfData);

        OGLDisplayList tempList(iManager);

        tempList.SetDefaultViewport(Zero<Vec2i>(), Vec2i(size.x, size.y));
        tempList.SetDefaultDepth(true,true);
        tempList.SetDefaultScissor(Vec2i(0,0),Vec2i(-1,-1));
        tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

        tempList.InitForPush();

        tempList.Clear(0,true,true);

        OGLVAssembly emptyAss;
        emptyAss.m_IBuffer = NULL;
        emptyAss.m_IOffset = 0;

        tempList.PushData(&skyData);
        tempList.SetVAssembly(&emptyAss);
        tempList.SetProgram(m_IrradianceMapProgram);
        tempList.PushDraw(1, OGLDraw::TriangleStrip, 4, 0, 0);
        tempList.PopData();

        tempList.Render(&ctx, &framebuffer);

        Image* retImg = NULL;
        Err res = texLoader.ReadTexture(renderTexture.get(), retImg);
        if(res)
        {
          float* resValue = reinterpret_cast<float*>(retImg->GetPixel(0,0));
          for(unsigned int pix = 0; pix<sizePix; ++pix)
          {
            accumValue[pix] += resValue[pix];
          }
          eXl_DELETE retImg;
        }
      }

      Image* classicImage = new Image(NULL, size, Image::RGB, Image::Char, 4);
      unsigned char* basePtr = reinterpret_cast<unsigned char*>(classicImage->GetPixel(0,0));
      for(unsigned int y = 0; y<size.y; ++y)
      {
        unsigned char* pixPtr = basePtr + classicImage->GetRowStride() * y;
        for(unsigned int x = 0; x<size.x; ++x)
        {
          pixPtr[0] = Mathf::Clamp(255.0 * (*accumValue * glm::pi<float>() / totSample), 0, 255);
          ++accumValue;
          pixPtr[1] = Mathf::Clamp(255.0 * (*accumValue * glm::pi<float>() / totSample), 0, 255);
          ++accumValue;
          pixPtr[2] = Mathf::Clamp(255.0 * (*accumValue * glm::pi<float>() / totSample), 0, 255);
          ++accumValue;
          pixPtr += 3;
        }
      }
      eXl_DELETE finalImage;
      images.push_back(classicImage);
      char buffer[512];
      sprintf(buffer, "D:\\CubeMap\\Irr_%i.png", i);
      ImageStreamer::Save(classicImage, buffer);
    }
    return IntrusivePtr<OGLTexture>(OGLTextureLoader::CreateCubeMap(images.data(), true));
  }

  void IBLCompute::MakeSpecularMipmap(OGLSemanticManager& iManager, OGLTexture* iCubeMap)
  {
    Image::Size size = iCubeMap->GetSize();

    unsigned int numLod = log2(Mathi::Min(size.x, size.y)) + 1;
    iCubeMap->SetTextureNumLOD(numLod);

    float roughnessStep = 1.0 / float(numLod);
    //float thetaStep = Mathf::ASin(roughnessStep);
    unsigned int curLod = 0;
    ++curLod;
    size = size / 2;
    while(size.x * size.y > 0)
    {
      unsigned int const sizePix = 3 * size.x * size.y;

      IntrusivePtr<OGLTexture> renderTexture(OGLTextureLoader::Create(size, OGLTextureLoader::RGB32F));

      OGLFramebuffer framebuffer(size);
      framebuffer.AddColorAttachement(renderTexture.get());
      framebuffer.AddDepthStencilAttachement(NULL);

      OGLRenderContext ctx(iManager);

      std::vector<Image*> images;

      for(unsigned int i = 0 ; i<6; ++i)
      {
        Image* finalImage = new Image(NULL, size, Image::RGB, Image::Float, 4);
        float* accumValue = reinterpret_cast<float*>(finalImage->GetPixel(0,0));
        for(unsigned int pix = 0; pix<sizePix; ++pix)
        {
          accumValue[pix] = 0.0;
        }

        unsigned int const numSteps = 10;
        //float stepTheta = 0.05 * (roughnessStep * curLod);
        //float stepPhi = 0.025;
        //float range = 2.0*glm::pi<float>() / numSteps;
        //float rangeTheta = Mathf::ASin(roughnessStep * curLod);
        //unsigned int const totSample = ((2.0 * glm::pi<float>()) / stepPhi) * (rangeTheta / stepTheta);
        //for(unsigned int step = 0 ; step<numSteps; ++step)
        {
          OGLIrradianceMapAlgo::AlgoData irrConfData;

          irrConfData.phiComputationRange.x = curLod * roughnessStep;
          irrConfData.face = i;

          OGLShaderData skyData;
          skyData.AddTexture(OGLSkyAlgo::GetSkyTexture(), iCubeMap);
          skyData.AddData(OGLIrradianceMapAlgo::GetAlgoInfo(), &irrConfData);

          OGLDisplayList tempList(iManager);

          tempList.SetDefaultViewport(Zero<Vec2i>(), Vec2i(size.x, size.y));
          tempList.SetDefaultDepth(false, false);
          tempList.SetDefaultScissor(Vec2i(0,0),Vec2i(-1,-1));
          tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

          tempList.InitForPush();

          tempList.Clear(0,true,true);

          OGLVAssembly emptyAss;
          emptyAss.m_IBuffer = NULL;
          emptyAss.m_IOffset = 0;

          tempList.PushData(&skyData);
          tempList.SetVAssembly(&emptyAss);
          tempList.SetProgram(m_SpecularMapProgram);
          tempList.PushDraw(1, OGLDraw::TriangleStrip, 4, 0, 0);
          tempList.PopData();

          tempList.Render(&ctx, &framebuffer);

          Image* retImg = NULL;
          Err res = OGLTextureLoader::ReadTexture(renderTexture.get(), retImg);
          if(res)
          {
            float* resValue = reinterpret_cast<float*>(retImg->GetPixel(0,0));
            for(unsigned int pix = 0; pix<sizePix; ++pix)
            {
              accumValue[pix] += resValue[pix];
            }
            eXl_DELETE retImg;
          }
        }

        Image* classicImage = new Image(NULL, size, Image::RGB, Image::Char, 1);
        unsigned char* basePtr = reinterpret_cast<unsigned char*>(classicImage->GetPixel(0,0));
        for(unsigned int y = 0; y<size.y; ++y)
        {
          unsigned char* pixPtr = basePtr + classicImage->GetRowStride() * (size.y - 1 - y);
          for(unsigned int x = 0; x<size.x; ++x)
          {
            pixPtr[0] = Mathf::Clamp(255.0 * (*accumValue), 0, 255);
            ++accumValue;
            pixPtr[1] = Mathf::Clamp(255.0 * (*accumValue), 0, 255);
            ++accumValue;
            pixPtr[2] = Mathf::Clamp(255.0 * (*accumValue), 0, 255);
            ++accumValue;
            pixPtr += 3;
          }
        }
        unsigned int faceConv[6] = {0,1,2,3,4,5};
        iCubeMap->Update(AABB2Di::FromMinAndSize(Zero<Vec2i>(), Vec2i(size.x, size.y)), 
          OGLTextureElementType::UNSIGNED_BYTE, OGLTextureFormat::RGB, basePtr, curLod, faceConv[i]);
        eXl_DELETE finalImage;
#if 0
        char buffer[512];
        sprintf(buffer, "D:\\CubeMap\\SpecMip_%i_%i.png", i, curLod);
        ImageStreamer::Save(classicImage, buffer);
#endif
        eXl_DELETE classicImage;

      }
      ++curLod;
      size = size / 2;
    }
    //return texLoader.CreateCubeMap(images.data(), true);
  }

  IntrusivePtr<OGLTexture> IBLCompute::MakeEnvBrdfMap(OGLSemanticManager& iManager, Vec2i iSize)
  {
    unsigned int const sizePix = 3 * iSize.x * iSize.y;

    Image::Size size(iSize.x, iSize.y);
    IntrusivePtr<OGLTexture> renderTexture(OGLTextureLoader::Create(size, OGLTextureLoader::RGB8));

    OGLFramebuffer framebuffer(size);
    framebuffer.AddColorAttachement(renderTexture.get());
    framebuffer.AddDepthStencilAttachement(NULL);

    OGLRenderContext ctx(iManager);

    OGLIrradianceMapAlgo::AlgoData irrConfData;

    OGLShaderData skyData;
    //skyData.AddTexture(OGLSkyAlgo::GetSkyTexture(), iCubeMap);
    skyData.AddData(OGLIrradianceMapAlgo::GetAlgoInfo(), &irrConfData);

    OGLDisplayList tempList(iManager);

    tempList.SetDefaultViewport(Zero<Vec2i>(), iSize);
    tempList.SetDefaultDepth(true,true);
    tempList.SetDefaultScissor(Vec2i(0,0),Vec2i(-1,-1));
    tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

    tempList.InitForPush();

    tempList.Clear(0,true,true);

    OGLVAssembly emptyAss;
    emptyAss.m_IBuffer = NULL;
    emptyAss.m_IOffset = 0;

    tempList.PushData(&skyData);
    tempList.SetVAssembly(&emptyAss);
    tempList.SetProgram(m_EnvBRDFProgram);
    tempList.PushDraw(1, OGLDraw::TriangleStrip, 4, 0, 0);
    tempList.PopData();

    tempList.Render(&ctx, &framebuffer);

    Image* retImg = NULL;
    Err res = OGLTextureLoader::ReadTexture(renderTexture.get(), retImg);

    ImageStreamer::Save(retImg, "D:\\cubeMap\\EnvBrdfLUT.png");
    eXl_DELETE retImg;
    
    return renderTexture;
  }

  void MakeBox(OGLVAssembly& oAssembly, Vec3 const& iSize)
  {
    float vtxData[24 * 8];
    for(unsigned int i = 0; i<6; ++i)
    {
      unsigned int dir = i/2;

      Vec3 planeNormal = Zero<Vec3>();
      planeNormal[dir] = i % 2 == 0 ? -1.0 : 1.0;

      unsigned int dir2 = (dir + 1)%3;
      unsigned int dir3 = (dir + 2)%3;
      Vec3 sideVec[2] = {Zero<Vec3>(), Zero<Vec3>()};
      sideVec[0][dir2] = 1.0;
      sideVec[1][dir3] = 1.0;
      Vec3 basePoint = planeNormal * iSize[dir] / 2.0;
      float* data = vtxData + i * 4 * 8;

      for(unsigned int j = 0; j<4; ++j)
      {
        float mult1 = (j % 2 == 0 ? -1 : 1) * iSize[dir2] / 2.0;
        float mult2 = (j / 2 == 0 ? -1 : 1) * iSize[dir3] / 2.0;
        *((Vec3*)data) = basePoint + (sideVec[0] * mult1 + sideVec[1] * mult2);
        data += 3;
        *((Vec3*)data) = planeNormal;
        data += 3;
        data[0] = 0.5 + mult1 * 0.5;
        data[1] = 0.5 + mult2 * 0.5;
        data += 2;
      }
    }

    unsigned int indexData[6*6];
    for(unsigned int i = 0; i<6; ++i)
    {
      unsigned int baseIdx = 4*i;
      unsigned int* idx = indexData + 6*i;
      idx[0] = baseIdx;
      idx[1] = baseIdx + 1;
      idx[2] = baseIdx + 2;
      idx[3] = baseIdx + 2;
      idx[4] = baseIdx + 1;
      idx[5] = baseIdx + 3;
    }

    OGLBuffer* boxVtx = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER,sizeof(vtxData),vtxData);
    OGLBuffer* boxIdx = OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER,sizeof(indexData),indexData);

    oAssembly.AddAttrib(boxVtx,OGLBaseAlgo::GetPosAttrib(),3,8*sizeof(float),0);
    oAssembly.AddAttrib(boxVtx,OGLMeshAlgo::GetNormalAttrib(),3,8*sizeof(float),3*sizeof(float));
    oAssembly.AddAttrib(boxVtx,OGLBaseAlgo::GetTexCoordAttrib(),2,8*sizeof(float),6*sizeof(float));
    oAssembly.m_IBuffer = boxIdx;
    oAssembly.m_IOffset = 0;
  }

  unsigned int MakeSphere(OGLVAssembly& oAssembly, float iRadius)
  {
    unsigned int subDivHemi = 30;
    unsigned int subDivHeight = subDivHemi / 2;

    float stepPhi = 2*glm::pi<float>() / subDivHemi;
    float stepTheta = glm::pi<float>() / subDivHeight;

    Vector<float> vtxData((subDivHemi * (subDivHeight - 1) + 2) * 6);
    Vector<unsigned int> idxData;
    Vec3* dataPtr = reinterpret_cast<Vec3*>(vtxData.data());
    dataPtr[0] = Vec3(0.0, iRadius, 0.0);
    dataPtr[1] = Vec3(0.0, 1.0, 0.0);
    dataPtr += 2;
    for(unsigned int j = 0; j<subDivHemi - 1; ++j)
    {
      idxData.push_back(0);
      idxData.push_back(1 + j);
      idxData.push_back(2 + j);
    }
    idxData.push_back(0);
    idxData.push_back(1 + subDivHemi - 1);
    idxData.push_back(1);
    for(unsigned int i = 1; i<subDivHeight; ++i)
    {
      float theta = stepTheta * (float(subDivHeight) / 2.0 - i);
      float cosTheta = Mathf::Cos(theta);
      float sinTheta = Mathf::Sin(theta);
      for(unsigned int j = 0; j<subDivHemi; ++j)
      {
        if(i != subDivHeight - 1)
        {
          if(j != subDivHemi - 1)
          {
            idxData.push_back(1 + (i - 1)*subDivHemi + j);
            idxData.push_back(1 + i*subDivHemi + j);
            idxData.push_back(2 + i*subDivHemi + j);
            idxData.push_back(1 + (i - 1)*subDivHemi + j);
            idxData.push_back(2 + i*subDivHemi + j);
            idxData.push_back(2 + (i - 1)*subDivHemi + j);
          }
          else
          {
            idxData.push_back(1 + (i - 1)*subDivHemi + j);
            idxData.push_back(1 + i*subDivHemi + j);
            idxData.push_back(1 + i*subDivHemi);
            idxData.push_back(1 + (i - 1)*subDivHemi + j);
            idxData.push_back(1 + i*subDivHemi);
            idxData.push_back(1 + (i - 1)*subDivHemi);
          }
        }

        float phi = j * stepPhi;
        float cosPhi = Mathf::Cos(phi);
        float sinPhi = Mathf::Sin(phi);
        dataPtr[1] = (UnitX<Vec3>() * cosPhi + UnitZ<Vec3>() * sinPhi) * cosTheta + UnitY<Vec3>() * sinTheta;
        dataPtr[1] = glm::normalize(dataPtr[1]);
        dataPtr[0] = iRadius * dataPtr[1];
        dataPtr += 2;
      }
    }

    dataPtr[0] = Vec3(0.0, -iRadius, 0.0);
    dataPtr[1] = Vec3(0.0, -1.0, 0.0);
    dataPtr += 2;
    for(unsigned int j = 0; j<subDivHemi - 1; ++j)
    {
      idxData.push_back(1 + (subDivHeight - 2) * subDivHemi + j);
      idxData.push_back(2 + (subDivHeight - 2) * subDivHemi + j);
      idxData.push_back(vtxData.size() / 6 - 1);
    }
    idxData.push_back(1 + (subDivHeight - 2) * subDivHemi + subDivHemi - 1);
    idxData.push_back(1 + (subDivHeight - 2) * subDivHemi);
    idxData.push_back(vtxData.size() / 6 - 1);

    OGLBuffer* boxVtx = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER,sizeof(float) * vtxData.size(),vtxData.data());
    OGLBuffer* boxIdx = OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER,idxData.size() * sizeof(unsigned int),idxData.data());

    oAssembly.AddAttrib(boxVtx,OGLBaseAlgo::GetPosAttrib(),3,6*sizeof(float),0);
    oAssembly.AddAttrib(boxVtx,OGLMeshAlgo::GetNormalAttrib(),3,6*sizeof(float),3*sizeof(float));
    //oAssembly.AddAttrib(boxVtx,OGLBaseAlgo::GetTexCoordAttrib(),2,6*sizeof(float),6*sizeof(float));
    oAssembly.m_IBuffer = boxIdx;
    oAssembly.m_IOffset = 0;

    return idxData.size();
  }

  void GetViewProjMat(Vec2 const& iTheta, Vec2 const& iOffset, float iZoom, float iScreenRatio,
    /*AABB2Di const& iSceneBox, */
    //Vec3 iSceneOrigin,
    Vec3 iSceneSize,
    Mat4& oProj, Mat4& oView)
  {
    oProj = Zero<Mat4>();
    oView = Identity<Mat4>();

    //Vec2i sizeI = iSceneBox.GetSize();
    //Vec2 size = Vec2(sizeI.x, sizeI.y);
    //Vec2i center = iSceneBox.GetCenter();

    //float r = sqrt(1.0 + size.Length()*size.Length()) / 2;
    float r = iZoom * iZoom * length(iSceneSize);

    float fov = glm::pi<float>() / 4.0;
    //float r = sqrt(3.0);
    float nearP = r *(1 / sin(fov) - 1);
    float farP = nearP + 2 * r;
    float usefulNear = nearP;
    //glFrustum(-ratio * a, ratio * a, -a, a, a, b);

    oProj[0][0] = 1.0 / iScreenRatio;
    oProj[1][1] = 1.0 ;
    oProj[2][2] = -1.0* (usefulNear + farP) / (farP - usefulNear);
    oProj[3][2] = -2.0 * usefulNear * farP / (farP - usefulNear);
    oProj[2][3] = -1.0;
    //oProj = oProj.Transpose();

    //Quaternion quat(UnitX<Vec3>(), iTheta.x);
    //quat = quat * (Quaternion(UnitY<Vec3>(), iTheta.y));
    //Vec3 basisX = quat.Rotate(Vec3::UNIT_X);
    //Vec3 basisY = quat.Rotate(Vec3::UNIT_Y);
    //Vec3 basisZ = quat.Rotate(Vec3::UNIT_Z);
    //
    //memcpy(oView.m_Data + 0, &basisX, sizeof(Vec3));
    //memcpy(oView.m_Data + 4, &basisY, sizeof(Vec3));
    //memcpy(oView.m_Data + 8, &basisZ, sizeof(Vec3));
    //
    //oView = oView.Transpose();
    //
    //oView.m_Data[12] += iOffset.x;
    //oView.m_Data[13] += iOffset.y;
    //oView.m_Data[14] += (-nearP - r/2 + 0.5) * iZoom;
    //
  }

  void GetCameraViewProjMat(Vec3 const (&iBasis)[3], Vec3 const& iPos, float iScreenRatio, Mat4& oProj, Mat4& oView, float fov, float displayedSize)
  {
    oProj = Zero<Mat4>();
    oView = Identity<Mat4>();

    float nearP = displayedSize * (1.0 / sin(fov) - 1);
    float farP = displayedSize * 1000;

    oProj[0][0] = 2 * nearP / (displayedSize * iScreenRatio);
    oProj[1][1] = 2 * nearP / displayedSize ;
    oProj[2][2] = -1.0* (nearP + farP) / (farP - nearP);
    oProj[3][2] = -2.0 * nearP * farP / (farP - nearP);
    oProj[2][3] = -1.0;

    oView[0] = Vec4(iBasis[0], 0);
    oView[1] = Vec4(iBasis[1], 0);
    oView[2] = Vec4(iBasis[2], 0);

    Vec3 transPos = oView[0] * (-iPos.x) + oView[1] * (-iPos.y) + oView[2] * (-iPos.z);
    oView[3] = Vec4(iPos, 1);

    oView = inverse(oView);
  }
}