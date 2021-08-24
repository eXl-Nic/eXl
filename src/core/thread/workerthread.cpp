/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/thread/workerthread.hpp>
#include <thread>
#include <condition_variable>

namespace eXl
{

  class WorkerThread_Impl : public std::thread
  {
  public:
    WorkerThread_Impl(WorkerThread& iHost) 
      : m_Host(iHost)
      , std::thread(std::ref(*this))
      , m_ThreadState(eWaiting)
      , m_Request(eWaiting)
    {}

    void operator()()
    {
      m_Host.InternalRun();
    }

    std::condition_variable m_CondThread;
    std::condition_variable m_CondMain;
    std::mutex m_Mutex;

    enum ThreadState
    {
      eWaiting = 0,
      eRunning = 1,
      eStop    = 2,
      eNoRequest = 3
    };

    volatile ThreadState m_ThreadState;
    volatile ThreadState m_Request;

  protected:
    WorkerThread& m_Host;
  };

  unsigned int WorkerThread::GetHardwareConcurrency()
  {
    return std::thread::hardware_concurrency();
  }

  WorkerThread::WorkerThread()
  {
    m_Impl = new WorkerThread_Impl(*this);
  }

  WorkerThread::~WorkerThread()
  {
    delete m_Impl;
  }

  void WorkerThread::Join()
  {
    m_Impl->join();
  }

  void WorkerThread::Start()
  {
    //Interrupt();
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    m_Impl->m_Request = WorkerThread_Impl::eRunning;
    m_Impl->m_CondThread.notify_one();
  }

  void WorkerThread::Interrupt()
  {
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    if(m_Impl->m_ThreadState == WorkerThread_Impl::eRunning)
    {
      m_Impl->m_Request = WorkerThread_Impl::eWaiting;
      m_Impl->m_CondThread.notify_one();
      while(m_Impl->m_ThreadState != WorkerThread_Impl::eWaiting)
      {
        m_Impl->m_CondMain.wait(lock);
      }
    }
  }

  void WorkerThread::Stop()
  {
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    m_Impl->m_Request = WorkerThread_Impl::eStop;
    m_Impl->m_CondThread.notify_one();
  }

  bool WorkerThread::CheckInterrupt()
  {
    std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
    return m_Impl->m_Request == WorkerThread_Impl::eWaiting;
  }

  void WorkerThread::InternalRun()
  {
    bool notStopping = true;
    bool running = false;
    while(notStopping)
    {
      {
        std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
        if(m_Impl->m_Request != WorkerThread_Impl::eNoRequest
          && m_Impl->m_ThreadState != m_Impl->m_Request)
        {
          bool wasRunning = running;

          notStopping = !(m_Impl->m_Request == WorkerThread_Impl::eStop);
          running = notStopping && m_Impl->m_Request == WorkerThread_Impl::eRunning ;
          m_Impl->m_ThreadState = m_Impl->m_Request;
          m_Impl->m_CondMain.notify_one();
          m_Impl->m_Request = WorkerThread_Impl::eNoRequest;

          if(wasRunning && !running)
          {
            SignalStop();
          }
        }
        if(notStopping && !running)
        {
          m_Impl->m_CondThread.wait(lock);
        }
      }

      if(running)
      {
        Run();
        std::unique_lock<std::mutex> lock(m_Impl->m_Mutex);
        m_Impl->m_ThreadState = WorkerThread_Impl::eWaiting;
        running = false;
      }
    }
  }
}