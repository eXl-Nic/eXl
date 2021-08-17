#pragma once

#include <dunatk/common/world.hpp>
#include <dunatk/gfx/gfxsystem.hpp>

namespace eXl
{
  class DunAtk_Application;
  class World;
  class Random;
  class Image;

  class SampleExperiment
  {
  public:

    enum ColorAlgo
    {
      KMeans,
      GMM,
      SVM,
      NumColorAlgos = 3,
    };

    enum ResampleAlgo
    {
      Average,
      Score,
      NumSampleAlgo = 2,
    };

    void Init(World& iWorld, Random& iRand);

    void Step(World& iWorld);

    void Learn();

    void Resample();

    void BuildComp(GfxSystem& iGfx);

    uint32_t m_NumClasses = 4;
    uint32_t m_NumIterations = 10000;

    Vector2<uint32_t> m_TargetSize = Vector2<uint32_t>(64, 64);

    ColorAlgo m_ColorAlgo = GMM;
    ResampleAlgo m_SampleAlgo = Score;

    Image* m_Image = nullptr;
    Image* m_Recolored[NumColorAlgos] = {0};
    Image* m_Resampled[NumSampleAlgo] = {0};

    bool m_ResampledDirty = false;
    eXl::ObjectHandle m_TexDisplayObject;

    Random* m_RandGen;
  };
}