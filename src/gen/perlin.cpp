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
    _Build(Vector3i(iDim, 0, 0), iRand);
  }

  Perlin::Perlin(Vector2i iDim, Random& iRand)
  {
    _Build(Vector3i(iDim.X(), iDim.Y(), 0), iRand);
  }

  Perlin::Perlin(Vector3i iDim, Random& iRand)
  {
    _Build(iDim, iRand);
  }

  void Perlin::Reset(uint32_t iDim, Random& iRand)
  {
    _Build(Vector3i(iDim, 0, 0), iRand);
  }

  void Perlin::Reset(Vector2i iDim, Random& iRand)
  {
    _Build(Vector3i(iDim.X(), iDim.Y(), 0), iRand);
  }

  void Perlin::Reset(Vector3i iDim, Random& iRand)
  {
    _Build(iDim, iRand);
  }

  float Perlin::Get(float iCoord)
  {
    return _Get(Vector3f(iCoord, 0.0, 0.0));
  }

  float Perlin::Get(Vector2f iCoord)
  {
    return _Get(Vector3f(iCoord.X(), iCoord.Y(), 0.0));
  }

  float Perlin::Get(Vector3f iCoord)
  {
    return _Get(iCoord) ;
  }

  float Perlin::GetFractal(float iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(Vector3f(iCoord, 0.0, 0.0), iParams);
  }

  float Perlin::GetFractal(Vector2f iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(Vector3f(iCoord.X(), iCoord.Y(), 0.0), iParams);
  }

  float Perlin::GetFractal(Vector3f iCoord, FractalParameters const& iParams)
  {
    return _GetFractal(iCoord, iParams);
  }

  void Perlin::_Build(Vector3i iSize, Random& iRand)
  {
    m_Size = Vector3i(Mathi::Max(iSize.X(), 0), Mathi::Max(iSize.Y(), 0), Mathi::Max(iSize.Z(), 0));

    m_Dimension = m_Size.X() != 0 ? 1 : 0 ;
    m_Dimension += m_Dimension * (m_Size.Y() != 0 ? 1 : 0);
    m_Dimension += (m_Dimension/2) * (m_Size.Z() != 0 ? 1 : 0);
    eXl_ASSERT(m_Dimension > 0 && m_Dimension <= 3);
    if(m_Dimension > 0)
    {
      unsigned int numDots = 1<<m_Dimension;

      m_DotProds.resize(numDots);

      unsigned int vecSize = m_Dimension;
      for(unsigned int i = 0; i < m_Dimension; ++i)
      {
        vecSize *= m_Size.m_Data[i];
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
          Vector2f* vec = reinterpret_cast<Vector2f*>(m_Grid.data() + 2*i);
          vec->X() = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->Y() = (1.0 - (iRand.Generate() % 2) * 2.0) * Mathf::Sqrt(1.0 - vec->X() * vec->X());
        }
        break;
      case 3:
        for(unsigned int i = 0; i<vecSize / 3; ++i)
        {
          Vector3f* vec = reinterpret_cast<Vector3f*>(m_Grid.data() + 3*i);
          vec->X() = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->Y() = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->Z() = (int(iRand.Generate() % 20000) - 10000) / float(10000);
          vec->Normalize();
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

  float Perlin::_GetFractal(Vector3f iCoord, FractalParameters const& iParams)
  {
    if (iParams.period < Mathf::ZERO_TOLERANCE
      || iParams.persistence < Mathf::ZERO_TOLERANCE
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

  float Perlin::_Get(Vector3f iCoord, uint32_t iIter)
  {
    unsigned int xSteps = m_Dimension > 0 ? 2 : 0;
    unsigned int ySteps = m_Dimension > 1 ? 2 : 1;
    unsigned int zSteps = m_Dimension > 2 ? 2 : 1;
    
    unsigned int zFactor = m_Size.X() * m_Size.Y();
    unsigned int yFactor = m_Size.X();

    Vector3i extPoints[2];
    extPoints[0] = Vector3i(iCoord.X(), iCoord.Y(), iCoord.Z());
    for(unsigned int dim = 0; dim < m_Dimension; ++dim)
    {
      extPoints[1].m_Data[dim] = extPoints[0].m_Data[dim] + 1;
    }
    Vector3f wheight(Mathf::SCurve3(iCoord.X() - extPoints[0].X()), 
      Mathf::SCurve3(iCoord.Y() - extPoints[0].Y()), 
      Mathf::SCurve3(iCoord.Z() - extPoints[0].Z()));
    Vector3f res;
    Vector3i curPt;
    for(unsigned int z = 0; z<zSteps; ++z)
    {
      for(unsigned int y = 0; y<ySteps; ++y)
      {
        for(unsigned int x = 0; x<xSteps; ++x)
        {
          Vector3i curPt(extPoints[x].X(), extPoints[y].Y(), extPoints[z].Z());
          Vector3i sampleCoords = curPt;
          for (uint32_t dim = 0; dim < m_Dimension; ++dim)
          {
            if (iIter % (1 << (dim + 1)) >= (1 << dim))
            {
              sampleCoords.m_Data[dim] = m_Size.m_Data[dim] - 1 - sampleCoords.m_Data[dim];
            }
          }
          for (unsigned int dim = 0; dim < m_Dimension; ++dim)
          {
            if (m_Size.m_Data[dim] != 0)
              sampleCoords.m_Data[dim] = Mathi::Max(curPt.m_Data[dim], 0.0) % m_Size.m_Data[dim];
            else
              sampleCoords.m_Data[dim] = 0;
          }

          float* dotProd = m_DotProds.data() + 4 * z + 2 * y + x;
          *dotProd = 0.0;
          float* basePtr = m_Grid.data() + m_Dimension * (sampleCoords.Z() * zFactor + sampleCoords.Y() * yFactor + sampleCoords.X());
          for(unsigned int dim = 0; dim < m_Dimension; ++dim)
          {
            (*dotProd) += (curPt.m_Data[dim] - iCoord.m_Data[dim]) * (*basePtr);
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
        m_DotProds[i] = Lerp(m_DotProds[2 * i + 0], m_DotProds[2 * i + 1], wheight.m_Data[whOffset]);
      }
      ++whOffset;
      numIter /= 2;
    }
    float value = (m_DotProds[0] * Mathf::Sqrt(m_Dimension));

    return value;
  }
}