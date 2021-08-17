
#include EXL_TEXT("sdlapplication.hpp")
#include EXL_TEXT("sdlkeytranslator.hpp")
#include <SDL.h>

#include <gametk/gfxsys.hpp>
#include <gametk/inputsystem.hpp>
#include <core/base/log.hpp>
#include <core/kernel/subsystem.hpp>

namespace eXl
{
  void* SDLGetWinHandle(SDL_Window* iWin);

  SDLApplication::SDLApplication()
    :m_View(NULL)
  {
    SDLKeyTranslator::Init();
  }
    
  void SDLApplication::Start()
  {
    LOG_INFO<<EXL_TEXT("SDLAppl Start")<<EXL_TEXT("\n");
    Log_Manager::EnableStream(LUA_OUT_STREAM);
    Application::Start();
  }

  void SDLApplication::Terminated()
  {
    Application::Terminated();
  }

  class SDLView : public GfxView
  {
  public:
    SDLView(SDL_Window* window, Vector2i const& iSize)
      :GfxView(iSize)
      ,m_Window(window)
    {
      
    }

    SDL_Window* GetWindow(){return m_Window;}

    void* GetWinHandle()
    {
     return SDLGetWinHandle(m_Window);
    }

  protected:
    SDL_Window* m_Window;
  };

  GfxView* SDLApplication::GetDefaultView()
  {
    if(m_View == NULL)
    {
      SDL_Init(SDL_INIT_VIDEO);

      SDL_Window* win = SDL_CreateWindow(EXL_TEXT("eXl"),0,0,m_Width,m_Height,SDL_WINDOW_OPENGL);
    
      SDL_GLContext context;
      //Enable alpha channel (for glReadPixel w/o FBO.)
      //SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8);
      context = SDL_GL_CreateContext(win);

      //GetGFXSysInitData()->winHandle = SDLGetWinHandle(win);
      //GetGFXSysInitData()->W = m_Width;
      //GetGFXSysInitData()->H = m_Height;
      //m_WindowHandle = win;
      m_View = eXl_NEW SDLView(win,Vector2i(m_Width,m_Height));
    }
    return m_View;
  }

  void SDLApplication::PumpMessages()
  {
    SDL_Event evt;
    while(SDL_PollEvent(&evt))
    {
      switch(evt.type)
      {
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        if(evt.key.repeat == 0)
        {
          unsigned int text = (evt.key.keysym.sym & SDLK_SCANCODE_MASK) == 0 ? evt.key.keysym.sym : 0;
          InputSystem::InjectKeyEvent(
            text,
            SDLKeyTranslator::Translate(evt.key.keysym.scancode),
            evt.type == SDL_KEYDOWN
            );
        }
        break;
      case SDL_MOUSEMOTION:
        InputSystem::InjectMouseMoveEvent(evt.motion.x,evt.motion.y,evt.motion.xrel,evt.motion.yrel,false);
        break;
      case SDL_MOUSEWHEEL:
        InputSystem::InjectMouseMoveEvent(evt.wheel.y,evt.wheel.y,evt.wheel.y,evt.wheel.y,true);
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        InputSystem::InjectMouseEvent(evt.button.button,evt.type==SDL_MOUSEBUTTONDOWN);
        break;
      case SDL_QUIT:
        Stop();
        return;
      default:
        break;
      }
    }
  }

  void SDLApplication::SwapView(GfxView* iView)
  {
    GetGfxSys()->RenderOneFrame(iView);
    SDL_GL_SwapWindow(static_cast<SDLView*>(iView)->GetWindow());
  }
}

#include <SDL_syswm.h>

namespace eXl
{
  void* SDLGetWinHandle(SDL_Window* iWin)
  {
    SDL_SysWMinfo winInfo;
    SDL_VERSION(&winInfo.version);
    SDL_GetWindowWMInfo(iWin,&winInfo);
#ifdef WIN32
    return winInfo.info.win.window;
#else ifdef __LINUX__
    return winInfo.info.x11.window;
#endif
  }
}
