/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <gen/perlin.hpp>
#include <core/random.hpp>
#include <cfloat>

namespace eXl
{
  Perlin::Perlin(unsigned int iDim, Random& iRand)
  {
    _Build(Vec3i(iDim, 0, 0), iRand);
  }

  Perlin::Perlin(Vec2i iDim, Random& iRand)
  {
    _Build(Vec3i(iDim.x, iDim.y, 0), iRand);
  }

  Perlin::Perlin(Vec3i iDim, Random& iRand)
  {
    _Build(iDim, iRand);
  }

  void Perlin::Reset(uint32_t iDim, Random& iRand)
  {
    _Build(Vec3i(iDim, 0, 0), iRand);
  }

  void Perlin::Reset(Vec2i iDim, Random& iRand)
  {
    _Build(Vec3i(iDim.x, iDim.y, 0), iRand);
  }

  void Perlin::Reset(Vec3i iDim, Random& iRand)
  {
    _Build(iDim, iRand);
  }

  float Perlin::Get(float iCoord)
  {
    return _Get(Vec3(iCoord, 0.0, 0.0));
  }

  float Perlin::Get(Vec2 iCoord)
  {
    return _Get(Vec3(iCoord.x, iCoord.y, 0.0));
  }

  float Perlin::Get(Vec3 iCoord)
  {
    return _Get(iCoord) ;
  }

  float Perlin::GetFractal(float iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(Vec3(iCoord, 0.0, 0.0), iParams);
  }

  float Perlin::GetFractal(Vec2 iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(Vec3(iCoord.x, iCoord.y, 0.0), iParams);
  }

  float Perlin::GetFractal(Vec3 iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(iCoord, iParams);
  }

  void Perlin::_Build(Vec3i iSize, Random& iRand)
  {
    m_Size = Vec3i(Mathi::Max(iSize.x, 0), Mathi::Max(iSize.y, 0), Mathi::Max(iSize.z, 0));

    m_Dimension = m_Size.x != 0 ? 1 : 0 ;
    m_Dimension += m_Dimension * (m_Size.y != 0 ? 1 : 0);
    m_Dimension += (m_Dimension/2) * (m_Size.z != 0 ? 1 : 0);
    eXl_ASSERT(m_Dimension > 0 && m_Dimension <= 3);
    if(m_Dimension > 0)
    {
      unsigned int numDots = 1<<m_Dimension;

      m_DotProds.resize(numDots);

      unsigned int vecSize = m_Dimension;
      for(unsigned int i = 0; i < m_Dimension; ++i)
      {
        vecSize *= m_Size[i];
      }
      m_Grid.resize(vecSize);
      switch(m_Dimension)
      {
      case 1:
        for(unsigned int i = 0; i<vecSize; ++i)
        {
          m_Grid[i] = (int(iRand.Generate() % 20000) - 10000) / float(10000);
        }
        break;
      case 2:
        for(unsigned int i = 0; i<vecSize / 2; ++i)
        {
          Vec2* vec = reinterpret_cast<Vec2*>(m_Grid.data() + 2*i);
          vec->x = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->y = (1.0 - (iRand.Generate() % 2) * 2.0) * Mathf::Sqrt(1.0 - vec->x * vec->x);
        }
        break;
      case 3:
        for(unsigned int i = 0; i<vecSize / 3; ++i)
        {
          Vec3* vec = reinterpret_cast<Vec3*>(m_Grid.data() + 3*i);
          vec->x = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->y = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->z = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          *vec = normalize(*vec);
        }
        break;
      }
    }
  }

  namespace
  {
    float Lerp(float iVal1, float iVal2, float iAlpha)
    {
      return iAlpha * iVal2 + (1.0 - iAlpha) * iVal1;
    }
  }

  float Perlin::_GetFractal(Vec3 iCoord, FractalParameters const& iParams)
  {
    if (iParams.period < Mathf::ZeroTolerance()
      || iParams.persistence < Mathf::ZeroTolerance()
      || iParams.octaves == 0)
    {
      return 0;
    }

    
    float value = 0.0;
    float totAmpl = 0.0;
    float amplitude = 1.0;
    float period = 1;
    for (uint32_t curOct = 0; curOct < iParams.octaves; ++curOct)
    {
      value += amplitude * _Get(iCoord * period);
      
      totAmpl += amplitude;
      amplitude *= iParams.persistence;
      period *= iParams.period;
    }

    value = (value /*/ totAmpl*/);

    static float minValue = FLT_MAX;
    static float maxValue = FLT_MIN;

    return Mathf::Clamp(value, -1.0, 1.0);
  }

  float Perlin::_Get(Vec3 iCoord, uint32_t iIter)
  {
    unsigned int xSteps = m_Dimension > 0 ? 2 : 0;
    unsigned int ySteps = m_Dimension > 1 ? 2 : 1;
    unsigned int zSteps = m_Dimension > 2 ? 2 : 1;
    
    unsigned int zFactor = m_Size.x * m_Size.y;
    unsigned int yFactor = m_Size.x;

    Vec3i extPoints[2];
    extPoints[0] = Vec3i(iCoord.x, iCoord.y, iCoord.z);
    for(unsigned int dim = 0; dim < m_Dimension; ++dim)
    {
      extPoints[1][dim] = extPoints[0][dim] + 1;
    }
    Vec3 wheight(Mathf::SCurve3(iCoord.x - extPoints[0].x), 
      Mathf::SCurve3(iCoord.y - extPoints[0].y), 
      Mathf::SCurve3(iCoord.z - extPoints[0].z));

    for(unsigned int z = 0; z<zSteps; ++z)
    {
      for(unsigned int y = 0; y<ySteps; ++y)
      {
        for(unsigned int x = 0; x<xSteps; ++x)
        {
          Vec3i curPt(extPoints[x].x, extPoints[y].y, extPoints[z].z);
          Vec3i sampleCoords = curPt;
          for (uint32_t dim = 0; dim < m_Dimension; ++dim)
          {
            if (iIter % (1 << (dim + 1)) >= (1 << dim))
            {
              sampleCoords[dim] = m_Size[dim] - 1 - sampleCoords[dim];
            }
          }
          for (unsigned int dim = 0; dim < m_Dimension; ++dim)
          {
            if (m_Size[dim] != 0)
              sampleCoords[dim] = Mathi::Max(curPt[dim], 0.0) % m_Size[dim];
            else
              sampleCoords[dim] = 0;
          }

          float* dotProd = m_DotProds.data() + 4 * z + 2 * y + x;
          *dotProd = 0.0;
          float* basePtr = m_Grid.data() + m_Dimension * (sampleCoords.z * zFactor + sampleCoords.y * yFactor + sampleCoords.x);
          for(unsigned int dim = 0; dim < m_Dimension; ++dim)
          {
            (*dotProd) += (curPt[dim] - iCoord[dim]) * (*basePtr);
            ++basePtr;
          }
        }
      }
    }
    unsigned int whOffset = 0;
    unsigned int numIter = 1 << (m_Dimension - 1);
    for(unsigned int dim = 0; dim < m_Dimension; ++dim)
    {
      for(unsigned int i = 0; i<numIter; ++i)
      {
        m_DotProds[i] = Lerp(m_DotProds[2 * i + 0], m_DotProds[2 * i + 1], wheight[whOffset]);
      }
      ++whOffset;
      numIter /= 2;
    }
    float value = (m_DotProds[0] * Mathf::Sqrt(m_Dimension));

    return value;
  }
}