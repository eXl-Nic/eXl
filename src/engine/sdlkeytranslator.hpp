/**
Copyright Nicolas Colombe
2009-2010
*/

#ifndef SDLKEYTRANSLATOR_INCLUDED
#define SDLKEYTRANSLATOR_INCLUDED

#include <SDL_keycode.h>
#include <core/base/keytranslator.hpp>

namespace eXl
{
  class SDLKeyTranslator : public KeyTranslator<SDL_Scancode,SDL_NUM_SCANCODES>
  {
  public:
    static void Init();
  };
}

#endif
