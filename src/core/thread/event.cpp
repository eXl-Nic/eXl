/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/thread/event.hpp>

#include <thread>
#include <condition_variable>

namespace eXl
{
  class Event_Impl
  {
  public:

    inline Event_Impl()
      :m_Counter(0)
    {

    }

    std::condition_variable m_Cond;
    std::mutex m_Mutex;
    volatile unsigned int m_Counter;
  };

  Event::Event()
  {
    m_Impl = new Event_Impl;
  }

  Event::~Event()
  {
    delete m_Impl;
  }

  void Event::Reset(unsigned int iNum)
  {
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    /*
    m_Impl->m_Counter = 0;
    m_Impl->m_Cond.notify_all();
    */
    m_Impl->m_Counter = iNum;
  }

  void Event::Signal()
  {
    unsigned int val = 0;
    {
      std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
      if(m_Impl->m_Counter > 0)
        val = --m_Impl->m_Counter;
    }
    if(val == 0)
      m_Impl->m_Cond.notify_all();
  }

  bool Event::Wait(unsigned int iTimeOut)
  {
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    while(m_Impl->m_Counter != 0)
    {
      if(iTimeOut != 0xFFFFFFFF)
      {
        std::cv_status status = m_Impl->m_Cond.wait_for(lock, std::chrono::duration<long long, std::micro>(iTimeOut));
        if(status == std::cv_status::timeout)
        {
          return false;
        }
        else 
        {
          return true;
        }
      }
      else
      {
        m_Impl->m_Cond.wait(lock);
        return true;
      }
    }
    return true;
  }
}