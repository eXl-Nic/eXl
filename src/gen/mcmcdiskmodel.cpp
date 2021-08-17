#include <gen/mcmcdiskmodel.hpp>
#include <math/mathtools.hpp>
#include <lbfgs.h>

namespace eXl
{
  namespace MCMC2D
  {
    static const String s_modelName = "MCMC2D::DiskModel";

    String const& DiskLearner::ModelName()
    {
      return s_modelName;
    }

    String const& DiskLearner::GetModelName() const
    {
      return s_modelName;
    }

    String const& DiskModel::GetModelName() const
    {
      return s_modelName;
    }

    LearnedModel* DiskLearner::UnstreamModel(Unstreamer& streamer) const
    {
      DiskModel* newModel = new DiskModel;

      newModel->Unstream(streamer);

      return newModel;
    }

    Err DiskModel::Stream(Streamer& streamer) const
    {
      streamer.BeginStruct();

      WriteCommon(streamer);

      streamer.PushKey("Elements");
      streamer.Write(&m_Elements);
      streamer.PopKey();
      streamer.PushKey("Params");
      streamer.Write(&m_Disks);
      streamer.PopKey();
      streamer.EndStruct();
      return Err::Success;
    }

    Err DiskModel::Unstream(Unstreamer& streamer)
    {
      streamer.BeginStruct();

      ReadCommon(streamer);

      streamer.PushKey("Elements");
      streamer.Read(&m_Elements);
      streamer.PopKey();
      streamer.PushKey("Params");
      streamer.Read(&m_Disks);
      streamer.PopKey();
      streamer.EndStruct();
      return Err::Success;
    }

    struct CellInfo
    {
      unsigned int element;
      float weight;
      bool isPres;
      Vector<unsigned int> interactionVars;
    };

    namespace
    {
      struct RegressionContext
      {
        Vector<CellInfo> cells;
        unsigned int numBeta;
        bool oneBeta = false;

        RegressionContext()
        {}


        static lbfgsfloatval_t _evaluate(
          void *instance,
          const lbfgsfloatval_t *x,
          lbfgsfloatval_t *g,
          const int n,
          const lbfgsfloatval_t step
        )
        {
          return reinterpret_cast<RegressionContext*>(instance)->evaluate(x, g, n, step);
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
            unsigned int const densityIdx = oneBeta ? 0 : cell.element;

            lbfgsfloatval_t density = x[densityIdx];
            lbfgsfloatval_t logLambda = density;
            for(auto& idx : cell.interactionVars)
            {
              logLambda += x[idx];
            }

            lbfgsfloatval_t lambda = exp(logLambda);

            if(cell.isPres)
            {
              presenceSum += logLambda;
            }

            absenceSum += cell.weight * lambda;

            lbfgsfloatval_t& betaDeriv = g[densityIdx];
            if(cell.isPres)
            {
              betaDeriv -= 1.0;
            }
            betaDeriv += cell.weight * lambda;

            for(auto& idx : cell.interactionVars)
            {
              if(cell.isPres)
              {
                g[idx] -= 1.0;
              }
              g[idx] += cell.weight * lambda;
            }
          }


          const float s_RegularisationCoeff = 0.01;

