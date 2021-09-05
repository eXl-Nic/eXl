/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/mcmcsynthesis.hpp>

namespace eXl
{
  namespace MCMC2D
  {
    class SVMLearnParams : public LearnParams
    {
    public:

      SVMLearnParams(Random& iRandGen)
        : LearnParams(iRandGen)
      {
      }

      float m_KernelSigma = 0.2;
    };

    class EXL_GEN_API SVMLearner : public Learner
    {
    public:

      SVMLearner(Vector<Element> const& iElements,  SVMLearnParams& iParams)
        : Learner(iElements, iParams)
      {}

      void Learn(LearnedModel*& ioModel, LearnExample const& iExamples, unsigned int const numIter, Debug* iDebug) override;

      void Refine(LearnedModel& ioModel, unsigned int const numIter) override;

      static String const& ModelName();

      String const& GetModelName() const override;

      static LearnedModel* StaticUnstreamModel(Unstreamer& streamer);

      LearnedModel* UnstreamModel(Unstreamer& streamer) const override;

      virtual SVMLearnParams const& GetParams() const { return static_cast<SVMLearnParams&>(m_Params); }

    };

    struct GaussianKernel
    {
      GaussianKernel()
        : m_Sigma(1.0)
        , m_SigmaSquared(1.0)
      {}

      GaussianKernel(double iSigma)
        : m_Sigma(iSigma)
        , m_SigmaSquared(iSigma * iSigma)
      {}

      GaussianKernel(GaussianKernel const& iKernel)
        : m_Sigma(iKernel.m_Sigma)
        , m_SigmaSquared(iKernel.m_SigmaSquared)
      {}

      template <unsigned int VectorDim1, unsigned int VectorDim2>
      double operator()(double const (&iVec1)[VectorDim1], double const (&iVec2)[VectorDim2]) const
      {
        static_assert(VectorDim1 == VectorDim2, "Incompatible vectors");
        double res = 0;

        for(int i=0;i<VectorDim1;i++)
        {
          double diff = (iVec1[i]-iVec2[i]);
          res += diff * diff;
        }

        return exp(-res*0.5/m_SigmaSquared);
      }

      double m_Sigma;
      double m_SigmaSquared;
    };

    template <unsigned int Dim, typename Kernel>
    struct Hyperplane 
    {
      Hyperplane()
      {
      }

      Hyperplane(Kernel const& iKernel)
        : m_Kernel(iKernel)
      {

      }

      Hyperplane(Hyperplane const& iPlane)
        : m_Kernel(iPlane.m_Kernel)
        , m_Bias(iPlane.m_Bias)
        , m_Supports(iPlane.m_Supports)
      {

      }

      struct SupportVector
      {
        double m_Coeff;
        double m_Position[Dim];
      };

      typedef Vector<SupportVector> Supports; 

      template <unsigned int VectorDim>
      double operator()(double const (&iVector)[VectorDim]) 
      {
        static_assert(VectorDim == Dim, "Incompatible vectors");
        double res = 0.0;
        for(auto const& support : m_Supports)
        {
          res += support.m_Coeff * m_Kernel(support.m_Position, iVector); 
        }

        return res + m_Bias;
      }
      Kernel m_Kernel;
      Supports m_Supports;
      double m_Bias;
    };

#define SVM_VECTOR_SIZE 3

    class SVMModel : public LearnedModel
    {
      friend class SVMLearner;
    public:

      static constexpr unsigned int Dim = SVM_VECTOR_SIZE;
      typedef GaussianKernel Kernel;
      typedef Hyperplane<Dim, Kernel> Hyperplane;

      SVMModel()
        : LearnedModel(Vector<DistParams>(), Vector<Element>(), false)
      {}

      SVMModel(Vector<DistParams> const& iDist, Vector<Element> const& iElements, bool iToroidal)
        : LearnedModel(iDist, iElements, iToroidal)
      {}

      String const& GetModelName() const override;

      float Sample(FullInteraction const&) override;

      Err Stream(Streamer& streamer) const override;
      Err Unstream(Unstreamer& streamer) override;

      bool CanSuggestPosition() const override;

      bool SuggestPosition(Random& iRand, InputBuilder& iBuilder, PlacedElement const& iElem, PlacedElement& ioSuggestion) const override;

      static const unsigned int s_NumSigma = 1;
      
    protected:

      Vector<Hyperplane> m_Hyperplanes;
    };
  }
}