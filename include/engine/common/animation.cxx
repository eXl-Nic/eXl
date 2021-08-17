#include <engine/common/animation.hpp>
#include <core/clock.hpp>

namespace eXl
{
  template <typename TimelineBehaviour, typename Impl>
  typename TimelineManager<TimelineBehaviour, Impl>::TimelineHandle TimelineManager<TimelineBehaviour, Impl>::Impl_Start(TimelineEntry*& oNewEntry)
  {
    TimelineHandle newTimeline = m_Timelines.Alloc();
    TimelineEntry& entry = m_Timelines.Get(newTimeline);

    entry.curTime = 0.0;
    entry.startTimestamp = Clock::GetTimestamp();

    oNewEntry = &entry;

    return newTimeline;
  }

  template <typename TimelineBehaviour, typename Impl>
  typename TimelineManager<TimelineBehaviour, Impl>::TimelineHandle TimelineManager<TimelineBehaviour, Impl>::Impl_StartLooping(float iLoopTime, TimelineEntry*& oNewEntry)
  {
    TimelineHandle newTimeline = m_Timelines.Alloc();
    TimelineEntry& entry = m_Timelines.Get(newTimeline);

    entry.curTime = 0.0;
    entry.startTimestamp = Clock::GetTimestamp();
    entry.loopTime = iLoopTime * Clock::GetTicksPerSecond();

    oNewEntry = &entry;

    return newTimeline;
  }

  template <typename TimelineBehaviour, typename Impl>
  void TimelineManager<TimelineBehaviour, Impl>::Tick()
  {
    m_CurTimestamp = Clock::GetTimestamp();
    m_Timelines.Iterate([&](TimelineEntry& entry, TimelineHandle handle)
    {
      uint64_t elapsed = m_CurTimestamp - entry.startTimestamp;

      if (entry.loopTime)
      {
        while (elapsed > *entry.loopTime)
        {
          elapsed -= *entry.loopTime;
          entry.startTimestamp += *entry.loopTime;
        }
      }
      
      entry.curTime = float(elapsed) / float(Clock::GetTicksPerSecond());
      if (!entry.Update(entry.curTime, *static_cast<Impl*>(this)))
      {
        m_ToDelete.insert(handle);
      }
    });

    for (auto handle : m_ToDelete)
    {
      Stop(handle);
    }
    m_ToDelete.clear();
  }

  template <typename TimelineBehaviour, typename Impl>
  void TimelineManager<TimelineBehaviour, Impl>::Stop(TimelineHandle iHandle)
  {
    m_Timelines.Release(iHandle);
  }

  template <typename TimelineBehaviour, typename Impl>
  float TimelineManager<TimelineBehaviour, Impl>::GetTime(TimelineHandle iHandle)
  {
    if (m_Timelines.IsValid(iHandle))
    {
      TimelineEntry& entry = m_Timelines.Get(iHandle);
      return entry.curTime;
    }
    return 0.0;
  }
}