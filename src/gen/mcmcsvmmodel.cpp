/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/mcmcsvmmodel.hpp>
#include <math/mathtools.hpp>
//#include <core/thread/workerthread.hpp>
//#include <core/thread/event.hpp>
//#include <atomic>
#include <core/clock.hpp>

#include <svm.h>
#include <lbfgs.h>
#include <fstream>

namespace eXl
{

  namespace TypeTraits
  {
    template <>
    Err Stream<MCMC2D::SVMModel::Hyperplane>(MCMC2D::SVMModel::Hyperplane const* iObj, Streamer& iStreamer)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Sigma");
      iStreamer.Write(&iObj->m_Kernel.m_Sigma);
      iStreamer.PopKey();
      iStreamer.PushKey("Bias");
      iStreamer.Write(&iObj->m_Bias);
      iStreamer.PopKey();
      iStreamer.PushKey("Vectors");
      iStreamer.BeginSequence();
      for(auto& vector : iObj->m_Supports)
      {
        iStreamer.BeginStruct();
        iStreamer.PushKey("Weight");
        iStreamer.Write(&vector.m_Coeff);
        iStreamer.PopKey();
        iStreamer.PushKey("Center");
#if SVM_VECTOR_SIZE == 2
        static_assert(sizeof(Vector2d) == MCMC2D::SVMModel::Dim * sizeof(double), "");
        iStreamer.Write(reinterpret_cast<Vector2d const*>(&vector.m_Position));
#else
        static_assert(sizeof(Vector3d) == MCMC2D::SVMModel::Dim * sizeof(double), "");
        iStreamer.Write(reinterpret_cast<Vector3d const*>(&vector.m_Position));
#endif
        iStreamer.PopKey();
        iStreamer.EndStruct();
      }
      iStreamer.EndSequence();
      iStreamer.PopKey();
      iStreamer.EndStruct();

      return Err::Success;
    }

    template <>
    Err Unstream<MCMC2D::SVMModel::Hyperplane>(MCMC2D::SVMModel::Hyperplane * oObj, Unstreamer& iUnstreamer)
    {
      iUnstreamer.BeginStruct();
      iUnstreamer.PushKey("Sigma");
      iUnstreamer.Read(&oObj->m_Kernel.m_Sigma);
      oObj->m_Kernel.m_SigmaSquared = oObj->m_Kernel.m_Sigma * oObj->m_Kernel.m_Sigma;
      iUnstreamer.PopKey();
      iUnstreamer.PushKey("Bias");
      iUnstreamer.Read(&oObj->m_Bias);
      iUnstreamer.PopKey();
      iUnstreamer.PushKey("Vectors");
      Err seq = iUnstreamer.BeginSequence();
      if((seq))
      {
        do
        {
          MCMC2D::SVMModel::Hyperplane::SupportVector vector;
          iUnstreamer.BeginStruct();
          iUnstreamer.PushKey("Weight");
          iUnstreamer.Read(&vector.m_Coeff);
          iUnstreamer.PopKey();
          iUnstreamer.PushKey("Center");
#if SVM_VECTOR_SIZE == 2
          iUnstreamer.Read(reinterpret_cast<Vector2d*>(vector.m_Position));
#else
          iUnstreamer.Read(reinterpret_cast<Vector3d*>(vector.m_Position));
#endif
          iUnstreamer.PopKey();
          iUnstreamer.EndStruct();
          
          oObj->m_Supports.emplace_back(vector);

        }while((seq = iUnstreamer.NextSequenceElement()));
      }
      iUnstreamer.PopKey();
      iUnstreamer.EndStruct();

      return Err::Success;
    }
  }


