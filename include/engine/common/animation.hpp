#pragma once

#include <core/containers.hpp>
#include <engine/common/object.hpp>

#include <math/math.hpp>

namespace eXl
{
  template <typename T>
  struct LinearInterpolator
  {
    LinearInterpolator(T const& iValue1, T const& iValue2)
      : m_Value1(iValue1)
      , m_Value2(iValue2)
    {}

    T operator() (float iDelta) const
    {
      return m_Value2 * iDelta + m_Value1 * (1.0 - iDelta);
    }

    T const& m_Value1;
    T const& m_Value2;
  };

  template <typename T, typename Interpolator = LinearInterpolator<T>>
  class Animation
  {
  public:
    struct KeyFrame
    {
      KeyFrame(T const& iValue, float iTime)
        : value(iValue)
        , time(iTime)
      {}
      bool operator < (KeyFrame const& iOther) const
      {
        return time < iOther.time;
      }
      T value;
      float time;
    };

    void Add(T const& iValue, float iTime)
    {
      if (!m_Frames.empty())
      {
        eXl_ASSERT_REPAIR_BEGIN(iTime > m_Frames.back().time)
        {
          m_Frames.push_back(KeyFrame(iValue, iTime));
          std::sort(m_Frames.begin(), m_Frames.end());
          return;
        }
      }
      m_Frames.push_back(KeyFrame(iValue, iTime));
    }

    T GetValue(float iTime, bool* oAnimEnded = nullptr)
    {
      if (m_Frames.empty())
      {
        return T();
      }

      KeyFrame dummy(T(), iTime);
      // Keep a hint somewhere to speed up the search ?
      auto frameNext = std::lower_bound(m_Frames.begin(), m_Frames.end(), dummy);
      auto framePrev = frameNext;
      float framePrevTime;
      if (frameNext == m_Frames.begin())
      {
        //Assume looping
        framePrev = std::prev(m_Frames.end());
        framePrevTime = 0.0;
      }
      else if (frameNext == m_Frames.end())
      {
        if (oAnimEnded)
        {
          *oAnimEnded = true;
        }
        return m_Frames.back().value;
      }
      else
      {
        framePrev = std::prev(frameNext);
        framePrevTime = framePrev->time;
      }

      float delta = (iTime - framePrevTime) / (frameNext->time - framePrevTime);

      Interpolator interp(framePrev->value, frameNext->value);

      return interp(delta);
    }

    Vector<KeyFrame> m_Frames;
  };

  using LinearFloatAnimation = Animation<float>;
  using LinearPositionAnimation = Animation<Vector3f>;

  template <typename TimelineBehaviour, typename Impl>
  class TimelineManager
  {
  public:

    struct TimelineEntry : public TimelineBehaviour
    {
      uint64_t startTimestamp;
      boost::optional<uint64_t> loopTime;
      float curTime;
    };

    using TimelineTable = ObjectTable<TimelineEntry>;
    using TimelineHandle = typename TimelineTable::Handle;

    void Stop(TimelineHandle);

    void Tick();

    float GetTime(TimelineHandle);

  protected:

    TimelineHandle Impl_Start(TimelineEntry*& oNewEntry);
    TimelineHandle Impl_StartLooping(float iLoopTime, TimelineEntry*& oNewEntry);

    TimelineTable m_Timelines;
    UnorderedSet<TimelineHandle> m_ToDelete;
    uint64_t m_CurTimestamp;
  };
}