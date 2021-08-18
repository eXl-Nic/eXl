/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/pooledlist.hpp>
#include <math/segment.hpp>
#include <math/mathexp.hpp>

#include <boost/rational.hpp>
#include <boost/pool/object_pool.hpp>

namespace eXl
{
  template <typename IntReal = int32_t>
  struct FastRational
    :boost::less_than_comparable < FastRational<IntReal>,
    boost::equality_comparable < FastRational<IntReal>,
    boost::less_than_comparable2 < FastRational<IntReal>, IntReal,
    boost::equality_comparable2 < FastRational<IntReal>, IntReal,
    boost::addable < FastRational<IntReal>,
    boost::subtractable < FastRational<IntReal>,
    boost::multipliable < FastRational<IntReal>,
    boost::dividable < FastRational<IntReal>,
    boost::addable2 < FastRational<IntReal>, IntReal,
    boost::subtractable2 < FastRational<IntReal>, IntReal,
    boost::subtractable2_left < FastRational<IntReal>, IntReal,
    boost::multipliable2 < FastRational<IntReal>, IntReal,
    boost::dividable2 < FastRational<IntReal>, IntReal,
    boost::dividable2_left < FastRational<IntReal>, IntReal,
    boost::incrementable < FastRational<IntReal>,
    boost::decrementable < FastRational<IntReal>
    > > > > > > > > > > > > > > > >
  {
  public:

    FastRational(IntReal iNum)
      : m_N(iNum){}

    FastRational(IntReal iNum, IntReal iDen)
      : m_N(iNum)
      , m_D(iDen)
    {}

    FastRational(FastRational const& iR)
      : m_N(iR.m_N)
      , m_D(iR.m_D)
    {}

    FastRational& operator=(IntReal i) { m_N = i; m_D = 1; return *this; }
    FastRational& assign(IntReal n, IntReal d)
    {
      m_N = n;
      m_D = d;
      return *this;
    }

    IntReal numerator() const { return m_N; }
    IntReal denominator() const { return m_D; }


    FastRational& operator+= (const FastRational& r)
    {
      if(m_D == r.m_D)
      {
        m_N += r.m_N;
      }
      else
      {
        assign(m_N * r.m_D + r.m_N * m_D, m_D * r.m_D);
        sanitize();
      }
      return *this;
    }
    FastRational& operator-= (const FastRational& r)
    {
      if(m_D == r.m_D)
      {
        m_N -= r.m_N;
      }
      else
      {
        assign(m_N * r.m_D - r.m_N * m_D, m_D * r.m_D);
        sanitize();
      }
      return *this;
    }
    FastRational& operator*= (const FastRational& r)
    {
      assign(m_N * r.m_N, m_D * r.m_D);
      sanitize();
      return *this;
    }
    FastRational& operator/= (const FastRational& r)
    {
      assign(m_N * r.m_D, m_D * r.m_N);
      sanitize();
      return *this;
    }

    FastRational& operator+= (IntReal i) { m_N += i * m_D; return *this; }
    FastRational& operator-= (IntReal i) { m_N -= i * m_D; return *this; }
    FastRational& operator*= (IntReal i)
    {
      m_N *= i;
      sanitize();
    }
    FastRational& operator/= (IntReal i)
    {
      m_D *= i;
      sanitize();
    }

    const FastRational& operator++() { m_N += m_D; return *this; }
    const FastRational& operator--() { m_N -= m_D; return *this; }
    bool operator!() const { return !m_N; }

    explicit operator bool() const { return operator !() ? false : true; }

    explicit operator int() const
    {
      return m_N / m_D;
    }

    explicit operator float() const
    {
      return static_cast<float>(m_N) / m_D;
    }

    bool operator< (const FastRational& r) const
    {
      return (*this - r).m_N < 0;
    }

    bool operator== (const FastRational& r) const
    {
      return (*this - r).m_N == 0;
    }

    bool operator< (IntReal i) const
    {
      return m_N < i * m_D;
    }

    bool operator> (IntReal i) const
    {
      return m_N > i * m_D;
    }

    bool operator== (IntReal i) const
    {
      FastRational temp = *this;
      temp.reduce();
      return m_D == 1 && m_N == i;
    }

    void sanitize()
    {
      if (m_D < 0)
      {
        m_D *= -1;
        m_N *= -1;
      }
    }

    void reduce()
    {
      sanitize();

      if (m_D == 1)
      {
        return;
      }

      IntReal pgcd = Math<IntReal>::PGCD(m_N, m_D);
      m_N /= pgcd;
      m_D /= pgcd;
    }

  protected:
    IntReal m_N = 0;
    IntReal m_D = 1;
  };
  
