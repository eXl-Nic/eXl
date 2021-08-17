/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/clock.hpp>


#ifdef WIN32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include<Mmsystem.h>
#else
#include <time.h>
#endif

Clock::Clock()
{
  Initialize();
}
 
Clock::~Clock()
{
}

#ifdef WIN32 

static_assert(sizeof(LARGE_INTEGER) == sizeof(uint64_t), "");

struct Clock_Impl
{
  Clock_Impl()
  {
    PerfTime = QueryPerformanceFrequency((LARGE_INTEGER*)&Frq) == 0 ? false : true;
    if (PerfTime)
    {
      OneTick = 1.0f / Frq;
    }
    else
    {
      OneTick = 0.001f;
    }
  }

  uint64_t GetTimestamp() const
  {
    uint64_t time;
    if (PerfTime)
    {
      QueryPerformanceCounter((LARGE_INTEGER*)&time);
    }
    else
    {
      time = timeGetTime();
    }
    return time;
  }

  bool PerfTime;
  uint64_t Frq;
  float OneTick;
};
#else

struct Clock_Impl
{
  Clock_Impl()
  {
    timespec ts;

    clock_getres(CLOCK_MONOTONIC, &ts);

    Frq = 10000000;
    OneTick = 1.0 / Frq;
  }

  uint64_t GetTimestamp() const
  {
    timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    // in 100 ns intervals.
    return static_cast<uint64_t>(ts.tv_sec) * 10000000u + static_cast<uint64_t>(ts.tv_nsec) / 100u;
  }

  uint64_t Frq;
  float OneTick;
};

#endif

Clock_Impl& GetImpl()
{
  static Clock_Impl s_Impl;
  return s_Impl;
}

void Clock::Initialize()
{
  Time0 = GetTimestamp();
}

float Clock::GetTime()
{
  Time1 = GetTimestamp();
 
  float dt=(Time1-Time0) * GetImpl().OneTick;
  Time0=Time1;

  return dt;
}

float Clock::PeekTime() const
{
  uint64_t time = GetTimestamp();
 
  float dt=(time-Time0)* GetImpl().OneTick;
  return dt;
}

uint64_t Clock::GetTicksPerSecond()
{
  return GetImpl().Frq;
}

uint64_t Clock::GetTimestamp()
{
  return GetImpl().GetTimestamp();
}