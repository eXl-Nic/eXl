/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/obb.hpp>
#include <math/principalaxis.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  OBB::OBB(Polygoni const& iPoly)
  {
    m_Dims.m_Data[0] = m_Dims.m_Data[1] = m_Axis[0] = m_Axis[1] = glm::zero<Vec2d>();
    if (!iPoly.Empty())
    {
      ComputePrincipalAxis computer;
      computer(iPoly);

      //Vec2d center = computer.GetCenter();
      m_Axis[0] = computer.GetPrimaryAxis();
      m_Axis[0] = normalize(m_Axis[0]);
      m_Axis[1] = Vec2d(-m_Axis[0].y, m_Axis[0].x);
      Vector<Vec2i> const& border = iPoly.Border();

      Vec2d initVal(border[0].x, border[0].y);
      m_Dims.m_Data[0].x = m_Dims.m_Data[1].x = dot((initVal /*- center*/), m_Axis[0]);
      m_Dims.m_Data[0].y = m_Dims.m_Data[1].y = dot((initVal /*- center*/), m_Axis[1]);

      for (unsigned int i = 1; i < border.size(); ++i)
      {
        Vec2d point(border[i].x, border[i].y);
        if (initVal == point)
          break;

        //point = point - center;
        point = Vec2d(dot(point, m_Axis[0]), dot(point, m_Axis[1]));
        m_Dims.Absorb(point);
      }
    }
  }

  Polygoni OBB::MakePoly()
  {
    Vector<Vec2i> points;

    points.push_back(MathTools::ToIVec(m_Dims.m_Data[0].x * m_Axis[0] + m_Dims.m_Data[0].y * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[1].x * m_Axis[0] + m_Dims.m_Data[0].y * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[1].x * m_Axis[0] + m_Dims.m_Data[1].y * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[0].x * m_Axis[0] + m_Dims.m_Data[1].y * m_Axis[1]));

    return Polygoni(points);
  }

}