  template <typename IntReal>
  inline FastRational<IntReal> operator+ (const FastRational<IntReal>& r)
  {
    return r;
  }

  template <typename IntReal>
  inline FastRational<IntReal> operator- (const FastRational<IntReal>& r)
  {
    return FastRational<IntReal>(-r.numerator(), r.denominator());
  }

  typedef /*boost::rational<int64_t>*/ /*float*/ FastRational<int64_t> QType;
  typedef Vector2<QType> Vector2Q;

  template <typename Real>
  Vector2Q ToVec2Q(Vector2<Real> const& iVec)
  {
    return Vector2Q(iVec.X(), iVec.Y());
  }

  template <typename Real>
  Vector2<Real> FromVec2Q(Vector2Q const& iVec)
  {
    return Vector2<Real>(static_cast<Real>(iVec.X()), static_cast<Real>(iVec.Y()));
    //return Vector2<Real>(iVec.X().numerator() / iVec.X().denominator(), iVec.Y().numerator() / iVec.Y().denominator());
  }

  class EXL_MATH_API Intersector
  {
  public:

    struct ActiveSegment
    {
      Vector2Q m_Point;
      uint32_t m_Idx;
    };

    struct UserEvent
    {
      Vector2i m_Position;
      uint32_t m_Idx;
    };

    typedef std::function<bool(Vector<ActiveSegment> const& iSweepLine, uint32_t iPosInSweepLine, Segment<QType> const& iCandidate)> Filter;
    typedef std::function<void(Vector<ActiveSegment> const& iSweepLine, int32_t iLowSeg, int32_t iHighSeg, UserEvent const& iEvent)> EvtCallback;

    struct Parameters
    {
      Filter m_Filter;
      EvtCallback m_UsrEvtCb;
      std::vector<UserEvent> m_UsrEvts;
    };

    Intersector();

    Err IntersectSegments(Vector<Segmenti> const& iSegments, Vector<std::pair<uint32_t, Segmenti>>& oSegs, Parameters const& iParams = Parameters());

  private:

    struct Event
    {
      enum Type
      {
        Intersection,
        End,
        Start,
        User,
      };

      Event(PooledList<uint32_t>::Pool& iPool, Vector2Q const& iPoint, uint32_t iSeg, Type iType);
      Event(PooledList<uint32_t>::Pool& iPool, Vector2Q const& iPoint, uint32_t iSeg1, uint32_t iSeg2);
      Event(Event const& iEvt);
      Event& operator=(Event const& iEvt);
      Event(Event&& iEvt);
      Event& operator=(Event&& iEvt);

      bool IsEmpty() const;

      bool operator<(Event const& iOther) const;

      Vector2Q m_Point;
      mutable PooledList<uint32_t> m_SegmentsStart;
      mutable PooledList<uint32_t> m_SegmentsInter;
      mutable PooledList<uint32_t> m_SegmentsEnd;
      //Type m_Type;
    };

    struct OrderedSeg
    {
      OrderedSeg(Segmenti const& iSeg, uint32_t iOrigSeg);

      QType GetYAt(QType iX, Event const& iEvt) const;

      Vector2Q m_Start;
      Vector2Q m_End;
      boost::optional<QType> m_Slope;
      uint32_t m_OrigSeg;
    };

    void CheckIntersection(Event const& iEvt, OrderedSeg const& iSeg1, OrderedSeg const& iSeg2);
    bool CheckIntersection(Event const& iEvt, OrderedSeg const& iSeg1, OrderedSeg const& iSeg2, boost::optional<Event>& outSeg);

    void InsertEvent(Event&& iEvt);

    void RemoveEvt(uint32_t iSeg1, uint32_t iSeg2);

    void SortSegmentExtremities();

    Vector<ActiveSegment> m_ActiveSegments;
    Vector<uint32_t> m_SegInsertCount;
    PooledList<uint32_t>::Pool m_SegListPool;
    Vector<uint32_t> m_SortArray;

    Vector<Event> m_EventQueue;

    //MemoryPool m_EvtListAlloc;
    //typedef std::multiset<Event, std::less<Event>, PooledAllocator<Event>> EventQueue;
    //EventQueue m_EventQueue;
    Vector<OrderedSeg> m_Segments;
  };
}