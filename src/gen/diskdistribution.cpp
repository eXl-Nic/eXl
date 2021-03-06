/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/diskdistribution.hpp>
#include <math/math.hpp>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

namespace eXl
{

  unsigned int DiskDistribution::GetSection(float iAngle) const
  {
    return _GetSection(iAngle);
  }

  int DiskDistribution::GetQuantile(unsigned int iSection, float iDist) const
  {
    return _GetQuantile(iSection, iDist);
  }

  void DiskDistribution::ClearValues()
  {
    for(unsigned int i = 0; i<m_Distrib.size(); ++i)
    {
      for(unsigned int j = 0; j<m_Distrib[i].size(); ++j)
      {
        m_Distrib[i][j].m_Value = 0.0;
      }
    }
  }

  inline unsigned int DiskDistribution::_GetSection(float iAngle) const
  {
    iAngle += (Mathf::Pi() / m_Distrib.size());
    int div = iAngle / (Mathf::Pi() * 2);
    if(iAngle < 0)
    {
      --div;
    }
    iAngle -= div * Mathf::Pi() * 2;

    return (unsigned int)((iAngle / (Mathf::Pi() * 2)) * m_Distrib.size()) % m_Distrib.size();
  }

  inline int DiskDistribution::_GetQuantile(unsigned int iSection, float iDist) const
  {
    float curDist =  0.0;
    int quantile = -1;
    
    while((quantile + 1) < m_Distrib[iSection].size())
    {
      ++quantile;
      curDist += m_Distrib[iSection][quantile].m_Dist;
      if(curDist >= iDist)
      {
        return quantile;
      }
    }

    return -1;
  }

  float DiskDistribution::GetProba(float iDist, float iAngle) const
  {
    if(m_Distrib.size() == 0)
      return 1.0;

    int section = GetSection(iAngle);

    //if(m_Distrib[section].size() == 0)
    //  return 1.0;
    //else
    //{

    int quantile = _GetQuantile(section, iDist);
    if(quantile >= 0)
    {
      return m_Distrib[section][quantile].m_Value;
    }

    return 1.0;
  }

  Err DiskDistribution::Unstream(Unstreamer& iUnstreamer)
  {
    //iUnstreamer.BeginStruct();
    //iUnstreamer.PushKey("Distrib");
    Err seq1 = iUnstreamer.BeginSequence();
    if((seq1))
    {
      do
      {
        m_Distrib.push_back(eXl::Vector<Quantile>());
        Err seq2 = iUnstreamer.BeginSequence();
        if((seq2))
        {
          do
          {
            Quantile curQuant;
            iUnstreamer.BeginStruct();
            iUnstreamer.PushKey("Value");
            iUnstreamer.ReadFloat(&curQuant.m_Value);
            iUnstreamer.PopKey();
            iUnstreamer.PushKey("Distance");
            iUnstreamer.ReadFloat(&curQuant.m_Dist);
            iUnstreamer.PopKey();
            iUnstreamer.BeginStruct();
            m_Distrib.back().push_back(curQuant);
          }while((seq2 = iUnstreamer.NextSequenceElement()));
        }
      }while((seq1 = iUnstreamer.NextSequenceElement()));
    }
    //iUnstreamer.PopKey();
    //iUnstreamer.EndStruct();

    return Err::Success;
  }

  Err DiskDistribution::Stream(Streamer& iStreamer) const
  {
    //iStreamer.BeginStruct();
    //iStreamer.PushKey("Distrib");
    iStreamer.BeginSequence();
    for(unsigned int i = 0; i<m_Distrib.size(); ++i)
    {
      iStreamer.BeginSequence();
      for(unsigned int j = 0; j<m_Distrib[i].size(); ++j)
      {
        iStreamer.BeginStruct();
        iStreamer.PushKey("Value");
        iStreamer.WriteFloat(&m_Distrib[i][j].m_Value);
        iStreamer.PopKey();
        iStreamer.PushKey("Distance");
        iStreamer.WriteFloat(&m_Distrib[i][j].m_Dist);
        iStreamer.PopKey();
        iStreamer.EndStruct();
      }
      iStreamer.EndSequence();
    }
    iStreamer.EndSequence();
    //iStreamer.PopKey();
    //iStreamer.EndStruct();

    return Err::Success;
  }


  float DoubleDiskDistribution::GetProba(float iDist, float iAngle1, float iAngle2) const
  {
    if(m_Disks.size() == 0)
      return 1.0;

    unsigned int i = _GetDisk(iAngle1);

    return m_Disks[i].GetProba(iDist, iAngle2);
  }

  unsigned int DoubleDiskDistribution::GetDisk(float iAngle) const
  {
    return _GetDisk(iAngle);
  }

  int DoubleDiskDistribution::GetQuantile(unsigned int iSection1, unsigned int iSection2, float iDist) const
  {
    return _GetQuantile(iSection1, iSection2, iDist);
  }

  void DoubleDiskDistribution::ClearValues()
  {
    for(auto& disk : m_Disks)
      disk.ClearValues();
  }

  void DoubleDiskDistribution::Unstream(Unstreamer& iUnstreamer)
  {
    Err seq1 = iUnstreamer.BeginSequence();
    if((seq1))
    {
      do
      {
        m_Disks.push_back(DiskDistribution());
        m_Disks.back().Unstream(iUnstreamer);
        
      }while((seq1 = iUnstreamer.NextSequenceElement()));
    }
  }

