/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <math/seginter.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  Intersector::Event::Event(PooledList<uint32_t>::Pool& iPool, Vector2Q const& iPoint, uint32_t iSeg, Type iType)
    : m_SegmentsStart(iPool)
    , m_SegmentsInter(iPool)
    , m_SegmentsEnd(iPool)
    , m_Point(iPoint)
  {
    if(iType == Start)
    {
      m_SegmentsStart.PushBack(iSeg);
    }
    else if (iType == End)
    {
      m_SegmentsEnd.PushBack(iSeg);
    }
  }

  Intersector::Event::Event(PooledList<uint32_t>::Pool& iPool, Vector2Q const& iPoint, uint32_t iSeg1, uint32_t iSeg2)
    : m_SegmentsStart(iPool)
    , m_SegmentsInter(iPool)
    , m_SegmentsEnd(iPool)
    , m_Point(iPoint)
  {
    m_SegmentsInter.PushBack(iSeg1);
    m_SegmentsInter.PushBack(iSeg2);
  }

  Intersector::Event::Event(Event const& iEvt)
    : m_SegmentsStart(iEvt.m_SegmentsStart)
    , m_SegmentsInter(iEvt.m_SegmentsInter)
    , m_SegmentsEnd(iEvt.m_SegmentsEnd)
    , m_Point(iEvt.m_Point)
  {

  }

  Intersector::Event& Intersector::Event::operator=(Event const& iEvt)
  {
    m_SegmentsStart = iEvt.m_SegmentsStart;
    m_SegmentsInter = iEvt.m_SegmentsInter;
    m_SegmentsEnd = iEvt.m_SegmentsEnd;
    m_Point = iEvt.m_Point;

    return *this;
  }

  Intersector::Event::Event(Event&& iEvt)
    : m_SegmentsStart(std::move(iEvt.m_SegmentsStart))
    , m_SegmentsInter(std::move(iEvt.m_SegmentsInter))
    , m_SegmentsEnd(std::move(iEvt.m_SegmentsEnd))
    , m_Point(iEvt.m_Point)
  {

  }

  Intersector::Event& Intersector::Event::operator=(Event&& iEvt)
  {
    m_SegmentsStart = std::move(iEvt.m_SegmentsStart);
    m_SegmentsInter = std::move(iEvt.m_SegmentsInter);
    m_SegmentsEnd = std::move(iEvt.m_SegmentsEnd);
    m_Point = iEvt.m_Point;

    return *this;
  }

  bool Intersector::Event::IsEmpty() const
  {
    return m_SegmentsStart.Begin() == m_SegmentsStart.End() 
      && m_SegmentsEnd.Begin() == m_SegmentsEnd.End() 
      && m_SegmentsInter.Begin() == m_SegmentsInter.End();
  }

  bool operator < (Vector2Q const& iPt1, Vector2Q const& iPt2)
  {
    if (iPt1.x == iPt2.x)
    {
      return iPt1.y < iPt2.y;
    }
    return iPt1.x < iPt2.x;
  }

  bool Intersector::Event::operator<(Event const& iOther) const
  {
    return m_Point < iOther.m_Point;
  }

  Intersector::OrderedSeg::OrderedSeg(Segmenti const& iSeg, uint32_t iOrigSeg)
    : m_Start(ToVec2Q(iSeg.m_Ext1))
    , m_End(ToVec2Q(iSeg.m_Ext2))
    , m_OrigSeg(iOrigSeg)
  {
    if(iSeg.m_Ext2 < iSeg.m_Ext1)
    {
      std::swap(m_Start, m_End);
    }
    if(m_End.x != m_Start.x)
    {
      m_Slope = QType(m_End.y - m_Start.y) / QType(m_End.x - m_Start.x);
    }
  }

  QType Intersector::OrderedSeg::GetYAt(QType iX, Event const& iEvt) const
  {
    if(m_Slope)
    {
      return m_Start.y + (*m_Slope) * (iX - m_Start.x);
    }
    else
    {
      return iEvt.m_Point.y;
    }
  }

  bool Intersector::CheckIntersection(Event const& iEvt, OrderedSeg const& iSeg1, OrderedSeg const& iSeg2, Optional<Event>& outSeg)
  {
    uint32_t seg1Idx = &iSeg1 - m_Segments.data();
    uint32_t seg2Idx = &iSeg2 - m_Segments.data();

    Vector2Q interPt(0, 0);
    uint32_t res = Segment<QType>::Intersect(iSeg1.m_Start, iSeg1.m_End, iSeg2.m_Start, iSeg2.m_End, interPt, 0);
    if(res == Segment<QType>::PointOnSegments)
    {
      if(iEvt.m_Point < interPt || iEvt.m_Point == interPt)
      {
        outSeg = Event(m_SegListPool, interPt, seg1Idx, seg2Idx);
      }
    }
    else if(res & Segment<QType>::ConfoundSegments)
    {
      //eXl_ASSERT(iEvt.m_Type == Event::Start);

      if(iSeg1.m_End.x > iSeg2.m_End.x)
      {
        //Reinsert seg1's start at seg2's end.
        Event newStart(m_SegListPool, iSeg2.m_End, seg1Idx, Event::Start);
        auto lowerBound = std::lower_bound(m_EventQueue.rbegin(), m_EventQueue.rend(), newStart);
        if(lowerBound->m_Point == iSeg2.m_End)
        {
          lowerBound->m_SegmentsStart.PushBack(seg1Idx);
        }
        else
        {
          m_EventQueue.insert(lowerBound.base(), std::move(newStart));
        }
        
        //m_EventQueue.emplace(std::move(newStart));
      }
      else if(iSeg1.m_End != iEvt.m_Point)
      {
        //Seg1 is useless, remove it.
        Event evt(m_SegListPool, iSeg1.m_End, seg1Idx, Event::End);
        auto toFixup = std::lower_bound(m_EventQueue.rbegin(), m_EventQueue.rend(), evt);

        // remove iSeg1's endpoint from the list.
        auto iterToRemove = std::prev(toFixup.base());
        auto endingSegs = iterToRemove->m_SegmentsEnd.Begin();
        while(*endingSegs != seg1Idx)
        {
          ++endingSegs;
        }

        //eXl_ASSERT(endingSegs != iterToRemove->m_SegmentsEnd.End());
        iterToRemove->m_SegmentsEnd.Erase(endingSegs);

        if(iterToRemove->IsEmpty())
        {
          m_EventQueue.erase(iterToRemove);
        }
      }

      for (auto iter = m_ActiveSegments.begin(); iter != m_ActiveSegments.end(); ++iter)
      {
        if (iter->m_Idx == seg1Idx)
        {
          m_ActiveSegments.erase(iter);
          break;
        }
      }

      return false;
    }

    return true;
  }

  void Intersector::InsertEvent(Event&& iEvt)
  {
    //for(auto iter = iEvt.m_Segments.Begin(); iter != iEvt.m_Segments.End(); ++iter)
    //{
    //  ++m_SegInsertCount[*iter];
    //}

    auto lowerBound = std::lower_bound(m_EventQueue.rbegin(), m_EventQueue.rend(), iEvt);
    //auto lowerBound = m_EventQueue.find(iEvt);
    if(lowerBound != m_EventQueue.rend() 
      //lowerBound != m_EventQueue.end()
      //&& lowerBound->m_Type == Event::Intersection 
      && lowerBound->m_Point == iEvt.m_Point
      )
    {
      for(auto startSegIter = iEvt.m_SegmentsStart.Begin(); startSegIter != iEvt.m_SegmentsStart.End(); ++startSegIter )
      {
        lowerBound->m_SegmentsStart.PushBack(*startSegIter);
      }
      for(auto endSegIter = iEvt.m_SegmentsEnd.Begin(); endSegIter != iEvt.m_SegmentsEnd.End(); ++endSegIter )
      {
        lowerBound->m_SegmentsEnd.PushBack(*endSegIter);
      }
      for(auto interSegIter = iEvt.m_SegmentsInter.Begin(); interSegIter != iEvt.m_SegmentsInter.End(); ++interSegIter )
      {
        lowerBound->m_SegmentsInter.PushBack(*interSegIter);
      }
    }
    else
    {
      m_EventQueue.insert(lowerBound.base(), std::move(iEvt));
      //m_EventQueue.emplace(std::move(iEvt));
    }
    
  }

  void Intersector::CheckIntersection(Event const& iEvt, OrderedSeg const& iSeg1, OrderedSeg const& iSeg2)
  {
    Optional<Event> evt;
    CheckIntersection(iEvt, iSeg1, iSeg2, evt);
    if(evt && evt->m_Point != iEvt.m_Point)
    {
      InsertEvent(std::move(*evt));
    }
  }

  void Intersector::RemoveEvt(uint32_t iSeg1, uint32_t iSeg2)
  {
    if(iSeg1 != iSeg2)
    {
      //if(m_SegInsertCount[iSeg1] == 0 || m_SegInsertCount[iSeg2] == 0)
      //{
      //  return;
      //}

      for(auto iter = m_EventQueue.begin(); iter!= m_EventQueue.end(); ++iter)
      {
        uint32_t countSeg = 0;
        PooledList<uint32_t>::Iterator segs[] = {iter->m_SegmentsInter.End(), iter->m_SegmentsInter.End()};
        for(auto iterSeg = iter->m_SegmentsInter.Begin(); iterSeg != iter->m_SegmentsInter.End(); ++iterSeg)
        {
          ++countSeg;
          if(*iterSeg == iSeg1 && segs[0] == iter->m_SegmentsInter.End())
          {
            segs[0] = iterSeg;
          } 
          if(*iterSeg == iSeg2 && segs[1] == iter->m_SegmentsInter.End())
          {
            segs[1] = iterSeg;
          }
        }

        if(segs[0] != iter->m_SegmentsInter.End()
          && segs[1] != iter->m_SegmentsInter.End())
        {
          if(iter->IsEmpty())
          {
            m_EventQueue.erase(iter);
          }
          else
          {
            iter->m_SegmentsInter.Erase(segs[0]);
            iter->m_SegmentsInter.Erase(segs[1]);
          }
          //m_SegInsertCount[iSeg1]--;
          //m_SegInsertCount[iSeg2]--;

          break;
        }
      }
    }
  };

  Intersector::Intersector()
    //: m_EvtListAlloc(4096)
    //, m_EventQueue(std::less<Event>(), PooledAllocator<Event>(m_EvtListAlloc))
  {

  }

  void Intersector::SortSegmentExtremities()
  {
    for(auto& evt : m_EventQueue)
    {
      auto sortPredicate = [&] (uint32_t const& iSeg1, uint32_t const& iSeg2)
      {
        auto const& seg1 = m_Segments[iSeg1];
        auto const& seg2 = m_Segments[iSeg2];

        if(seg1.m_Slope)
        {
          if(seg2.m_Slope)
          {
            return *seg1.m_Slope < *seg2.m_Slope;
          }
        }

        return seg2.m_Slope && !seg1.m_Slope;

      };

      m_SortArray.clear();
      for(auto startSegIter = evt.m_SegmentsStart.Begin(); startSegIter != evt.m_SegmentsStart.End(); ++startSegIter )
      {
        m_SortArray.push_back(*startSegIter);
      }
      evt.m_SegmentsStart.Clear();

      std::sort(m_SortArray.begin(), m_SortArray.end(), sortPredicate);

      for(uint32_t segIdx : m_SortArray)
      {
        evt.m_SegmentsStart.PushBack(segIdx);
      }

      m_SortArray.clear();
      for(auto endSegIter = evt.m_SegmentsEnd.Begin(); endSegIter != evt.m_SegmentsEnd.End(); ++endSegIter )
      {
        m_SortArray.push_back(*endSegIter);
      }
      evt.m_SegmentsEnd.Clear();

      std::sort(m_SortArray.begin(), m_SortArray.end(), sortPredicate);

      for(uint32_t segIdx : m_SortArray)
      {
        evt.m_SegmentsEnd.PushBack(segIdx);
      }

    }
  }

  Err Intersector::IntersectSegments(Vector<Segmenti> const& iSegments, Vector<std::pair<uint32_t, Segmenti>>& oSegs, Parameters const& iParams)
  {
    //m_EventQueue.~EventQueue();
    //m_EvtListAlloc.Reset();
    //new(&m_EventQueue) EventQueue(std::less<Event>(), PooledAllocator<Event>(m_EvtListAlloc));
    //m_SegListPool.Reserve(1024);

    m_EventQueue.clear();
    m_Segments.clear();
    m_ActiveSegments.clear();
    m_SegInsertCount.clear();

    {
      uint32_t segCounter = 0;
      for(auto const& seg : iSegments)
      {
        if(seg.m_Ext1 != seg.m_Ext2)
        {
          m_Segments.push_back(OrderedSeg(seg, segCounter++));

          //m_EventQueue.emplace_back(Event(m_SegListPool, m_Segments.back().m_Start, m_Segments.size() - 1, Event::Start));
          //m_EventQueue.emplace_back(Event(m_SegListPool, m_Segments.back().m_End, m_Segments.size() - 1, Event::End));

          InsertEvent(Event(m_SegListPool, m_Segments.back().m_Start, m_Segments.size() - 1, Event::Start));
          InsertEvent(Event(m_SegListPool, m_Segments.back().m_End, m_Segments.size() - 1, Event::End));

          //m_EventQueue.emplace(Event(m_SegListPool, m_Segments.back().m_Start, m_Segments.size() - 1, Event::Start));
          //m_EventQueue.emplace(Event(m_SegListPool, m_Segments.back().m_End, m_Segments.size() - 1, Event::End));

          //m_SegInsertCount.push_back(0);
        }
      }
      //if(iParams.m_UsrEvtCb)
      //{
      //  for(uint32_t pos = 0; pos < iParams.m_UsrEvts.size(); ++pos)
      //  {
      //    m_EventQueue.emplace_back(Event(m_SegListPool, ToVec2Q(iParams.m_UsrEvts[pos].m_Position), pos, Event::User));
      //  }
      //}

      //std::sort(m_EventQueue.begin(), m_EventQueue.end());
    }

    //std::reverse(m_EventQueue.begin(), m_EventQueue.end());

    SortSegmentExtremities();

    while(!m_EventQueue.empty())
    {
      Event curEvt = std::move(m_EventQueue.back());
      m_EventQueue.pop_back();

      //Event curEvt = *m_EventQueue.begin();
      //m_EventQueue.erase(m_EventQueue.begin());

      auto curX = curEvt.m_Point.x;

      auto activeSegComp = [this, curX, &curEvt](ActiveSegment const& iSeg1, ActiveSegment const& iSeg2)
      {
        auto const& seg1 = m_Segments[iSeg1.m_Idx];
        auto const& seg2 = m_Segments[iSeg2.m_Idx];

        auto y1 = seg1.GetYAt(curX, curEvt);
        auto y2 = seg2.GetYAt(curX, curEvt);

        if(y1 != y2)
        {
          return y1 < y2;
        }
        else
        {
          if(seg1.m_Slope)
          {
            if(seg2.m_Slope)
            {
              return *seg1.m_Slope < *seg2.m_Slope;
            }
            return true;
          }
          return false;
          
        }
      };

      //if(curEvt.m_Type == Event::Intersection)
      //{
      //  for(auto iter = curEvt.m_Segments.Begin(); iter != curEvt.m_Segments.End(); ++iter)
      //  {
      //    --m_SegInsertCount[*iter];
      //  }
      //}

      //switch(curEvt.m_Type)
      {

      //case Event::Intersection:
      if(curEvt.m_SegmentsInter.Begin() != curEvt.m_SegmentsInter.End())
      {
        //Set<uint32_t> checkSet;
        int32_t posLow = INT_MAX;
        int32_t posHigh = -INT_MAX;

        for(auto iter = curEvt.m_SegmentsInter.Begin(); iter != curEvt.m_SegmentsInter.End(); ++iter)
        {
          for(int32_t i = 0; i<m_ActiveSegments.size(); ++i)
          {
            if(m_ActiveSegments[i].m_Idx == *iter)
            {
              if(i < posLow)
              {
                posLow = i;
              }
              if(i > posHigh)
              {
                posHigh = i;
              }
              //checkSet.insert(i);
              break;
            }
          }
        }
        //if(checkSet.size() != (posHigh - posLow) + 1)
        //{
        //  //eXl_ASSERT(false);
        //  return Err::Failure;
        //}
        if(!(posLow < m_ActiveSegments.size() && posHigh > posLow))
        {
          //eXl_ASSERT(false);
          return Err::Failure;
        }

        for(int32_t i = posLow; i<=posHigh; ++i)
        {
          ActiveSegment& activeSeg = m_ActiveSegments[i];

          if(activeSeg.m_Point != curEvt.m_Point)
          {
            if(!iParams.m_Filter || iParams.m_Filter(m_ActiveSegments, i, Segment<QType>{ activeSeg.m_Point, curEvt.m_Point} ))
            {
              Segmenti interSeg = {FromVec2Q<int>(activeSeg.m_Point), FromVec2Q<int>(curEvt.m_Point)};
              oSegs.push_back(std::make_pair(activeSeg.m_Idx, interSeg));
            }
            activeSeg.m_Point = curEvt.m_Point;
          }
        }

        std::reverse(m_ActiveSegments.begin() + posLow, m_ActiveSegments.begin() + posHigh + 1);

        //Order is reversed now.
        uint32_t lowSegIdx = m_ActiveSegments[posLow].m_Idx;
        uint32_t highSegIdx = m_ActiveSegments[posHigh].m_Idx;
        auto const& lowSeg = m_Segments[lowSegIdx];
        auto const& highSeg = m_Segments[highSegIdx];

        if(posHigh + 1 < m_ActiveSegments.size()) 
        {
          uint32_t higherSegIdx = m_ActiveSegments[posHigh + 1].m_Idx;

          auto const& higherSeg = m_Segments[higherSegIdx];
          CheckIntersection(curEvt, highSeg, higherSeg);
          RemoveEvt(higherSegIdx, lowSegIdx);
        }
        if(posLow > 0) 
        {
          uint32_t lowerSegIdx = m_ActiveSegments[posLow - 1].m_Idx;

          auto const& lowerSeg = m_Segments[lowerSegIdx];
          CheckIntersection(curEvt, lowSeg, lowerSeg);
          RemoveEvt(highSegIdx, lowerSegIdx);
        }
      }

      //case Event::End:
      for(auto iterEnd = curEvt.m_SegmentsEnd.Begin(); iterEnd != curEvt.m_SegmentsEnd.End(); ++iterEnd)
      {
        uint32_t curSegIdx = *iterEnd;
        auto const& curSeg = m_Segments[curSegIdx];

        //ActiveSegment activeSeg = {curEvt.m_Point, curSegIdx};

        //auto lowerBound = std::lower_bound(m_ActiveSegments.begin(), m_ActiveSegments.end(), activeSeg, activeSegComp);
        int32_t curSegPos = -1;

        for(auto& seg : m_ActiveSegments)
        {
          if(seg.m_Idx == curSegIdx)
          {
            curSegPos = &seg - m_ActiveSegments.data();
          }
        }

        if(curSegPos == -1)
        {
          //eXl_ASSERT(false);
          return Err::Failure;
        }

        //Segmenti finalSeg = {FromVec2Q<int>(m_ActiveSegments[curSegPos].m_Point), FromVec2Q<int>(curEvt.m_Point)};
        if(m_ActiveSegments[curSegPos].m_Point != curEvt.m_Point)
        {
          if(!(iParams.m_Filter) || iParams.m_Filter(m_ActiveSegments, curSegPos, Segment<QType>{m_ActiveSegments[curSegPos].m_Point, curEvt.m_Point}))
          {
            Segmenti finalSeg = {FromVec2Q<int>(m_ActiveSegments[curSegPos].m_Point), FromVec2Q<int>(curEvt.m_Point)};
            oSegs.push_back(std::make_pair(curSegIdx, finalSeg));
          }
        }

        m_ActiveSegments.erase(m_ActiveSegments.begin() + curSegPos);

        int32_t lowerSegIdx = -1;
        int32_t higherSegIdx = -1;

        if(curSegPos > 0) 
        {
          lowerSegIdx = m_ActiveSegments[curSegPos - 1].m_Idx;
        }
        if(curSegPos < m_ActiveSegments.size()) 
        {
          higherSegIdx = m_ActiveSegments[curSegPos].m_Idx;
        }

        if(lowerSegIdx != -1 && higherSegIdx != -1) 
        {
          auto const& lowerSeg = m_Segments[lowerSegIdx];
          auto const& higherSeg = m_Segments[higherSegIdx];

          CheckIntersection(curEvt, lowerSeg, higherSeg);
        }
      }

      //case Event::Start:
      for(auto iterStart = curEvt.m_SegmentsStart.Begin(); iterStart != curEvt.m_SegmentsStart.End(); ++iterStart)
      {
        uint32_t curSegIdx = *iterStart;
        auto const& curSeg = m_Segments[curSegIdx];

        ActiveSegment newSeg = {curEvt.m_Point, curSegIdx};

        auto lowerBound = std::lower_bound(m_ActiveSegments.begin(), m_ActiveSegments.end(), newSeg, activeSegComp);

        int32_t lowerSegIdx = -1;
        int32_t higherSegIdx = -1;

        Optional<Event> evts[2];

        bool acceptSegment = true;

        if(lowerBound > m_ActiveSegments.begin())
        {
          lowerSegIdx = (lowerBound - 1)->m_Idx;
          auto const& lowerSeg = m_Segments[lowerSegIdx];

          acceptSegment &= CheckIntersection(curEvt, curSeg, lowerSeg, evts[0]);
        }
        if(lowerBound != m_ActiveSegments.end())
        {
          higherSegIdx = lowerBound->m_Idx;
          auto const& higherSeg = m_Segments[higherSegIdx];

          acceptSegment &= CheckIntersection(curEvt, curSeg, higherSeg, evts[1]);
        }

        if(acceptSegment)
        {
          for(auto& optEvt : evts)
          {
            if(optEvt)
            {
              if(optEvt->m_Point != curEvt.m_Point)
              {
                InsertEvent(std::move(*optEvt));
              }
              else
              {
                ActiveSegment* activeSeg = nullptr;
                // Only report event.
                for(auto iterSeg = optEvt->m_SegmentsInter.Begin(); iterSeg != optEvt->m_SegmentsInter.End(); ++iterSeg)
                {
                  uint32_t segIdx = *iterSeg;
                  if(segIdx == curSegIdx)
                    continue;

                  for(auto& seg : m_ActiveSegments)
                  {
                    if(seg.m_Idx == segIdx)
                    {
                      activeSeg = &seg;
                      break;
                    }
                  }

                  //eXl_ASSERT(activeSeg);

                  if(activeSeg->m_Point != curEvt.m_Point)
                  {
                    if(!iParams.m_Filter || iParams.m_Filter(m_ActiveSegments, activeSeg - m_ActiveSegments.data(), Segment<QType>{activeSeg->m_Point, curEvt.m_Point}))
                    {
                      Segmenti interSeg = {FromVec2Q<int>(activeSeg->m_Point), FromVec2Q<int>(curEvt.m_Point)};
                      oSegs.push_back(std::make_pair(activeSeg->m_Idx, interSeg));
                    }
                    activeSeg->m_Point = curEvt.m_Point;
                  }
                }
              }
            }
          }

          m_ActiveSegments.insert(lowerBound, newSeg);

          if(lowerSegIdx != -1 && higherSegIdx != -1)
          {
            // Only remove the event when the three segments do not intersect at the same point.
            if(!evts[0] 
              || !evts[1] 
              || evts[0]->m_Point != evts[1]->m_Point)
            {
              RemoveEvt(lowerSegIdx, higherSegIdx);
            }
          }
        }
      }
      
      /*
      case Event::User:
      {
        auto userPtComp = [this, curX, &curEvt](ActiveSegment const& iSeg1, ActiveSegment const& iSeg2)
        {
          auto const& seg = m_Segments[iSeg1.m_Idx == -1 ? iSeg2.m_Idx : iSeg1.m_Idx];
          
          auto y1 = seg.GetYAt(curX, curEvt);
          
          return y1 < curEvt.m_Point.y;
        };

        ActiveSegment dummySeg = {curEvt.m_Point, -1};
        auto lowerBound = std::lower_bound(m_ActiveSegments.begin(), m_ActiveSegments.end(), dummySeg, userPtComp);

        int32_t lowSeg = -1;
        int32_t highSeg = -1;

        if(lowerBound != m_ActiveSegments.begin())
        {
          lowSeg = (lowerBound - 1) - m_ActiveSegments.begin();
        }

        if(lowerBound != m_ActiveSegments.end())
        {
          highSeg = lowerBound - m_ActiveSegments.begin();
        }

        iParams.m_UsrEvtCb(m_ActiveSegments, lowSeg, highSeg, iParams.m_UsrEvts[*curEvt.m_Segments.Begin()]);

        break;
      }*/
      
      }
    }

    return Err::Success;
  }
}