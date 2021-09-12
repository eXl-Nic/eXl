#include "sampleexperiment.hpp"
#include <core/image/image.hpp>
#include <core/image/imagestreamer.hpp>

#include <ogl/renderer/ogltextureloader.hpp>

#include <dunatk/common/app.hpp>
#include <dunatk/common/menumanager.hpp>
#include "utils.hpp"
#include <imgui.h>
#if 0
#include <opencv2/ml.hpp>

namespace eXl
{
  class LearingParamsPanel : public MenuManager::Panel
  {
  public:
    LearingParamsPanel(SampleExperiment& iExperiment)
      : m_Experiment(iExperiment)
    {

    }

  protected:
    void Display() override
    {
      int param = m_Experiment.m_NumClasses;
      ImGui::InputInt("Num classes", &param);
      m_Experiment.m_NumClasses = Mathi::Clamp(param, 1, 1024);

      param = m_Experiment.m_NumIterations;
      ImGui::InputInt("Num iterations", &param);
      m_Experiment.m_NumIterations = Mathi::Clamp(param, 1, 1000000);

      char const* algoNames[] = {"KMeans", "GMM"};

      param = m_Experiment.m_ColorAlgo;
      if (ImGui::BeginCombo("Color Sampling Algo", algoNames[param]))
      {
        for (unsigned int i = 0; i < 2; ++i)
        {
          bool selected = false;
          ImGui::Selectable(algoNames[i], &selected);
          if (selected)
          {
            param = i;
          }
        }
        ImGui::EndCombo();
      }

      if (param != m_Experiment.m_ColorAlgo)
      {
        m_Experiment.m_ResampledDirty = true;
      }
      m_Experiment.m_ColorAlgo = (SampleExperiment::ColorAlgo)param;
      
      if (ImGui::Button("Learn"))
      {
        m_Experiment.Learn();
      }

      if (ImGui::Button("Resample"))
      {
        m_Experiment.Resample();
      }
    }

    SampleExperiment& m_Experiment;
  };

  void SampleExperiment::Init(World& iWorld, Random& iRand)
  {
    m_TexDisplayObject = iWorld.CreateObject();
    GfxSystem* gfxSys = iWorld.GetSystem<GfxSystem>();
    gfxSys->CreateComponent(m_TexDisplayObject);

    iWorld.GetSystem<Transforms>()->AddTransform(m_TexDisplayObject, nullptr);

    m_Image = ImageStreamer::Get().Load("D:\\Cloud_Strife.png");

    BuildComp(*gfxSys);

    eXl_ASSERT(m_Image->GetFormat() == Image::Char);
    eXl_ASSERT(m_Image->GetComponents() == Image::RGBA || m_Image->GetComponents() == Image::BGRA);

    EngineCommon_Application& appl = static_cast<EngineCommon_Application&>(Application::GetAppl());

    appl.GetMenuManager().AddMenu("ResampleImage")
      .AddOpenPanelCommand("LearnColorParams", [this] { return new LearingParamsPanel(*this); })
      .EndMenu();
  }

