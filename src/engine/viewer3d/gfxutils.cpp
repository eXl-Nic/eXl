
#include "gfxutils.hpp"

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

  IntrusivePtr<OGLTexture> MakeIrradianceCubemap(OGLSemanticManager& iManager, OGLTexture* iCubeMap)
  {
    OGLCompiledProgram const* irrMapProg = OGLIrradianceMapAlgo::CreateIrradianceMapProgram(iManager);
    Image::Size size = iCubeMap->GetSize();
    unsigned int const sizePix = 3 * size.X() * size.Y();
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
      unsigned int const totSample = ((2.0 * Mathf::PI) / stepPhi) * ((Mathf::PI / 2) / stepTheta);
      float range = 2.0*Mathf::PI / numSteps;
      for(unsigned int step = 0 ; step<numSteps; ++step)
      {
        OGLIrradianceMapAlgo::AlgoData irrConfData;
        irrConfData.phiComputationRange = Vector3f(range * step, range * (step + 1), stepPhi);
        irrConfData.thetaComputationRange.Z() = stepTheta;
        irrConfData.face = i;

        OGLShaderData skyData;
        skyData.AddTexture(OGLSkyAlgo::GetSkyTexture(), iCubeMap);
        skyData.AddData(OGLIrradianceMapAlgo::GetAlgoInfo(), &irrConfData);

        OGLDisplayList tempList(iManager);

        tempList.SetDefaultViewport(Vector2i::ZERO, Vector2i(size.X(), size.Y()));
        tempList.SetDefaultDepth(true,true);
        tempList.SetDefaultScissor(Vector2i(0,0),Vector2i(-1,-1));
        tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

        tempList.InitForPush();

        tempList.Clear(0,true,true);

        OGLVAssembly emptyAss;
        emptyAss.m_IBuffer = NULL;
        emptyAss.m_IOffset = 0;

        tempList.PushData(&skyData);
        tempList.SetVAssembly(&emptyAss);
        tempList.SetProgram(irrMapProg);
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
      for(unsigned int y = 0; y<size.Y(); ++y)
      {
        unsigned char* pixPtr = basePtr + classicImage->GetRowStride() * y;
        for(unsigned int x = 0; x<size.X(); ++x)
        {
          pixPtr[0] = Mathf::Clamp(255.0 * (*accumValue * Mathf::PI / totSample), 0, 255);
          ++accumValue;
          pixPtr[1] = Mathf::Clamp(255.0 * (*accumValue * Mathf::PI / totSample), 0, 255);
          ++accumValue;
          pixPtr[2] = Mathf::Clamp(255.0 * (*accumValue * Mathf::PI / totSample), 0, 255);
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

  void MakeSpecularMipmap(OGLSemanticManager& iManager, OGLTexture* iCubeMap)
  {
    OGLCompiledProgram const* specMapProg = OGLIrradianceMapAlgo::CreateSpecularMapProgram(iManager);
    Image::Size size = iCubeMap->GetSize();

    unsigned int numLod = log2(Mathi::Min(size.X(), size.Y())) + 1;
    iCubeMap->SetTextureNumLOD(numLod);

    float roughnessStep = 1.0 / float(numLod);
    //float thetaStep = Mathf::ASin(roughnessStep);
    unsigned int curLod = 0;
    ++curLod;
    size = size / 2;
    while(size.X() * size.Y() > 0)
    {
      unsigned int const sizePix = 3 * size.X() * size.Y();

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
        //float range = 2.0*Mathf::PI / numSteps;
        //float rangeTheta = Mathf::ASin(roughnessStep * curLod);
        //unsigned int const totSample = ((2.0 * Mathf::PI) / stepPhi) * (rangeTheta / stepTheta);
        //for(unsigned int step = 0 ; step<numSteps; ++step)
        {
          OGLIrradianceMapAlgo::AlgoData irrConfData;

          irrConfData.phiComputationRange.X() = curLod * roughnessStep;
          irrConfData.face = i;

          OGLShaderData skyData;
          skyData.AddTexture(OGLSkyAlgo::GetSkyTexture(), iCubeMap);
          skyData.AddData(OGLIrradianceMapAlgo::GetAlgoInfo(), &irrConfData);

          OGLDisplayList tempList(iManager);

          tempList.SetDefaultViewport(Vector2i::ZERO, Vector2i(size.X(), size.Y()));
          tempList.SetDefaultDepth(true,true);
          tempList.SetDefaultScissor(Vector2i(0,0),Vector2i(-1,-1));
          tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

          tempList.InitForPush();

          tempList.Clear(0,true,true);

          OGLVAssembly emptyAss;
          emptyAss.m_IBuffer = NULL;
          emptyAss.m_IOffset = 0;

          tempList.PushData(&skyData);
          tempList.SetVAssembly(&emptyAss);
          tempList.SetProgram(specMapProg);
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
        for(unsigned int y = 0; y<size.Y(); ++y)
        {
          unsigned char* pixPtr = basePtr + classicImage->GetRowStride() * (size.Y() - 1 - y);
          for(unsigned int x = 0; x<size.X(); ++x)
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
        iCubeMap->Update(AABB2Di(Vector2i::ZERO, Vector2i(size.X(), size.Y())), 
          OGLTextureElementType::UNSIGNED_BYTE, OGLTextureFormat::RGB, basePtr, curLod, faceConv[i]);
        eXl_DELETE finalImage;
        char buffer[512];
        sprintf(buffer, "D:\\CubeMap\\SpecMip_%i_%i.png", i, curLod);
        ImageStreamer::Save(classicImage, buffer);
        eXl_DELETE classicImage;
      }
      ++curLod;
      size = size / 2;
    }
    //return texLoader.CreateCubeMap(images.data(), true);
  }

  IntrusivePtr<OGLTexture> MakeEnvBrdfMap(OGLSemanticManager& iManager, Vector2i iSize)
  {
    OGLCompiledProgram const* envBrdfProg = OGLIrradianceMapAlgo::CreateEnvBrdfProgram(iManager);
    unsigned int const sizePix = 3 * iSize.X() * iSize.Y();

    Image::Size size(iSize.X(), iSize.Y());
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

    tempList.SetDefaultViewport(Vector2i::ZERO, iSize);
    tempList.SetDefaultDepth(true,true);
    tempList.SetDefaultScissor(Vector2i(0,0),Vector2i(-1,-1));
    tempList.SetDefaultBlend(false, OGLBlend::ONE, OGLBlend::ZERO);

    tempList.InitForPush();

    tempList.Clear(0,true,true);

    OGLVAssembly emptyAss;
    emptyAss.m_IBuffer = NULL;
    emptyAss.m_IOffset = 0;

    tempList.PushData(&skyData);
    tempList.SetVAssembly(&emptyAss);
    tempList.SetProgram(envBrdfProg);
    tempList.PushDraw(1, OGLDraw::TriangleStrip, 4, 0, 0);
    tempList.PopData();

    tempList.Render(&ctx, &framebuffer);

    Image* retImg = NULL;
    Err res = OGLTextureLoader::ReadTexture(renderTexture.get(), retImg);

    ImageStreamer::Save(retImg, "D:\\cubeMap\\EnvBrdfLUT.png");
    eXl_DELETE retImg;
    
    return renderTexture;
  }

  void MakeBox(OGLVAssembly& oAssembly, Vector3f const& iSize)
  {
    float vtxData[24 * 8];
    for(unsigned int i = 0; i<6; ++i)
    {
      unsigned int dir = i/2;

      Vector3f planeNormal;
      planeNormal.m_Data[dir] = i % 2 == 0 ? -1.0 : 1.0;

      unsigned int dir2 = (dir + 1)%3;
      unsigned int dir3 = (dir + 2)%3;
      Vector3f sideVec[2];
      sideVec[0].m_Data[dir2] = 1.0;
      sideVec[1].m_Data[dir3] = 1.0;
      Vector3f basePoint = planeNormal * iSize.m_Data[dir] / 2.0;
      float* data = vtxData + i * 4 * 8;

      for(unsigned int j = 0; j<4; ++j)
      {
        float mult1 = (j % 2 == 0 ? -1 : 1) * iSize.m_Data[dir2] / 2.0;
        float mult2 = (j / 2 == 0 ? -1 : 1) * iSize.m_Data[dir3] / 2.0;
        *((Vector3f*)data) = basePoint + (sideVec[0] * mult1 + sideVec[1] * mult2);
        data += 3;
        *((Vector3f*)data) = planeNormal;
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

    float stepPhi = 2*Mathf::PI / subDivHemi;
    float stepTheta = Mathf::PI / subDivHeight;

    Vector<float> vtxData((subDivHemi * (subDivHeight - 1) + 2) * 6);
    Vector<unsigned int> idxData;
    Vector3f* dataPtr = reinterpret_cast<Vector3f*>(vtxData.data());
    dataPtr[0] = Vector3f(0.0, iRadius, 0.0);
    dataPtr[1] = Vector3f(0.0, 1.0, 0.0);
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
        dataPtr[1] = (Vector3f::UNIT_X * cosPhi + Vector3f::UNIT_Z * sinPhi) * cosTheta + Vector3f::UNIT_Y * sinTheta;
        dataPtr[1].Normalize();
        dataPtr[0] = iRadius * dataPtr[1];
        dataPtr += 2;
      }
    }

    dataPtr[0] = Vector3f(0.0, -iRadius, 0.0);
    dataPtr[1] = Vector3f(0.0, -1.0, 0.0);
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

  void GetViewProjMat(Vector2f const& iTheta, Vector2f const& iOffset, float iZoom, float iScreenRatio,
    /*AABB2Di const& iSceneBox, */
    //Vector3f iSceneOrigin,
    Vector3f iSceneSize,
    Matrix4f& oProj, Matrix4f& oView)
  {
    oProj.MakeZero();
    oView.MakeIdentity();

    //Vector2i sizeI = iSceneBox.GetSize();
    //Vector2f size = Vector2f(sizeI.X(), sizeI.Y());
    //Vector2i center = iSceneBox.GetCenter();

    //float r = sqrt(1.0 + size.Length()*size.Length()) / 2;
    float r = iZoom * iZoom * iSceneSize.Length();

    float fov = Mathd::PI / 4.0;
    //float r = sqrt(3.0);
    float nearP = r *(1 / sin(fov) - 1);
    float farP = nearP + 2 * r;
    float usefulNear = nearP;
    //glFrustum(-ratio * a, ratio * a, -a, a, a, b);

    oProj.m_Data[0] = 1.0 / iScreenRatio;
    oProj.m_Data[5] = 1.0 ;
    oProj.m_Data[10] = -1.0* (usefulNear + farP) / (farP - usefulNear);
    oProj.m_Data[14] = -2.0 * usefulNear * farP / (farP - usefulNear);
    oProj.m_Data[11] = -1.0;
    //oProj = oProj.Transpose();

    Quaternionf quat(Vector3f::UNIT_X, iTheta.X());
    quat = quat * (Quaternionf(Vector3f::UNIT_Y, iTheta.Y()));
    Vector3f basisX = quat.Rotate(Vector3f::UNIT_X);
    Vector3f basisY = quat.Rotate(Vector3f::UNIT_Y);
    Vector3f basisZ = quat.Rotate(Vector3f::UNIT_Z);

    memcpy(oView.m_Data + 0, &basisX, sizeof(Vector3f));
    memcpy(oView.m_Data + 4, &basisY, sizeof(Vector3f));
    memcpy(oView.m_Data + 8, &basisZ, sizeof(Vector3f));

    oView = oView.Transpose();

    oView.m_Data[12] += iOffset.X();
    oView.m_Data[13] += iOffset.Y();
    oView.m_Data[14] += (-nearP - r/2 + 0.5) * iZoom;
    //
  }

  void GetCameraViewProjMat(Vector3f const (&iBasis)[3], Vector3f const& iPos, float iScreenRatio, Matrix4f& oProj, Matrix4f& oView, float fov, float displayedSize)
  {
    oProj.MakeZero();
    oView.MakeIdentity();

    float nearP = displayedSize * (1.0 / sin(fov) - 1);
    float farP = displayedSize * 1000;

    oProj.m_Data[0] = 2 * nearP / (displayedSize * iScreenRatio);
    oProj.m_Data[5] = 2 * nearP / displayedSize ;
    oProj.m_Data[10] = -1.0* (nearP + farP) / (farP - nearP);
    oProj.m_Data[14] = -2.0 * nearP * farP / (farP - nearP);
    oProj.m_Data[11] = -1.0;

    Vector3f basisX = iBasis[0];
    Vector3f basisY = iBasis[1];
    Vector3f basisZ = iBasis[2];

    memcpy(oView.m_Data + 0, &basisX, sizeof(Vector3f));
    memcpy(oView.m_Data + 4, &basisY, sizeof(Vector3f));
    memcpy(oView.m_Data + 8, &basisZ, sizeof(Vector3f));

    Vector3f transPos = basisX * (-iPos.X()) + basisY * (-iPos.Y()) + basisZ * (-iPos.Z());
    oView.m_Data[12] = iPos.X();
    oView.m_Data[13] = iPos.Y();
    oView.m_Data[14] = iPos.Z();

    oView = oView.Inverse();
  }
}