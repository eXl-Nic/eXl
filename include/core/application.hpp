/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/path.hpp>

namespace eXl
{
  
  class EXL_CORE_API Application
  {
  public:
    
    static Application& GetAppl();

    Application();

    virtual ~Application();
    
    inline void SetArgs(int iArgc, char** iArgv)
    {
      m_Argc = iArgc;
      m_ArgV = iArgv;
    }
  
    virtual void Start();

    virtual void Terminated();

    void DefaultLoop();

    virtual void Tick(float iDelta)
    {
      PumpMessages(iDelta);
    }

    virtual void PumpMessages(float iDelta)
    {}

    void Stop()
    {
      running = false;
    }

    bool IsRunning()
    {
      return running;
    }

    void SetWindowSize(unsigned int iWidth, unsigned int iHeight)
    {
      m_Width = iWidth;
      m_Height = iHeight;
    }

    void GetWindowSize(unsigned int& oWidth, unsigned int& oHeight)
    {
      oWidth = m_Width;
      oHeight = m_Height;
    }

    uint32_t GetSeed()
    {
      return m_Seed;
    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    Path const& GetAppPath()
    {
      return m_AppPath;
    }
    void SetAppPath(Path const& iPath)
    { 
      m_AppPath = iPath;
    }
#endif
  protected:
    
    int m_Argc;
    char** m_ArgV;

    bool running;
#ifdef EXL_RSC_HAS_FILESYSTEM
    Path m_AppPath;
#endif
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_Seed = 0;
  };
}
