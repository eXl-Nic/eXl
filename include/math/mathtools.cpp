
#include <math/polygon.hpp>
#include "mathtools.hpp"
#include "geometrytraits.hpp"

namespace eXl
{
  template <>
  void MathTools::SimplifyPolygon<int>(Polygoni const& iPoly, double iEpsilon, Polygoni& oPoly)
  {
    boost::geometry::simplify(iPoly, oPoly, iEpsilon);
  }

  template <>
  void MathTools::Stream<float>(Vector2f const& iPoint, Streamer& iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey(EXL_TEXT("X"));
    iStreamer.WriteFloat(iPoint.m_Data + 0);
    iStreamer.PopKey();
    iStreamer.PushKey(EXL_TEXT("Y"));
    iStreamer.WriteFloat(iPoint.m_Data + 1);
    iStreamer.PopKey();
    iStreamer.EndStruct();
  }

  template <>
  void MathTools::Unstream<float>(Vector2f& oPoint, Unstreamer& iUnstreamer)
  {
    iUnstreamer.BeginStruct();
    iUnstreamer.FindKey(EXL_TEXT("X"));
    iUnstreamer.ReadFloat(&oPoint.X());
    iUnstreamer.EndKey();
    iUnstreamer.FindKey(EXL_TEXT("Y"));
    iUnstreamer.ReadFloat(&oPoint.Y());
    iUnstreamer.EndKey();
    iUnstreamer.EndStruct();
  }

  template <>
  void MathTools::Stream<int>(Vector2i const& iPoint, Streamer& iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey(EXL_TEXT("X"));
    iStreamer.WriteInt(iPoint.m_Data + 0);
    iStreamer.PopKey();
    iStreamer.PushKey(EXL_TEXT("Y"));
    iStreamer.WriteInt(iPoint.m_Data + 1);
    iStreamer.PopKey();
    iStreamer.EndStruct();
  }

  template <>
  void MathTools::Unstream<int>(Vector2i& oPoint, Unstreamer& iUnstreamer)
  {
    iUnstreamer.BeginStruct();
    iUnstreamer.FindKey(EXL_TEXT("X"));
    iUnstreamer.ReadInt(&oPoint.X());
    iUnstreamer.EndKey();
    iUnstreamer.FindKey(EXL_TEXT("Y"));
    iUnstreamer.ReadInt(&oPoint.Y());
    iUnstreamer.EndKey();
    iUnstreamer.EndStruct();
  }
}