#define EXP_METHOD
#define ONE_BETA

  namespace MCMC2D
  {
    static const String s_modelName = "MCMC2D::SVMModel";

    String const& SVMLearner::ModelName()
    {
      return s_modelName;
    }

    String const& SVMLearner::GetModelName() const
    {
      return s_modelName;
    }

    String const& SVMModel::GetModelName() const
    {
      return s_modelName;
    }

    LearnedModel* SVMLearner::UnstreamModel(Unstreamer& streamer) const
    {
      return StaticUnstreamModel(streamer);
    }

    LearnedModel* SVMLearner::StaticUnstreamModel(Unstreamer& streamer)
    {
      SVMModel* newModel = new SVMModel;

      newModel->Unstream(streamer);

      return newModel;
    }

    Err SVMModel::Stream(Streamer& streamer) const
    {
      streamer.BeginStruct();
      WriteCommon(streamer);
      streamer.PushKey("Params");
      streamer.Write(&m_Hyperplanes);
      streamer.PopKey();
      streamer.EndStruct();
      return Err::Success;
    }

    Err SVMModel::Unstream(Unstreamer& streamer)
    {
      streamer.BeginStruct();
      ReadCommon(streamer);
      streamer.PushKey("Params");
      streamer.Read(&m_Hyperplanes);
      streamer.PopKey();
      streamer.EndStruct();
      
      return Err::Success;
    }

    double SoftPlusFun(double iVal)
    {
      double res = log(1.0 + exp(iVal));
      return res * res;
    }

    double SoftPlusFunD(double iVal)
    {
      double res = 1.0 / (1.0 + exp(-iVal));
      return 2 * res * SoftPlusFun(iVal);
    }

    //double SVMModel::Sigma(uint32_t idx)
    //{
    //  double sigmas[s_NumSigma] = {0.2/*, 0.1*/};
    //  return sigmas[idx];
    //}
    
    float SVMModel::Sample(FullInteraction const& iInter)
    {
#if SVM_VECTOR_SIZE == 3
      Vector3d point;
#else
      Vector2d point;
#endif
      point.m_Data[0] = iInter.dir1.X() * 0.5;
      point.m_Data[1] = iInter.dir1.Y() * 0.5;
#if SVM_VECTOR_SIZE == 2
      point *= iInter.dist / GetMaxDist(iInter.oneHotIdx);
#else
      point.m_Data[2] = 2*(iInter.dist / GetMaxDist(iInter.oneHotIdx)) - 1.0;
#endif
     
      double alpha = 1.0;
      double constant = 0.0;
      if(m_Mask)
      {
        m_Mask->ComputeDistribution(iInter.oneHotIdx, iInter.dist, iInter.dir1, constant, alpha);
      }
      
      double val = 0.0;
      for(unsigned int sigma = 0; sigma < s_NumSigma; ++sigma)
      {
        val += m_Hyperplanes[(sigma * m_Hyperplanes.size() / s_NumSigma) + iInter.oneHotIdx](point.m_Data);
      }

      //DistParams const& params = m_DistParams.size() > 1 ? m_DistParams[iInter.oneHotIdx] : m_DistParams[0];

#ifdef EXP_METHOD
      //return alpha + (1.0 - alpha) * exp(val);
      return alpha * exp(val) + (1.0 - alpha) * constant;
#else
      return alpha * SoftPlusFun(val) + (1.0 - alpha) * constant;
#endif
    }

    namespace
    {
        struct CellInfo
        {
          unsigned int element;
          float weight;
          bool isPres;
          Vector<FullInteraction> interactions;
        };

        struct RegressionContext_SVM
        {
          Vector<SVMModel::Kernel> const& kernels;
          Vector<CellInfo> cells;
          Vector<Vector<Vector<Vector3d>>> supportVectors;
          Vector<Vector<unsigned int>> varOffset;
          Vector<Vector<unsigned int>> planeIdx;
          Vector<lbfgsfloatval_t> cellCache;
          Vector<lbfgsfloatval_t> cellInterCache;
          unsigned int numBeta;
          bool oneBeta = false;
          Vector<LearnedModel::DistParams> distParams;
          float m_RegularisationCoeff;

          RegressionContext_SVM(Vector<SVMModel::Kernel> const& iKernel, float iRegCoeff)
            : kernels(iKernel)
            , m_RegularisationCoeff(iRegCoeff)
          {}


          static lbfgsfloatval_t _evaluate(
            void *instance,
            const lbfgsfloatval_t *x,
            lbfgsfloatval_t *g,
            const int n,
            const lbfgsfloatval_t step
          )
          {
            return reinterpret_cast<RegressionContext_SVM*>(instance)->evaluate(x, g, n, step);
          }

          lbfgsfloatval_t evaluate(
            const lbfgsfloatval_t *x,
            lbfgsfloatval_t *g,
            const int n,
            const lbfgsfloatval_t step
          )
          {

            lbfgsfloatval_t presenceSum = 0.0;
            lbfgsfloatval_t absenceSum = 0.0;

            memset(g, 0, n*sizeof(lbfgsfloatval_t));

            for(auto& cell : cells)
            {
              cellCache.clear();
              cellInterCache.clear();
              unsigned int curVec = 0;
#ifdef ONE_BETA
              unsigned int const densityIdx = 0;
              lbfgsfloatval_t density = x[0];
#else
              unsigned int const densityIdx = cell.element;
              lbfgsfloatval_t density = x[densityIdx];
#endif

#ifdef EXP_METHOD
              lbfgsfloatval_t logLambda = density;
#else
              lbfgsfloatval_t lambda = density;
#endif
              for(unsigned int i = 0; i< cell.interactions.size(); ++i)
              {
                auto& inter = cell.interactions[i];
                lbfgsfloatval_t interValue = 0.0;
                Vector3d point;
                point.m_Data[0] = inter.dir1.X() * 0.5;
                point.m_Data[1] = inter.dir1.Y() * 0.5;

#if SVM_VECTOR_SIZE == 2
                point *= inter.dist / GetMaxDist_Common(inter.oneHotIdx, distParams);
#else
                point.m_Data[2] = 2*(inter.dist / GetMaxDist_Common(inter.oneHotIdx, distParams)) - 1;
#endif
                
                for(unsigned int kernelNum = 0; kernelNum < varOffset.size(); ++kernelNum)
                {
                  interValue += x[planeIdx[kernelNum][inter.oneHotIdx]];
                  unsigned int varIdx = varOffset[kernelNum][inter.oneHotIdx];
                  for(auto& support : supportVectors[kernelNum][inter.oneHotIdx])
                  {
                    cellCache.push_back(kernels[kernelNum](point.m_Data, support.m_Data));
#ifdef EXP_METHOD
                    interValue += cellCache[curVec] * x[varIdx];
#else
                    interValue += cellCache[curVec] * x[varIdx];
#endif
                    ++varIdx;
                    ++curVec;
                  }
                }
#ifdef EXP_METHOD
                logLambda += interValue;
#else
                lambda *= SoftPlusFun(interValue);
#endif
                cellInterCache.push_back(interValue);
              }
#ifndef EXP_METHOD
              
#endif
              curVec = 0;
#ifdef EXP_METHOD
              lbfgsfloatval_t lambda = exp(logLambda);
#else
              lbfgsfloatval_t logLambda = log(lambda);
#endif

              if(cell.isPres)
              {
                presenceSum += logLambda;
              }
              absenceSum += cell.weight * lambda;

              lbfgsfloatval_t& betaDeriv = g[densityIdx];
              if(cell.isPres)
              {
#ifdef EXP_METHOD
                betaDeriv -= 1.0;
#else
                betaDeriv -= 1.0 / density;
#endif
              }
#ifdef EXP_METHOD
              betaDeriv += cell.weight * lambda;
#else
              betaDeriv += cell.weight * lambda / density;
#endif

              unsigned int interIdx = 0;

              for(unsigned int i = 0; i< cell.interactions.size(); ++i)
              {
                auto& inter = cell.interactions[i];
                for(unsigned int kernelNum = 0; kernelNum < varOffset.size(); ++kernelNum)
                {
                  unsigned int curPlaneIdx = planeIdx[kernelNum][inter.oneHotIdx];
#ifndef EXP_METHOD
                  lbfgsfloatval_t cellSPDeriv = SoftPlusFunD(cellInterCache[interIdx]) / SoftPlusFun(cellInterCache[interIdx]);
#endif
                  if(cell.isPres)
                  {
#ifdef EXP_METHOD
                    g[curPlaneIdx] -= 1.0;
#else
                    g[curPlaneIdx] -= cellSPDeriv;
#endif
                  }
#ifdef EXP_METHOD
                  g[curPlaneIdx] += cell.weight * lambda;
#else
                  g[curPlaneIdx] += cell.weight * lambda * cellSPDeriv;
#endif

                  unsigned int varIdx = varOffset[kernelNum][inter.oneHotIdx];
                  for(auto& support : supportVectors[kernelNum][inter.oneHotIdx])
                  {
                    if(cell.isPres)
                    {
#ifdef EXP_METHOD
                      g[varIdx] -= cellCache[curVec];
#else
                      g[varIdx] -= cellCache[curVec] * cellSPDeriv;
#endif
                    }
#ifdef EXP_METHOD
                    g[varIdx] += cellCache[curVec] * cell.weight * lambda;
#else
                    g[varIdx] += cellCache[curVec] * cell.weight * lambda * cellSPDeriv;
#endif
                    ++varIdx;
                    ++curVec;
                  }
                }
                ++interIdx;
              }
            }

            lbfgsfloatval_t regularisationTerm = 0.0;
            //for(auto& planeId : planeIdx)
            {
              //for(auto i : planeId)
              for(int i = 0; i<n; ++i)
              {
                regularisationTerm += m_RegularisationCoeff * 0.5 * x[i] * x[i];
                g[i] += m_RegularisationCoeff * x[i];
              }
            }

            return (absenceSum - presenceSum) + regularisationTerm;
          }

          static int _progress(
            void *instance,
            const lbfgsfloatval_t *x,
            const lbfgsfloatval_t *g,
            const lbfgsfloatval_t fx,
            const lbfgsfloatval_t xnorm,
            const lbfgsfloatval_t gnorm,
            const lbfgsfloatval_t step,
            int n,
            int k,
            int ls
          )
          {
            return reinterpret_cast<RegressionContext_SVM*>(instance)->progress(x, g, fx, xnorm, gnorm, step, n, k, ls);
          }

          int progress(
            const lbfgsfloatval_t *x,
            const lbfgsfloatval_t *g,
            const lbfgsfloatval_t fx,
            const lbfgsfloatval_t xnorm,
            const lbfgsfloatval_t gnorm,
            const lbfgsfloatval_t step,
            int n,
            int k,
            int ls
          )
          {
            printf("Iteration %d:\n", k);
            printf("  fx = %f\n", fx);
            printf("  xnorm = %f, gnorm = %f, step = %f\n", xnorm, gnorm, step);
            printf("\n");
            return 0;
          }
        };
      }

    //template <int DIM,class KERNEL>
    //std::ostream& operator<<(std::ostream& os,svm::Hyperplane<DIM,KERNEL> const& h) {
    //  typename std::vector<std::pair<svm::Real,svm::Vector<DIM,KERNEL> > >::iterator iter;
    //
    //  os << h.b;
    //  os << ' ';
    //  os << h.supports.size() << std::endl;
    //  for(iter=h.supports.begin();iter!=h.supports.end();iter++)
    //    os << iter->first << ' ' << iter->second << std::endl;
    //
    //  return os;
    //}
    //
    //template<int DIM, class KERNEL>
    //std::ostream& operator<<(std::ostream& os, svm::Vector<DIM,KERNEL> const& v) 
    //{
    //  int i;
    //
    //  for(i=0;i<DIM;i++)
    //    os << v[i] << ' ';
    //
    //  return os;
    //}

    void SVMLearner::Refine(LearnedModel& ioModel, unsigned int const numIter)
    {
      SVMModel& model = static_cast<SVMModel&>(ioModel);

      InputBuilder& builder = GetBuilder();
      auto& cells = GetCells();

      uint32_t modelNumSigma = model.m_Hyperplanes.size() / builder.oneHotSize;

      Vector2i curSigmaRange(0, modelNumSigma);

      Vector<Vector<SVMModel::Hyperplane>> planes(modelNumSigma, Vector<SVMModel::Hyperplane>(builder.oneHotSize));

      Vector<SVMModel::Kernel> kernels;
      for(uint32_t sigmaIdx = 0; sigmaIdx < modelNumSigma; ++sigmaIdx)
      {
        uint32_t kernelOffset = sigmaIdx * builder.oneHotSize;
        kernels.push_back(model.m_Hyperplanes[kernelOffset].m_Kernel);
        for(uint32_t idx = 0; idx < builder.oneHotSize; ++idx)
        {
          planes[sigmaIdx][idx] = model.m_Hyperplanes[kernelOffset + idx];
        }
      }

      RegressionContext_SVM ctx(kernels, m_Params.m_RegularisationCoeff);
#ifdef ONE_BETA
      ctx.numBeta = 1;
#else
      ctx.numBeta = GetElements().size();
#endif

      for(auto& val : m_DistParams)
      {
        ctx.distParams.push_back(val);
      }

      unsigned int numVars = ctx.numBeta;

      for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
      {
        ctx.planeIdx.push_back(Vector<unsigned int>());
        for(unsigned int i = 0; i < builder.oneHotSize; ++i)
        {
          ctx.planeIdx.back().push_back(numVars);
          ++numVars;
        }
      }

      for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
      {
        ctx.varOffset.push_back(Vector<unsigned int>());
        ctx.supportVectors.push_back(Vector<Vector<Vector3d>>());
        for(auto& plane : planes[numSigma])
        {
          ctx.supportVectors.back().push_back(Vector<Vector3d>());
          ctx.varOffset.back().push_back(numVars);
          for(auto& supportVector : plane.m_Supports)
          {
            if(supportVector.m_Coeff != 0.0)
            {
              ctx.supportVectors.back().back().push_back(*reinterpret_cast<Vector3d const*>(supportVector.m_Position));
              ++numVars;
            }
          }
        }
      }

      for(auto& cell : cells)
      {
        CellInfo info;
        info.isPres = cell.isPresence;
        info.weight = cell.weight;
        info.element = cell.elem1;
        for(auto& inter : cell.interactions)
        {
          info.interactions.push_back(FullInteraction::Make(builder, cell, inter));
        }

        ctx.cells.push_back(std::move(info));
      }

      lbfgsfloatval_t* x = lbfgs_malloc(numVars);
      {
        lbfgsfloatval_t* xIter = x;
        lbfgsfloatval_t* xIterEnd = x + numVars;
        uint32_t i = 0;
#ifndef ONE_BETA
        
        for(; xIter < x + ctx.numBeta; ++xIter, ++i)
#endif
        {
#ifdef EXP_METHOD
          *xIter = log(model.m_Elements[i].m_AbsDensity);
#else
          *xIter = model.m_Elements[i].m_AbsDensity;
#endif
        }

#ifdef ONE_BETA
        ++xIter;
#endif

        for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
        {
          for(auto& plane : planes[numSigma])
          {
            *xIter = plane.m_Bias;
            ++xIter;
          }
        }

        for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
        {
          for(auto& plane : planes[numSigma])
          {
            for(auto& supportVector : plane.m_Supports)
            {
              if(supportVector.m_Coeff != 0.0)
              {
                *xIter = supportVector.m_Coeff;
                ++xIter;
              }
            }
          }
        }
      }

      lbfgsfloatval_t fx;
      lbfgs_parameter_t params;
      lbfgs_parameter_init(&params);
      
#ifdef _DEBUG
      params.max_iterations = 20;
#else
      params.max_iterations = numIter;
#endif

#if 1
      auto res = lbfgs(numVars, x, &fx, &RegressionContext_SVM::_evaluate, &RegressionContext_SVM::_progress, &ctx, &params);
      printf("LBFGS result : %i\n", res);
#else
      Vector<lbfgsfloatval_t> g(numVars);

      double const learnRate = 1.0;
      double const adadeltaDecay = 0.9;
      //double const whDecay = 0.00001;

      Vector<lbfgsfloatval_t> accumAdaDeltaGrad(numVars, 0.0);
      Vector<lbfgsfloatval_t> accumAdaDeltaUpd(numVars, 0.0);

      for(uint32_t blah = 0 ; blah < 1000; ++blah)
      {
        lbfgsfloatval_t val = ctx.evaluate(x, g.data(), numVars, blah);

        printf("Step %i : %f\n", blah, val);

        for(unsigned int wh = 0; wh < numVars; ++wh)
        {
          unsigned int const globOffset = wh;
          //double adagradfactor = accumAdagrad[wh];
          lbfgsfloatval_t grad = -g[globOffset];
          accumAdaDeltaGrad[globOffset] = adadeltaDecay * accumAdaDeltaGrad[globOffset] + (1.0 - adadeltaDecay) * grad * grad;

          double update = (  Mathd::Sqrt(accumAdaDeltaUpd[globOffset] + Mathd::ZeroTolerance()) 
            / Mathd::Sqrt(accumAdaDeltaGrad[globOffset] + Mathd::ZeroTolerance())) * grad;

          x[wh] += learnRate * (update /*- x[wh] * whDecay*/);//learnRate;
                                                                  //imgWeights[wh] += -grad * learnRate - imgWeights[wh] * whDecay;
                                                                  //imgWeights[wh] += (grad - imgWeights[wh] * whDecay) * 1.0;
                                                                  //Prevent scaling weight from becoming < 0
          //if(wh == m_Net.GetNumWeight() - 1)
          //{
          //  if(netWh[wh] < 1.0e-4)
          //  {
          //    netWh[wh] = 1.0e-4;
          //  }
          //}

          //x[globOffset] = netWh[wh];
          accumAdaDeltaUpd[globOffset] = adadeltaDecay * accumAdaDeltaUpd[globOffset] + (1.0 - adadeltaDecay) * update * update;

          //accumAdagrad[wh] += grad * grad;
        }
      }
#endif

      {
        lbfgsfloatval_t const* xIter = x;
        Vector<Element>& elements = model.m_Elements;
        for(auto& elem : elements)
        {
#ifdef EXP_METHOD
          elem.m_AbsDensity = exp(*xIter);
#else
          elem.m_AbsDensity = Mathf::Max(Mathd::ZeroTolerance(), Mathd::Abs(*xIter));
#endif

#ifndef ONE_BETA
          ++xIter;
#endif
        }
#ifdef ONE_BETA
        ++xIter;
#endif

        for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
        {
          for(auto& plane : planes[numSigma])
          {
            if(!plane.m_Supports.empty())
            {
              plane.m_Bias = *xIter;
            }
            else
            {
              plane.m_Bias = 0;
            }
            ++xIter;
          }
        }

        for(int numSigma = curSigmaRange.X(); numSigma < curSigmaRange.Y(); ++numSigma)
        {
          for(auto& plane : planes[numSigma])
          {

            for(auto& vector : plane.m_Supports)
            {
              if(vector.m_Coeff != 0.0)
              {
#ifdef EXP_METHOD
                vector.m_Coeff = *xIter;
#else
                vector.m_Coeff = *xIter;
#endif
                ++xIter;
              }
            }
          }
        }
      }

      lbfgs_free(x);
      x = nullptr;

      model.m_Hyperplanes.clear();
      for(int i = curSigmaRange.X(); i< curSigmaRange.Y(); ++i)
      {
        auto& curSigmaPlanes = planes[i];
        model.m_Hyperplanes.insert(model.m_Hyperplanes.begin(), curSigmaPlanes.begin(), curSigmaPlanes.end());
      }

      Vector<Element>& elements = model.m_Elements;
      uint32_t totElements = 0;
      for(unsigned int i = 0; i< elements.size(); ++i)
      {
        printf("Elem %i density was %f \n", i, elements[i].m_AbsDensity);
        unsigned int numPres = 0;
        float sampleSum = 0;
        for(auto& cell : cells)
        {
          if(cell.elem1 == i)
          {
            if(cell.isPresence)
            {
              ++numPres;
              ++totElements;
            }
            //else
            {
              float curCell = cell.weight;
              for(auto& inter : cell.interactions)
              {
                curCell *= model.Sample(FullInteraction::Make(builder, cell, inter));
              }
              sampleSum += curCell;
            }
          }
        }

        //elements[i].m_AbsDensity = float(numPres) / sampleSum;
        printf("Elem %i density %e \n", i, elements[i].m_AbsDensity);
        printf("NumPres % i : %i for weight %f\n", i, numPres, sampleSum);
      }

      for(unsigned int i = 0; i< elements.size(); ++i)
      {
        unsigned int numPres = 0;
        for(auto& cell : cells)
        {
          if(cell.elem1 == i)
          {
            if(cell.isPresence)
            {
              ++numPres;
            }
          }
        }

        elements[i].m_RelDensity = 0.5 * (1.0 / m_Elements.size() + (float)numPres / totElements);
      }
    }

    void SVMLearner::Learn(LearnedModel*& ioModel, /*Vector<*/LearnExample/*>*/ const& iExamples, unsigned int const numIter, Debug* iDebug)
    {
      BuildCells(iExamples);

      //if(iDebug)
      //{
      //  iDebug->GetSamplingCells(GetCells(), cells);
      //}

      InputBuilder& builder = GetBuilder();
      auto& cells = GetCells();

      Vector<Vector<SVMModel::Hyperplane>> planes(SVMModel::s_NumSigma, Vector<SVMModel::Hyperplane>(builder.oneHotSize));
      Vector<Vector<svm_node>> corpus(builder.oneHotSize);
      //Vector<svm::Data<SVMModel::Dim>> corpus(builder.oneHotSize);

      Map<std::pair<unsigned int, unsigned int>, float> importanceWeight;

      unsigned int const numDistStep = 20;
      unsigned int const numAngleSteps = 32;

      for(unsigned int idx = 0; idx < builder.oneHotSize; ++idx)
      {
        unsigned int curIdxCorpusSize = 0;

        for(auto const& cell : cells)
        {
          for(auto const& inter : cell.interactions)
          {
            FullInteraction fullInter = FullInteraction::Make(builder, cell, inter);
            if(fullInter.oneHotIdx == idx 
              && cell.isPresence
              )
            {
              ++curIdxCorpusSize;
            }
          }
        }
        
        corpus[idx].reserve(curIdxCorpusSize * (SVMModel::Dim + 1));
        //corpus[idx].Reserve(curIdxCorpusSize + 1 + (numDistStep - 1) * numAngleSteps);
      
        for(auto const& cell : cells)
        {
          if(cell.isPresence)
          {
            for(auto const& inter : cell.interactions)
            {
              FullInteraction fullInter = FullInteraction::Make(builder, cell, inter);
              if(fullInter.oneHotIdx == idx)
              {
                Vector3d curVect;
                
                curVect.m_Data[0] = fullInter.dir1.X() * 0.5;
                curVect.m_Data[1] = fullInter.dir1.Y() * 0.5;
#if SVM_VECTOR_SIZE == 2
                curVect *= fullInter.dist / GetMaxDist(fullInter.oneHotIdx);
#else
                curVect.m_Data[2] = 2*(fullInter.dist / GetMaxDist(fullInter.oneHotIdx)) - 1;
#endif

                //corpus[idx].Add(curVect, cell.isPresence? 1 : -1);
                svm_node node;
                node.index = 0;
                node.value = curVect.m_Data[0];
                corpus[idx].push_back(node);

                node.index = 1;
                node.value = curVect.m_Data[1];
                corpus[idx].push_back(node);

#if SVM_VECTOR_SIZE == 3
                node.index = 2;
                node.value = curVect.m_Data[2];
                corpus[idx].push_back(node);
#endif

                node.index = -1;
                node.value = 0;
                corpus[idx].push_back(node);
              }
            }
          }
        }

        //svm::Vector<2> centerVect;
        ////curVect[0] = fullInter.dist / GetParams().m_MaxDist;
        //centerVect[0] = 0;
        //centerVect[1] = 0;
        //corpus[idx].Add(centerVect, -1);

        //float const distIncrement = /*GetMaxDist(idx)*/ 1.0 / numDistStep;
        //
        //float const angleIncrement = (2 * Mathf::Pi()) / numAngleSteps;
        //for(unsigned int angleStep = 0; angleStep < numAngleSteps; ++angleStep)
        //{
        //  float curAngle = angleStep * angleIncrement;
        //  Vector2f dir(Mathf::Cos(curAngle), Mathf::Sin(curAngle));
        //  for(unsigned int distStep = 1; distStep < numDistStep; ++distStep)
        //  {
        //    float curDist = distIncrement * distStep;
        //    svm::Vector<2> curVect;
        //    
        //    curVect[0] = dir.X() * curDist;
        //    curVect[1] = dir.Y() * curDist;
        //    //curVect *= fullInter.dist / GetMaxDist(fullInter.oneHotIdx);
        //
        //    corpus[idx].Add(curVect, -1);
        //  }
        //}
      }



      //Event endEvent;
      //std::atomic<unsigned int> curIndex;
      //curIndex = 0;
      //
      //unsigned int const numThreads = WorkerThread::GetHardwareConcurrency() - 2;
      //LearnThread* threads = (LearnThread*)malloc(numThreads * sizeof(LearnThread));
      //for(unsigned int i = 0; i< numThreads; ++i)
      //{
      //  new (threads + i) LearnThread(endEvent, corpus, planes, curIndex);
      //}
      //
      //endEvent.Reset(numThreads);
      //curIndex = 0;
      //for(unsigned int i = 0; i< numThreads; ++i)
      //{
      //  threads[i].Start(corpus.size());
      //}
      //
      //endEvent.Wait();
      //
      //for(unsigned int i = 0; i< numThreads; ++i)
      //{
      //  threads[i].Stop();
      //  threads[i].~LearnThread();
      //}
      //free(threads);
      //threads = nullptr;
      Vector<SVMModel::Kernel> kernels;

      for(unsigned int numSigma = 0; numSigma < SVMModel::s_NumSigma; ++numSigma)
      {
        kernels.push_back(SVMModel::Kernel(GetParams().m_KernelSigma));

        for(unsigned int curIdx = 0; curIdx < builder.oneHotSize; ++curIdx)
        {
          //svm::Vector<2> curVect;
          //curVect[0] = 0.0;
          //curVect[1] = 0.0;
          //curVect[2] = 0.0;

          //corpus[curIdx].Add(curVect, -1);

          String planePath("Hyperplane_");
          planePath.append(StringUtil::FromInt(numSigma));
          planePath.append("_");
          planePath.append(StringUtil::FromInt(curIdx));

          std::ifstream planeFile(StringUtil::ToASCII(planePath).c_str());
          bool readPlanes = false;

          //if(0 && planeFile.is_open())
          //{
          //  float dist;
          //  char sep;
          //  planeFile >> dist;
          //  planeFile.get(sep);
          //  if(dist == GetParams().m_MaxDist_)
          //  {
          //    planeFile >> planes[numSigma][curIdx];
          //    readPlanes = true;
          //  }
          //}
          
          if(!readPlanes)
          {
            //bool hasPresence = false;
            //for(unsigned int i = 0; i< corpus[curIdx].Size(); ++i)
            //{
            //  SVMModel::SVMVector* X;
            //  svm::Real y;
            //  corpus[curIdx].Get(i, X, y);
            //  if(y == 1)
            //  {
            //    hasPresence = true;
            //    break;
            //  }
            //}

            auto& curPlane = planes[numSigma][curIdx];
            curPlane.m_Kernel = kernels[numSigma];

            if(!corpus[curIdx].empty())
            {
              Clock timer;

              timer.GetTime();

              const uint32_t numVectors = corpus[curIdx].size() / (SVMModel::Dim + 1);

              auto& curCorpus = corpus[curIdx];

              Vector<double> classes(numVectors, 1);
              Vector<svm_node*> vectors(numVectors);
              svm_node* nodePtr = curCorpus.data();
              for(auto& ptr : vectors)
              {
                ptr = nodePtr;
                nodePtr += (SVMModel::Dim + 1);
              }

              svm_parameter params;

              params.svm_type = ONE_CLASS;
              params.kernel_type = RBF;
              params.gamma = 0.5 / (GetParams().m_KernelSigma * GetParams().m_KernelSigma);
              
              params.cache_size = 16;
              params.eps = 1.0e-6;
              //params.C = 1.0;
              params.nu = 0.1;
              params.shrinking = 0;
              params.probability = 0;

              svm_problem problem;

              problem.x = vectors.data();
              problem.l = numVectors;
              problem.y = classes.data();

              svm_model* model = svm_train(&problem, &params);
              eXl_ASSERT(model);

              //svm::smo::C_SVM<SVMModel::Dim,svm::smo::DefaultC_SVMParam, SVMModel::Kernel> c_svm(kernels[numSigma]);

              //c_svm.TrainExamples(corpus[curIdx]);
              //c_svm.MakeHyperplane(planes[numSigma][curIdx]);
              svm_save_model((StringUtil::ToASCII(planePath) + "_Ref").c_str(), model);

#ifdef EXP_METHOD
              curPlane.m_Bias = model->rho[0] * -1.0;
#else
              curPlane.m_Bias = 0.0;
#endif

              for(int i = 0; i<model->l; ++i)
              {
                SVMModel::Hyperplane::SupportVector supportVector;
                supportVector.m_Coeff = model->sv_coef[0][i];
                supportVector.m_Position[0] = model->SV[i][0].value;
                supportVector.m_Position[1] = model->SV[i][1].value;

#if SVM_VECTOR_SIZE == 3
                supportVector.m_Position[2] = model->SV[i][2].value;
#endif
                curPlane.m_Supports.push_back(supportVector);
              }

              svm_free_and_destroy_model(&model);
              //curPlane.supports.clear();
              //for(auto vec : vectors)
              //{
              //  svm::Vector<3> curVect;
              //  curVect[0] = vec[0].value;
              //  curVect[1] = vec[1].value;
              //  curVect[2] = vec[2].value;
              //  
              //  curPlane.supports.push_back(std::make_pair(0.2, curVect));
              //}

              float timeInSec = timer.GetTime();

              printf("Finished %i (%i elements) in %f seconds\n", curIdx, numVectors, timeInSec);
            }
            else
            {
              planes[numSigma][curIdx].m_Supports.clear();
            }


            //std::ofstream outPlane(StringUtil::ToASCII(planePath).c_str());
            //std::ostream& outStream = outPlane;
            //if(outPlane.is_open())
            //{
            //  outStream << GetParams().m_MaxDist_ << std::endl;
            //  outStream << planes[numSigma][curIdx];
            //}

          }

          if(planes[numSigma][curIdx].m_Supports.empty())
          {
            planes[numSigma][curIdx].m_Bias = 0;
            //planes[numSigma][curIdx].supports.push_back(std::make_pair(-1, SVMModel::SVMVector::Null()));
          }
          //auto& supports = planes[numSigma][curIdx].supports;
          //for(int i = 0; i<(int)(supports.size()); ++i)
          //{
          //  auto& vector = supports[i];
          //  if(vector.first < 0.0)
          //  {
          //    supports[i] = supports.back();
          //    supports.pop_back();
          //    --i;
          //  }
          //}
        }
      }

      SVMModel* model = new SVMModel(m_DistParams, GetElements(), GetParams().m_Toroidal);

      for(auto& elem : model->m_Elements)
      {
#ifdef EXP_METHOD
        elem.m_AbsDensity = exp(1);
#else
        elem.m_AbsDensity = 1.0;
#endif
      }

      for(unsigned int numSigma = 0; numSigma < SVMModel::s_NumSigma; ++numSigma)
      {
        for(unsigned int curIdx = 0; curIdx < builder.oneHotSize; ++curIdx)
        {
          model->m_Hyperplanes.push_back(planes[numSigma][curIdx]);
        }
      }
      if(m_Mask)
      {
        model->m_Mask.reset(m_Mask->Clone());
      }

      Refine(*model, numIter);

      ioModel = model;
    }

    bool SVMModel::CanSuggestPosition() const
    {
      return true;
    }

    bool SVMModel::SuggestPosition(Random& iRand, InputBuilder& iBuilder, PlacedElement const& iRefElem, PlacedElement& ioSuggestion) const
    {
      unsigned int const otherElem = ioSuggestion.m_Element;

      Vector2d baseMine(Mathd::Cos(iRefElem.m_Angle), Mathd::Sin(iRefElem.m_Angle));
      unsigned int const baseElem = iRefElem.m_Element;

      unsigned int const idx0 = baseElem > otherElem ? otherElem - 1 : baseElem - 1;
      unsigned int const idx1 = baseElem > otherElem ? baseElem - otherElem : otherElem - baseElem;

      //Vector2d baseOther(Mathd::Cos(currentElement.m_Angle), Mathd::Sin(currentElement.m_Angle));

      //inter.dir2 = GetLocal(baseMine, baseOther);
      //inter.dir2.Normalize();

      auto& inter = iBuilder.interMap[idx0][idx1];

      unsigned int turn = Mathi::Max(1, inter.second);
      float const angleInc = (Mathf::Pi() * 2.0) / turn;
      unsigned int const locTurn = iRand.Generate() % turn;
      float const locAngle = angleInc * locTurn;

      auto& plane = m_Hyperplanes[inter.first + locTurn];

      if(plane.m_Supports.empty())
      {
        return false;
      }

      uint32_t vectorIdx = iRand.Generate() % plane.m_Supports.size();
      auto const& supportVector = plane.m_Supports[vectorIdx];

      unsigned int const refShapeNum = baseElem > otherElem ? ioSuggestion.m_ShapeNum : iRefElem.m_ShapeNum;
      auto box = m_Elements[idx0].m_Shapes[refShapeNum].GetAABB();
      float const diag = box.GetSize().Length() * 0.5;
      float smallDim = Mathi::Min(box.GetSize().X(), box.GetSize().Y());
      float range = (box.GetSize() / 2).Length() - smallDim;

#if SVM_VECTOR_SIZE == 3
      float dist = (supportVector.m_Position[2] + 1) * 0.5 * GetMaxDist(iBuilder.BuildOneHot(idx0, idx1, locAngle));
      dist += smallDim + ((iRand() % 1000) / 1000.0) * range;
      float angle1 = MathTools::GetAngleFromVec(Vector2d(supportVector.m_Position[0] * 2, supportVector.m_Position[1] * 2));
#else
#error
#endif

      //float dist = Mathf::Max(0.0, sampleRbf.dist + smallDim + ((iRand() % 1000) / 1000.0) * range + RandNormFloatIn(iRand, sampleRbf.distEpsilon * -3, sampleRbf.distEpsilon * 3));
      //float angle1 = sampleRbf.angle1 + RandNormFloatIn(iRand, sampleRbf.dir1Epsilon * -3, sampleRbf.dir1Epsilon);
      
      Vector2d localOther(Mathd::Cos(locAngle), Mathd::Sin(locAngle));

      if(baseElem > otherElem)
      {
        Vector2d worldOther = baseMine;

        baseMine.X() = worldOther.X() * localOther.X() - worldOther.Y() * localOther.Y();
        baseMine.Y() = -worldOther.Y() * localOther.X() - worldOther.X() * localOther.Y();
        ioSuggestion.m_Angle = MathTools::GetAngleFromVec(baseMine);
        dist *= -1;
      }
      else
      {
        Vector2d worldOther = localOther.X() * baseMine + localOther.Y() * MathTools::GetPerp(baseMine);
        ioSuggestion.m_Angle = MathTools::GetAngleFromVec(worldOther);
      }
      ioSuggestion.m_Angle = Mathi::Round(ioSuggestion.m_Angle / angleInc) * angleInc;
      //currentElement.m_Angle = //fmod(refElem.m_Angle, Mathd::PI * 2.0);

      Vector2d dirVec = baseMine * Mathd::Cos(angle1) + MathTools::GetPerp(baseMine) * Mathd::Sin(angle1);
      dirVec *= dist;

      ioSuggestion.m_Pos = MathTools::ToIVec(MathTools::ToDVec(iRefElem.m_Pos) + dirVec);
      return true;
    }
  }
}