          lbfgsfloatval_t regularisationTerm = 0.0;
          for(int i = 0; i<n; ++i)
          {
            regularisationTerm += s_RegularisationCoeff * 0.5 * x[i] * x[i];
            g[i] += s_RegularisationCoeff * x[i];
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
          return reinterpret_cast<RegressionContext*>(instance)->progress(x, g, fx, xnorm, gnorm, step, n, k, ls);
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

    struct CompareDist
    {
      inline bool operator()(DiskDistribution::Quantile const& iQuant1, DiskDistribution::Quantile const& iQuant2) const
      {
        return iQuant1.m_Dist < iQuant2.m_Dist;
      }
    };

    void InsertNewVal(Vector<DiskDistribution::Quantile>& iVec, float iDist)
    {
      DiskDistribution::Quantile compVal = {0.0, iDist};
      auto iter = std::lower_bound(iVec.begin(), iVec.end(), compVal, CompareDist());

      if(iter != iVec.end() && Mathf::Abs(iter->m_Dist - iDist) < Mathf::EPSILON)
      {
        iter->m_Value += 1.0;
      }
      else
      {
        DiskDistribution::Quantile newQuant = {1.0, iDist};
        iVec.insert(iter, newQuant);
      }
    }

    void InsertInterDouble(DoubleDiskDistribution& oDisk, ElementInter const& iInter)
    {
      unsigned int section1 = oDisk.GetDisk(MathTools::GetAngleFromVec(iInter.dir1));
      unsigned int section2 = oDisk.m_Disks[section1].GetSection(MathTools::GetAngleFromVec(iInter.dir2));
      InsertNewVal(oDisk.m_Disks[section1].m_Distrib[section2], iInter.distance);
    }

    void InsertInter(Vector<DiskDistribution>& ioDisks, FullInteraction const& iInter)
    {
      auto& disk = ioDisks[iInter.oneHotIdx];

      unsigned int section = disk.GetSection(MathTools::GetAngleFromVec(iInter.dir1));
      InsertNewVal(disk.m_Distrib[section], iInter.dist);
    }

    float HandleInteraction(DiskLearnParams const& iParams, DiskDistribution const& iOldDisk, DiskDistribution& iNewDisk, DiskDistribution& iResampledDisk, float iMaxDist)
    {
      iNewDisk.m_Distrib.resize(iOldDisk.m_Distrib.size());
      for(unsigned int i = 0; i<iNewDisk.m_Distrib.size(); ++i)
      {
        Vector<DiskDistribution::Quantile>const& curSection = iOldDisk.m_Distrib[i];
        Vector<DiskDistribution::Quantile>& newSection = iNewDisk.m_Distrib[i];

        if(curSection.size() == 0)
        {
          DiskDistribution::Quantile quant = {0.0, iMaxDist};
          newSection.push_back(quant);
        }
        else 
        {
          float totNumVal = 0;
          for(unsigned int j = 0; j<curSection.size(); ++j)
          {
            totNumVal += curSection[j].m_Value;
          }

          unsigned int numQuantile = 0;
          float const quantileStep = 1.0f / float(iParams.quantile) * totNumVal;
          float curLimit = quantileStep;

          float prevQuantileVal = 0.0;
          float prevQuantileDist = 0.0;
          unsigned int jdist = 0;
          unsigned int jval = 0;
          float curAccumVal = curSection[0].m_Value;
          float curDist = curSection[0].m_Dist;
          float nextDist = 0.0;
          if(curDist > (0.1 / float(iParams.quantile)) * iMaxDist)
          {
            DiskDistribution::Quantile quant = {0.0f, curSection[0].m_Dist - (0.1f / float(iParams.quantile)) * iMaxDist};
            newSection.push_back(quant);
            prevQuantileDist = quant.m_Dist;
            //prevRemain = curSection[0].m_Dist - quant.m_Dist;
          }
          if(curSection.size() > 1)
          {
            ++jdist;
            nextDist = curSection[jdist].m_Dist;
          }
          else
          {
            nextDist = iMaxDist;
          }

          while(numQuantile < iParams.numQuantiles)
          {
            if(curAccumVal > curLimit)
            {
              float quantileDist = (quantileStep / (curAccumVal - prevQuantileVal)) * (nextDist - prevQuantileDist);
              //quantileDist = Mathf::Max(iMaxDist / 15.0, quantileDist);
              DiskDistribution::Quantile quant = {1.0, quantileDist};
              newSection.push_back(quant);
              prevQuantileDist = quantileDist;
              prevQuantileVal = curLimit;
              curLimit += quantileStep;
              ++numQuantile;
            }
            else
            {
              if(jdist + 1<curSection.size())
              {
                ++jdist;
                curDist = nextDist;
                nextDist = curSection[jdist].m_Dist;
              }
              else
              {
                curDist = iMaxDist;
              }
              if(jval + 1 < curSection.size())
              {
                curAccumVal += curSection[jval].m_Value;
                ++jval;
              }
              else
              {
                break;
              }
            }
          }
        }
      }

      float querySize = 0.0;
      for(auto section : iNewDisk.m_Distrib)
      {
        float curVal = 0.0;
        for(auto quantile : section)
        {
          curVal += quantile.m_Dist;
        }
        if(curVal > querySize)
          querySize = curVal;
      }
      querySize = Mathi::Ceil(querySize);
      iResampledDisk = iNewDisk;
      for(unsigned int section = 0; section<iOldDisk.m_Distrib.size(); ++section)
      {
        Vector<DiskDistribution::Quantile> const& curSection = iOldDisk.m_Distrib[section];
        Vector<DiskDistribution::Quantile>& finalDiskSection = iNewDisk.m_Distrib[section];
        Vector<DiskDistribution::Quantile>& resampledSection = iResampledDisk.m_Distrib[section];

        for(unsigned int j = 0; j<resampledSection.size(); ++j)
        {
          resampledSection[j].m_Value = 0.0;
        }

        for(unsigned int j = 0; j<curSection.size(); ++j)
        {
          int quantile = iNewDisk.GetQuantile(section, curSection[j].m_Dist);
          if(quantile >= 0 && iNewDisk.m_Distrib[section][quantile].m_Value > 0.0)
          {
            resampledSection[quantile].m_Value += curSection[j].m_Value;
          }
          else
          {
            break;
          }
        }
        for(unsigned int j = 0; j<resampledSection.size(); ++j)
        {
          if(resampledSection[j].m_Value == 0.0)
          {
            finalDiskSection[j].m_Value = 0.0;
          }
        }
      }
      return querySize;
    }

    float DiskModel::Sample(FullInteraction const& iInter)
    {
      return m_Disks[iInter.oneHotIdx].GetProba(iInter.dist, MathTools::GetAngleFromVec(iInter.dir1));
    }

    struct VarKey
    {
      unsigned int oneHotIdx;
      unsigned int sectionIdx;
      unsigned int quantileIdx;

      bool operator<(VarKey const& iOther) const
      {
        if(oneHotIdx == iOther.oneHotIdx)
        {
          if(sectionIdx == iOther.sectionIdx)
          {
            return quantileIdx < iOther.quantileIdx;
          }
          return sectionIdx < iOther.sectionIdx;
        }
        return oneHotIdx < iOther.oneHotIdx;
      }
    };

    bool GetVarKey(FullInteraction const& iInter, Vector<DiskDistribution> const& iDisks, VarKey& oKey)
    {
      auto const& disk = iDisks[iInter.oneHotIdx];
      unsigned int const sectionIdx = disk.GetSection(MathTools::GetAngleFromVec(iInter.dir1));
      int quantileIdx = disk.GetQuantile(sectionIdx, iInter.dist);
      if(quantileIdx >= 0)
      {
        oKey.oneHotIdx = iInter.oneHotIdx;
        oKey.sectionIdx = sectionIdx;
        oKey.quantileIdx = quantileIdx;
        return true;
      }
      return false;
    }

    void DiskLearner::Refine(LearnedModel& ioModel, unsigned int const numIter)
    {
      InputBuilder& builder = GetBuilder();
      auto& cells = GetCells();

      DiskModel& model = static_cast<DiskModel&>(ioModel);

      Vector<DiskDistribution>& valDisk = model.m_Disks;

      Map<VarKey, unsigned int> varMap;
      unsigned int numVars = GetElements().size();
      VarKey curKey;
      curKey.oneHotIdx = 0;
      curKey.sectionIdx = 0;
      curKey.quantileIdx = 0;
      for(auto const& disk : valDisk)
      {
        for(auto const& section : disk.m_Distrib)
        {
          for(auto const& quantile : section)
          {
            if(quantile.m_Value != 0.0)
            {
              varMap.insert(std::make_pair(curKey, numVars));
              ++numVars;
            }
            ++curKey.quantileIdx;
          }
          ++curKey.sectionIdx;
          curKey.quantileIdx = 0;
        }
        ++curKey.oneHotIdx;
        curKey.quantileIdx = 0;
        curKey.sectionIdx = 0;
      }

      RegressionContext ctx;
      ctx.numBeta = GetElements().size();
      ctx.oneBeta = false;

      for(auto& cell : cells)
      {
        CellInfo curCell;
        curCell.element = cell.elem1;
        curCell.weight = cell.weight;
        curCell.isPres = cell.isPresence;
        for(auto& inter : cell.interactions)
        {
          FullInteraction fullInter = FullInteraction::Make(builder, cell, inter);

          VarKey key;
          if(GetVarKey(fullInter, valDisk, key))
          {
            auto var = varMap.find(key);
            if(var != varMap.end())
            {
              curCell.interactionVars.push_back(var->second);
            }
          }
        }
        ctx.cells.push_back(curCell);
      }

      lbfgsfloatval_t* x = lbfgs_malloc(numVars);

      memset(x, 0, sizeof(lbfgsfloatval_t) * numVars);

      {
        lbfgsfloatval_t* xIter = x;
        lbfgsfloatval_t* xIterEnd = x + numVars;

        int i = 0;
        for(; xIter != x + ctx.numBeta; ++xIter, ++i)
        {
          *xIter = log(m_Elements[i].m_AbsDensity);
        }

        for(auto& disk : valDisk)
        {
          for(auto& section : disk.m_Distrib)
          {
            for(auto& quantile : section)
            {
              if(quantile.m_Value != 0.0)
              {
                *xIter = log(quantile.m_Value);
                ++xIter;
              }
              else
              {
                quantile.m_Value = -std::numeric_limits<float>::infinity();
              }
            }
          }
        }
      }

      lbfgsfloatval_t fx;
      lbfgs_parameter_t params;
      lbfgs_parameter_init(&params);
#ifdef _DEBUG
      params.max_iterations = 200;
#else
      params.max_iterations = 2500;
#endif
      lbfgs(numVars, x, &fx, &RegressionContext::_evaluate, &RegressionContext::_progress, &ctx, &params);

      

      {
        lbfgsfloatval_t const* xIter = x;
        Vector<Element>& elements = model.m_Elements;
        for(auto& elem : elements)
        {
          elem.m_AbsDensity = exp(*xIter);
          ++xIter;
        }

        for(auto& disk : valDisk)
        {
          for(auto& section : disk.m_Distrib)
          {
            for(auto& quantile : section)
            {
              if(quantile.m_Value != -std::numeric_limits<float>::infinity())
              {
                quantile.m_Value = exp(*xIter);
                ++xIter;
              }
              else
              {
                quantile.m_Value = 0.0;
              }
            }
          }
        }
      }

      lbfgs_free(x);
      x = nullptr;
    }

    void DiskLearner::Learn(LearnedModel*& ioModel, /*Vector<*/LearnExample/*>*/ const& iExamples, unsigned int const numIter, Debug* iDebug)
    {
      m_Params.m_QuantileCull = false;
      BuildCells(iExamples);

      //if(iDebug)
      //{
      //  iDebug->GetSamplingCells(GetCells(), cells);
      //}

      InputBuilder& builder = GetBuilder();
      auto& cells = GetCells();

      DiskDistribution dummyDisk;
      dummyDisk.m_Distrib.resize(GetParams().angleSubdiv);

      Vector<DiskDistribution> diskPres(builder.oneHotSize, dummyDisk);
      Vector<DiskDistribution> resampledDisk = diskPres;
      Vector<DiskDistribution> valDisk = diskPres;

      Vector<float> absCellWeight;

      for(auto& cell : cells)
      {
        if(cell.isPresence)
        {
          for(auto& inter : cell.interactions)
          {
            FullInteraction fullInter = FullInteraction::Make(builder, cell, inter);
            
            InsertInter(diskPres, fullInter);
          }
        }
        else
        {
          absCellWeight.push_back(cell.weight);
        }
      }

      for(unsigned int i = 0; i<diskPres.size(); ++i)
      {
        HandleInteraction(GetParams(), diskPres[i], valDisk[i], resampledDisk[i], GetMaxDist(i));
      }

      for(auto& disk : valDisk)
      {
        for(auto& section : disk.m_Distrib)
        {
          for(auto& quantile : section)
          {
            if(quantile.m_Value != 0.0)
            {
              quantile.m_Value = 1.0;
            }
          }
        }
      }

      DiskModel* model = new DiskModel(m_DistParams, GetElements(), GetParams().m_Toroidal);

      model->m_Disks = std::move(valDisk);

      for(auto& element : model->m_Elements)
      {
        element.m_AbsDensity = exp(1);
      }

      ioModel = model;

      Refine(*model, numIter);
    }
  }
}