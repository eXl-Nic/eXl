/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>
#include <math/math.hpp>
namespace eXl
{
  class Random;

  class EXL_GEN_API Perlin
  {
  public:

    Perlin() {};
    Perlin(unsigned int iDim, Random& iRand);
    Perlin(Vec2i iDim, Random& iRand);
    Perlin(Vec3i iDim, Random& iRand);

    struct FractalParameters
    {
      uint32_t octaves = 4;
      float persistence = 0.5;
      float period = 2.0;
    };

    float GetFractal(float iCoord, FractalParameters const& iParams);
    float GetFractal(Vec2 iCoord, FractalParameters const& iParams);
    float GetFractal(Vec3 iCoord, FractalParameters const& iParams);

    float Get(float iCoord);
    float Get(Vec2 iCoord);
    float Get(Vec3 iCoord);

    void Reset(uint32_t iDim, Random& iRand);
    void Reset(Vec2i iDim, Random& iRand);
    void Reset(Vec3i iDim, Random& iRand);

  protected:

    void _Build(Vec3i iSize, Random& iRand);

    float _GetFractal(Vec3 iCoord, FractalParameters const& iParams);
    float _Get(Vec3 iCoord, unsigned int iIter = 0);

    Vector<float> m_Grid;
    Vec3i      m_Size;
    unsigned int  m_Dimension;
    Vector<float> m_DotProds;
  };
}