  void SampleExperiment::Learn()
  {
    Vector<Vector2i> samplePos;
    Vector<Vector3f> samples;

    auto imageSize = m_Image->GetSize();
    for (uint32_t y = 0; y < imageSize.Y(); ++y)
    {
      for (uint32_t x = 0; x < imageSize.X(); ++x)
      {
        uint8_t const* pixel = reinterpret_cast<uint8_t const*>(m_Image->GetPixel(y, x));

        if (pixel[3] > 192)
        {
          samples.push_back(Vector3f(float(pixel[0]) / 255, float(pixel[1]) / 255, float(pixel[2]) / 255));
          samplePos.push_back(Vector2i(x, y));
        }
      }
    }

    Vector<Vector3f> classes(m_NumClasses);
    Vector<uint32_t> sampleClass(samples.size());

    cv::Mat inputDataMat;
    int inputSize[] = { samples.size(), 3 };
    inputDataMat.create(2, inputSize, CV_32F);

    for (uint32_t sampleIdx = 0; sampleIdx < samples.size(); ++sampleIdx)
    {
      inputDataMat.row(sampleIdx).col(0) = samples[sampleIdx].X();
      inputDataMat.row(sampleIdx).col(1) = samples[sampleIdx].Y();
      inputDataMat.row(sampleIdx).col(2) = samples[sampleIdx].Z();
    }
    std::vector<int> output;
    std::vector<cv::Point3f> means;

    if (m_ColorAlgo == GMM)
    {
      cv::Ptr<cv::ml::EM> algo = cv::ml::EM::create();
      algo->setClustersNumber(m_NumClasses);
      bool succeeded = algo->trainEM(inputDataMat, cv::noArray(), output);
      eXl_ASSERT(succeeded);

      cv::Mat meansMat = algo->getMeans();
 
      for (uint32_t meanIdx = 0; meanIdx < m_NumClasses; ++meanIdx)
      {
        cv::Mat mean = meansMat.row(meanIdx);
        means.push_back(cv::Point3f(mean.at<double>(0), mean.at<double>(1), mean.at<double>(2)));
      }
    }

    if (m_ColorAlgo == KMeans)
    {
      cv::TermCriteria crit(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, Mathi::Max(m_NumIterations, samples.size() * 10), Mathf::EPSILON);
      cv::kmeans(inputDataMat, m_NumClasses, output, crit, 1, cv::KMEANS_RANDOM_CENTERS, means);
    }
    
    m_ResampledDirty = true;
    eXl_DELETE m_Recolored[m_ColorAlgo];

    m_Recolored[m_ColorAlgo] = eXl_NEW Image(*m_Image);

    for (uint32_t i = 0; i < samples.size(); ++i)
    {
      Vector2i const& pos = samplePos[i];
      cv::Point3f const& center = means[output[i]];
      uint8_t* pixelData = reinterpret_cast<uint8_t*>(m_Recolored[m_ColorAlgo]->GetPixel(pos.Y(), pos.X()));
      pixelData[0] = 255 * center.x;
      pixelData[1] = 255 * center.y;
      pixelData[2] = 255 * center.z;
    }
  }

