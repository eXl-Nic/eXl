/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/mcmcsynthesis.hpp>
#include <gen/diskdistribution.hpp>

namespace eXl
{
  namespace MCMC2D
  {
    class DiskLearnParams : public LearnParams
    {
    public:

      DiskLearnParams(Random& iRandGen)
        : LearnParams(iRandGen)
      {
      }

      unsigned int angleSubdiv = 8;
      unsigned int numQuantiles = 3;
      unsigned int quantile = 10;    
    };


    class EXL_GEN_API DiskLearner : public Learner
    {
    public:

      DiskLearner(Vector<Element> const& iElements,  DiskLearnParams& iParams)
        : Learner(iElements, iParams)
      {}

      void Learn(LearnedModel*& ioModel, LearnExample const& iExamples, unsigned int const numIter, Debug* iDebug) override;

      void Refine(LearnedModel& ioModel, unsigned int const numIter) override;

      static String const& ModelName();

      String const& GetModelName() const override;

      LearnedModel* UnstreamModel(Unstreamer& streamer) const override;

      static LearnedModel* StaticUnstreamModel(Unstreamer& streamer);

      virtual DiskLearnParams const& GetParams() const { return static_cast<DiskLearnParams&>(m_Params); }

    };

    class DiskModel : public LearnedModel
    {
      friend class DiskLearner;
    public:

      DiskModel()
        : LearnedModel(Vector<DistParams>(), Vector<Element>(), true)
      {}

      DiskModel(Vector<DistParams> const& iParams, Vector<Element> const& iElements, bool iToroidal)
        : LearnedModel(iParams, iElements, iToroidal)
      {}

      String const& GetModelName() const override;

      float Sample(FullInteraction const&) override;

      Err Stream(Streamer& streamer) const override;
      Err Unstream(Unstreamer& streamer) override;

    protected:
      Vector<DiskDistribution> m_Disks;
    };
  }
}