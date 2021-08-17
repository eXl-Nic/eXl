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
    unsigned int m_Width = 0;
    unsigned int m_Height = 0;
  };
}