  void SampleExperiment::Resample()
  {
    eXl_DELETE m_Resampled[m_SampleAlgo];
    Image* srcImage = m_Recolored[m_ColorAlgo] ? m_Recolored[m_ColorAlgo] : m_Image;

    size_t const pixelSize = srcImage->GetPixelSize();

    uint32_t totColorPixels = 0;
    UnorderedMap<uint32_t, float> colorProb;
    if (m_SampleAlgo == Score)
    {
      for (uint32_t y = 0; y < m_TargetSize.Y(); ++y)
      {
        uint8_t* row = (uint8_t*)srcImage->GetRow(y);
        for (uint32_t x = 0; x < m_TargetSize.X(); ++x)
        {
          if (row[3] > 128)
          {
            uint32_t color = row[0] | (row[1] << 8) | row[2] << 16;
            auto colorEntry = colorProb.insert(std::make_pair(color, 0)).first;
            ++colorEntry->second;
            ++totColorPixels;
          }
          row += pixelSize;
        }
      }
    }

    for (auto& entry : colorProb)
    {
      entry.second /= totColorPixels;
    }

    Image::Size samplingZones(srcImage->GetSize().X() / m_TargetSize.X(), srcImage->GetSize().Y() / m_TargetSize.Y());
    if (srcImage->GetSize().X() % m_TargetSize.X() != 0)
    {
      ++samplingZones.X();
    }
    if (srcImage->GetSize().Y() % m_TargetSize.Y() != 0)
    {
      ++samplingZones.Y();
    }

    Vector2i startOffset(samplingZones.X() * m_TargetSize.X() - srcImage->GetSize().X(), samplingZones.Y() * m_TargetSize.Y() - srcImage->GetSize().Y());
    startOffset /= 2;

    Vector2i startCoord = startOffset;
    Image resampledImage(nullptr, m_TargetSize, srcImage->GetComponents(), srcImage->GetFormat(), 1);
    for (uint32_t subY = 0; subY < m_TargetSize.Y(); ++subY)
    {
      uint8_t* destRow = (uint8_t*)resampledImage.GetRow(subY);
      for (uint32_t subX = 0; subX < m_TargetSize.X(); ++subX)
      {
        Vector2i endCoord(Mathi::Min((subX + 1) * samplingZones.X(), srcImage->GetSize().X()),
          Mathi::Min((subY + 1) * samplingZones.Y(), srcImage->GetSize().Y()));

        if (m_SampleAlgo == Average)
        {
          float average[4] = { 0 };
          uint32_t numPixels = 0;
          for (uint32_t pixelY = startCoord.Y(); pixelY < endCoord.Y(); ++pixelY)
          {
            uint8_t* srcRow = (uint8_t*)srcImage->GetRow(pixelY) + startCoord.X() * pixelSize;
            for (uint32_t pixelX = startCoord.X(); pixelX < endCoord.X(); ++pixelX)
            {
              average[0] += float(srcRow[0]) / 255;
              average[1] += float(srcRow[1]) / 255;
              average[2] += float(srcRow[2]) / 255;
              average[3] += float(srcRow[3]) / 255;
              ++numPixels;
              srcRow += pixelSize;
            }
          }

          destRow[0] = (average[0] / numPixels) * 255;
          destRow[1] = (average[1] / numPixels) * 255;
          destRow[2] = (average[2] / numPixels) * 255;
          destRow[3] = (average[3] / numPixels) * 255;
        }

        if (m_SampleAlgo == Score)
        {
          Map<uint32_t, float> scores;
          uint32_t numPixels = 0;
          for (uint32_t pixelY = startCoord.Y(); pixelY < endCoord.Y(); ++pixelY)
          {
            uint8_t* srcRow = (uint8_t*)srcImage->GetRow(pixelY) + startCoord.X() * pixelSize;
            for (uint32_t pixelX = startCoord.X(); pixelX < endCoord.X(); ++pixelX)
            {
              if (srcRow[3] > 128)
              {
                uint32_t color = srcRow[0] | (srcRow[1] << 8) | srcRow[2] << 16;
                auto colorEntry = scores.insert(std::make_pair(color, 0)).first;
                ++colorEntry->second;
                ++numPixels;
              }
              srcRow += pixelSize;
            }
          }
          if (numPixels > 0)
          {
            uint32_t chosen = scores.begin()->first;
            float curScore = scores.begin()->second / colorProb[scores.begin()->first];
            for (auto& colorEntry : scores)
            {
              float score = colorEntry.second / colorProb[colorEntry.first];
              if (score > curScore)
              {
                chosen = colorEntry.first;
                curScore = score;
              }
            }

            destRow[0] = (chosen >> 0 ) & 255;
            destRow[1] = (chosen >> 8 ) & 255;
            destRow[2] = (chosen >> 16) & 255;
            destRow[3] = 255;
          }
          else
          {
            destRow[0] = destRow[1] = destRow[2] = destRow[3] = 0;
          }
        }

        startCoord.X() = (subX + 1) * samplingZones.X();
        destRow += pixelSize;
      }
      startCoord.X() = startOffset.X();
      startCoord.Y() = (subY + 1) * samplingZones.Y();
    }

    m_Resampled[m_SampleAlgo] = eXl_NEW Image(std::move(resampledImage));

    m_ResampledDirty = true;
  }

  void SampleExperiment::Step(World& iWorld)
  {
    if (m_ResampledDirty)
    {
      BuildComp(*iWorld.GetSystem<GfxSystem>());
      m_ResampledDirty = false;
    }
  }

  void SampleExperiment::BuildComp(GfxSystem& iGfx)
  {
    Image* imageToUse = m_Resampled[m_SampleAlgo] ? m_Resampled[m_SampleAlgo] : (m_Recolored[m_ColorAlgo] ? m_Recolored[m_ColorAlgo] : m_Image);

    OGLTexture* texture = OGLTextureLoader::CreateFromImage(imageToUse, true);

    auto spriteGeom = Utils::MakeSpriteGeometry(80, Vector2f::ONE, Vector2i::ZERO);

    auto material = eXl_NEW SpriteMaterialInfo;
    material->m_Texture = texture;
    material->m_SpriteInfo.color = Vector4f::ONE;
    material->m_SpriteInfo.alphaMult = 1.0;
    material->SetupData();

    GfxComponent& testComp = iGfx.GetComponent(m_TexDisplayObject);
    testComp.SetGeometry(spriteGeom.get());
    testComp.ClearDraws();
    testComp.AddDraw(material, 6, 0);
  }
}

#endif