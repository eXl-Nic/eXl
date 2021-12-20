/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/containers.hpp>
#include <engine/common/object.hpp>
#include <engine/common/world.hpp>

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
  class TimelineManager : public WorldSystem
  {
  public:

    struct TimelineEntry : public TimelineBehaviour
    {
      double startTime;
      boost::optional<float> loopTime;
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
  };
}