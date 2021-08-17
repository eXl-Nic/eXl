#include "obb.hpp"
#include "principalaxis.hpp"
#include "mathtools.hpp"

namespace eXl
{
  OBB::OBB(Polygoni const& iPoly)
  {
    m_Dims.m_Data[0] = m_Dims.m_Data[1] = m_Axis[0] = m_Axis[1] = Vector2d::ZERO;
    if (!iPoly.Empty())
    {
      ComputePrincipalAxis computer;
      computer(iPoly);

      //Vector2d center = computer.GetCenter();
      m_Axis[0] = computer.GetPrimaryAxis();
      m_Axis[0].Normalize();
      m_Axis[1] = Vector2d(-m_Axis[0].Y(), m_Axis[0].X());
      Vector<Vector2i> const& border = iPoly.Border();

      Vector2d initVal(border[0].X(), border[0].Y());
      m_Dims.m_Data[0].X() = m_Dims.m_Data[1].X() = (initVal /*- center*/).Dot(m_Axis[0]);
      m_Dims.m_Data[0].Y() = m_Dims.m_Data[1].Y() = (initVal /*- center*/).Dot(m_Axis[1]);

      for (unsigned int i = 1; i < border.size(); ++i)
      {
        Vector2d point(border[i].X(), border[i].Y());
        if (initVal == point)
          break;

        //point = point - center;
        point = Vector2d(point.Dot(m_Axis[0]), point.Dot(m_Axis[1]));
        m_Dims.Absorb(point);
      }
    }
  }

  Polygoni OBB::MakePoly()
  {
    Vector<Vector2i> points;

    points.push_back(MathTools::ToIVec(m_Dims.m_Data[0].X() * m_Axis[0] + m_Dims.m_Data[0].Y() * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[1].X() * m_Axis[0] + m_Dims.m_Data[0].Y() * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[1].X() * m_Axis[0] + m_Dims.m_Data[1].Y() * m_Axis[1]));
    points.push_back(MathTools::ToIVec(m_Dims.m_Data[0].X() * m_Axis[0] + m_Dims.m_Data[1].Y() * m_Axis[1]));

    return Polygoni(points);
  }

}