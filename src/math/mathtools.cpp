/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <core/stream/serializer.hpp>
#include <math/mathtools.hpp>
#include <math/polygon.hpp>
#include <math/geometrytraits.hpp>

namespace eXl
{
  template <>
  void MathTools::SimplifyPolygon<int>(Polygoni const& iPoly, double iEpsilon, Polygoni& oPoly)
  {
    boost::geometry::simplify(iPoly, oPoly, iEpsilon);
  }

  uint32_t Factorial(uint32_t i)
  {
    if (i <= 1)
    {
      return 1;
    }

    return Factorial(i - 1) * i;
  }

  // Compute K amongst N with as an iterator adaptor.
  CombinationHelper_Iter::CombinationHelper_Iter(uint32_t iSize, uint32_t iNum, bool iIsEnd)
    : m_Size(iSize)
    , m_Num(iNum)
  {
    ComputeMaxStep(iSize, iNum);
    if (iIsEnd)
    {
      m_Step = m_MaxStep;
    }
    else
    {
      m_Step = 0;
      for (uint32_t i = 0; i < m_Num; ++i)
      {
        m_Stack.push_back(i);
      }
    }
  }

  void CombinationHelper_Iter::ComputeMaxStep(uint32_t iSize, uint32_t iNum)
  {
    if (iNum > iSize)
    {
      m_MaxStep = 0;
    }
    else
    {
      m_MaxStep = Factorial(iSize) / (Factorial(iNum) * Factorial(iSize - iNum));
    }
  }

  void CombinationHelper_Iter::Advance()
  {
    if (m_Step < m_MaxStep)
    {
      uint32_t missing = 1;
      while (!m_Stack.empty() && ((m_Size - 1) - m_Stack.back()) < missing)
      {
        m_Stack.pop_back();
        ++missing;
      }
      if (!m_Stack.empty())
      {
        m_Stack.back()++;
      }
      if (m_Stack.size() < m_Num)
      {
        while (m_Stack.size() < m_Num)
        {
          if (m_Stack.empty())
          {
            m_Stack.push_back(0);
          }
          else
          {
            m_Stack.push_back(m_Stack.back() + 1);
          }
        }
      }
      
      m_Step++;
    }
  }

}