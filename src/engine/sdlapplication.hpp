#include EXL_TEXT("application.hpp")

namespace eXl
{
  class SDLView;

  class APPLTOOLS_API SDLApplication : public Application
  {
  public:

    SDLApplication();
    
    void Start();

    void Terminated();

    GfxView* GetDefaultView();

    void PumpMessages();

    void SwapView(GfxView*);
  protected:
    SDLView* m_View;
  };
}