
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