  void DoubleDiskDistribution::Stream(Streamer& iStreamer) const
  {
    iStreamer.BeginSequence();
    for(unsigned int i = 0; i<m_Disks.size(); ++i)
    {
      m_Disks[i].Stream(iStreamer);
    }
    iStreamer.EndSequence();
  }

  inline unsigned int DoubleDiskDistribution::_GetDisk(float iAngle) const
  {
    iAngle += (Mathf::Pi() / m_Disks.size());
    int div = iAngle / (Mathf::Pi() * 2);
    if(iAngle < 0)
    {
      --div;
    }
    iAngle -= div * Mathf::Pi() * 2;

    return (iAngle / (Mathf::Pi() * 2)) * m_Disks.size();
  }
  
  inline int DoubleDiskDistribution::_GetQuantile(unsigned int iSection1, unsigned int iSection2, float iDist) const
  {
    if(iSection1 < m_Disks.size())
    {
      DiskDistribution const& disk = m_Disks[iSection1];
      return disk.GetQuantile(iSection2, iDist);
    }
    return -1;
  }

  void DiskFuncEval::Build(DoubleDiskDistribution const& iDisk, float iMaxLimit)
  {
    float curAngleStep1 = 2.0 * Mathf::Pi() / iDisk.m_Disks.size();
    float curAngle1 = 0.0;
    for(auto& disk : iDisk.m_Disks)
    {
      _Build(disk, iMaxLimit, curAngle1, curAngleStep1 / 3.0);
      curAngle1 += curAngleStep1;
    }
  }

  void DiskFuncEval::Build(DiskDistribution const& iDisk, float iMaxLimit)
  {
    _Build(iDisk, iMaxLimit, 0.0, 1.0);
  }

  void DiskFuncEval::_Build(DiskDistribution const& iDisk, float iMaxLimit, float iCurAngle1, float iCurGamma1)
  {
    float curAngleStep2 = 2.0 * Mathf::Pi() / iDisk.m_Distrib.size();
    float curAngle2 = 0.0;
    for(auto& section : iDisk.m_Distrib)
    {
      float curDist = 0.0;
      for(auto& quantile : section)
      {
        RBFParams params;
        params.m_Center = Vec3(curDist + (quantile.m_Dist / 2.0), iCurAngle1, curAngle2);
        params.m_Gamma = Vec3(quantile.m_Dist / 3, iCurGamma1, curAngleStep2 / 3) * 2;
        params.m_Weight = quantile.m_Value;

        m_Functions.push_back(params);

        curDist += quantile.m_Dist;
      }
      if(curDist + Mathf::Epsilon() < iMaxLimit)
      {
        RBFParams params;
        params.m_Center = Vec3(curDist, iCurAngle1, curAngle2);
        params.m_Gamma = Vec3(10.0 * iMaxLimit, iCurGamma1, curAngleStep2 / 3) * 2;
        params.m_Weight = 1.0;
        m_Limits.push_back(params);
      }
      curAngle2 += curAngleStep2;
    }
  }

  float AngleDist(float iAngle1, float iAngle2)
  {
    float dist = iAngle1 - iAngle2;
    if(dist > Mathf::Pi())
    {
      return 2.0 * Mathf::Pi() - dist;
    }
    else if(dist < -Mathf::Pi())
    {
      return dist + 2.0 * Mathf::Pi();
    }
    return dist;
  }

  Vec3 DiskFuncEval::RBFParams::GetDist(float x, float y, float z) const
  {
    Vec3 dist(m_Center.x - x, 
      AngleDist(y, m_Center.y), 
      AngleDist(z, m_Center.z));
    dist.x *= dist.x;
    dist.y *= dist.y;
    dist.z *= dist.z;

    return dist;
  }

  float DiskFuncEval::Eval(float iDist, float iAngle1, float iAngle2)
  {
    int div = iAngle1 / (Mathf::Pi() * 2);
    if(iAngle1 < 0)
    {
      --div;
    }
    iAngle1 -= div * Mathf::Pi() * 2;

    div = iAngle2 / (Mathf::Pi() * 2);
    if(iAngle2 < 0)
    {
      --div;
    }
    iAngle2 -= div * Mathf::Pi() * 2;

    float score = 0.0;
    float nullMult = 1.0;

    for(auto& function : m_Functions)
    {
      Vec3 const& gamma = function.m_Gamma;
      Vec3 dist = function.GetDist(iDist, iAngle1, iAngle2);
      float distCoeff = exp( - (dist.x / (2*gamma.x * gamma.x) + dist.y / (2*gamma.y * gamma.y) + dist.z / (2*gamma.z * gamma.z)));

      if(function.m_Weight == 0.0)
      {
        nullMult *= (1.0 - distCoeff);
      }
      else
      {
        distCoeff *= function.m_Weight / (pow(2.0 * Mathf::Pi(), 1.5f) * gamma.x * gamma.y * gamma.z);
        score += distCoeff;
      }
    }

    for(auto& function : m_Limits)
    {
      Vec3 const& gamma = function.m_Gamma;
      Vec3 dist = function.GetDist(iDist, iAngle1, iAngle2);
      float distCoeff = exp( - (dist.y / (2*gamma.y * gamma.y) + dist.z / (2*gamma.z * gamma.z)));
      distCoeff /= (1.0 + exp(gamma.x * (function.m_Center.x - iDist)));
      distCoeff *= function.m_Weight / (2.0 * Mathf::Pi() * gamma.y * gamma.z);
      score += distCoeff;
    }

    return score * nullMult;
  }

}