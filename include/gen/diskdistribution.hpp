/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>
#include <core/containers.hpp>
#include <math/vector3.hpp>

namespace eXl
{
  class Streamer;
  class Unstreamer;

  class EXL_GEN_API DiskDistribution
  {
  public:

    float GetProba(float iDist, float iAngle) const;

    unsigned int GetSection(float iAngle) const;
    int GetQuantile(unsigned int iSection, float iDist) const;

    void ClearValues();

    Err Unstream(Unstreamer& iUnstreamer);
    Err Stream(Streamer& iStreamer) const;

    struct Quantile
    {
      float m_Value;
      float m_Dist;
    };

    Vector<Vector<Quantile> > m_Distrib;
  protected:
    inline unsigned int _GetSection(float iAngle) const;
    inline int _GetQuantile(unsigned int iSection, float iDist) const;
  };

  class EXL_GEN_API DoubleDiskDistribution
  {
  public:

    float GetProba(float iDist, float iAngle1, float iAngle2) const;

    unsigned int GetDisk(float iAngle) const;

    int GetQuantile(unsigned int iSection1, unsigned int iSection2, float iDist) const;

    void ClearValues();

    void Unstream(Unstreamer& iUnstreamer);
    void Stream(Streamer& iStreamer) const;

    Vector<DiskDistribution> m_Disks;
  protected:
    inline unsigned int _GetDisk(float iAngle) const;
    inline int _GetQuantile(unsigned int iSection1, unsigned int iSection2, float iDist) const;
  };

  class DiskFuncEval
  {
  public:

    void Build(DoubleDiskDistribution const& iDisk, float iMaxLimit);

    void Build(DiskDistribution const& iDisk, float iMaxLimit);

    float Eval(float iDist, float iAngle1, float iAngle2);

  protected:

    void _Build(DiskDistribution const& iDisk, float iMaxLimit, float iCurAngle1, float iCurGamma1);

    struct RBFParams
    {
      Vector3f m_Center;
      Vector3f m_Gamma;
      float m_Weight;

      Vector3f GetDist(float x, float y, float z) const;
    };

    Vector<RBFParams> m_Functions;
    Vector<RBFParams> m_Limits;
  };
}
