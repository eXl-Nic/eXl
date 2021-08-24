/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/application.hpp>

#include <core/corelib.hpp>
#include <core/coretest.hpp>
#include <core/plugin.hpp>
#include <core/log.hpp>
#include <core/clock.hpp>

#include <vector>
#include <thread>

#ifdef __ANDROID__
#include <unistd.h>
#endif

namespace eXl
{

  static Application* s_Instance = nullptr;

  Application::Application():m_Width(1024),m_Height(768)
  { 
    eXl::StartCoreLib(nullptr);
    {
      LOG_INFO<<"Started Core lib"<<"\n";
    }
    
    running = false;
    eXl_ASSERT_MSG(s_Instance == nullptr, "Nope");
    s_Instance = this;

  }

  Application::~Application()
  {}

  Application& Application::GetAppl()
  {
    eXl_ASSERT_MSG(s_Instance != nullptr,"Too early !!");
    //if(s_Instance == NULL)
    //{
    //  s_Instance = new Application;
    //}
    return *s_Instance;
  }

  void Application::Start()
  {
    running = true;
  }

  void Application::DefaultLoop()
  {
    float const maxFrameTime = 1.0 / 60.0;
    Clock clock;
    while (IsRunning())
    {
      uint64_t curTimestamp = Clock::GetTimestamp();
      float delta = clock.GetTime();
      Tick(delta);
      uint64_t afterTickTimestamp = Clock::GetTimestamp();
      uint64_t nextTimestamp = curTimestamp + maxFrameTime * Clock::GetTicksPerSecond();
      if (nextTimestamp > afterTickTimestamp)
      {
        uint64_t sleepTimeInTicks = nextTimestamp - afterTickTimestamp;
        uint64_t sleepTimeInMs = 1000.0 * double(sleepTimeInTicks) / Clock::GetTicksPerSecond();
#ifdef __ANDROID__
        usleep(1000 * sleepTimeInMs);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeInMs));
#endif
      }
    }
  }

  void Application::Terminated()
  {
    
    eXl::StopCoreLib();
  }

  
}
