/**
Copyright Nicolas Colombe
2009-2010
*/

#include "sdlkeytranslator.hpp"

namespace eXl
{

  void SDLKeyTranslator::Init()
  {
    TranslatorTable[SDL_SCANCODE_UNKNOWN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_A] = K_A;
    TranslatorTable[SDL_SCANCODE_B] = K_B;
    TranslatorTable[SDL_SCANCODE_C] = K_C;
    TranslatorTable[SDL_SCANCODE_D] = K_D;
    TranslatorTable[SDL_SCANCODE_E] = K_E;
    TranslatorTable[SDL_SCANCODE_F] = K_F;
    TranslatorTable[SDL_SCANCODE_G] = K_G;
    TranslatorTable[SDL_SCANCODE_H] = K_H;
    TranslatorTable[SDL_SCANCODE_I] = K_I;
    TranslatorTable[SDL_SCANCODE_J] = K_J;
    TranslatorTable[SDL_SCANCODE_K] = K_K;
    TranslatorTable[SDL_SCANCODE_L] = K_L;
    TranslatorTable[SDL_SCANCODE_M] = K_M;
    TranslatorTable[SDL_SCANCODE_N] = K_N;
    TranslatorTable[SDL_SCANCODE_O] = K_O;
    TranslatorTable[SDL_SCANCODE_P] = K_P;
    TranslatorTable[SDL_SCANCODE_Q] = K_Q;
    TranslatorTable[SDL_SCANCODE_R] = K_R;
    TranslatorTable[SDL_SCANCODE_S] = K_S;
    TranslatorTable[SDL_SCANCODE_T] = K_T;
    TranslatorTable[SDL_SCANCODE_U] = K_U;
    TranslatorTable[SDL_SCANCODE_V] = K_V;
    TranslatorTable[SDL_SCANCODE_W] = K_W;
    TranslatorTable[SDL_SCANCODE_X] = K_X;
    TranslatorTable[SDL_SCANCODE_Y] = K_Y;
    TranslatorTable[SDL_SCANCODE_Z] = K_Z;
    TranslatorTable[SDL_SCANCODE_1] = K_KB_ONE;
    TranslatorTable[SDL_SCANCODE_2] = K_KB_TWO;
    TranslatorTable[SDL_SCANCODE_3] = K_KB_THREE;
    TranslatorTable[SDL_SCANCODE_4] = K_KB_FOUR;
    TranslatorTable[SDL_SCANCODE_5] = K_KB_FIVE;
    TranslatorTable[SDL_SCANCODE_6] = K_KB_SIX;
    TranslatorTable[SDL_SCANCODE_7] = K_KB_SEVEN;
    TranslatorTable[SDL_SCANCODE_8] = K_KB_EIGHT;
    TranslatorTable[SDL_SCANCODE_9] = K_KB_NINE;
    TranslatorTable[SDL_SCANCODE_0] = K_KB_ZERO;
    TranslatorTable[SDL_SCANCODE_RETURN] = K_ENTER;
    TranslatorTable[SDL_SCANCODE_ESCAPE] = K_ESCAPE;
    TranslatorTable[SDL_SCANCODE_BACKSPACE] = K_BKSP;
    TranslatorTable[SDL_SCANCODE_TAB] = K_TAB;
    TranslatorTable[SDL_SCANCODE_SPACE] = K_SPACE;
    TranslatorTable[SDL_SCANCODE_MINUS] = K_KB_SIX;
    TranslatorTable[SDL_SCANCODE_EQUALS] = K_KB_PLUS;
    TranslatorTable[SDL_SCANCODE_LEFTBRACKET] = K_KB_FIVE;
    TranslatorTable[SDL_SCANCODE_RIGHTBRACKET] = K_KB_ROUND;
    TranslatorTable[SDL_SCANCODE_BACKSLASH] = K_KB_EIGHT;
    TranslatorTable[SDL_SCANCODE_NONUSHASH] = K_KB_EIGHT;
    TranslatorTable[SDL_SCANCODE_SEMICOLON] = K_SEMICOLON;
    TranslatorTable[SDL_SCANCODE_APOSTROPHE] = K_KB_FOUR;
    TranslatorTable[SDL_SCANCODE_GRAVE] = K_KB_SEVEN;
    TranslatorTable[SDL_SCANCODE_COMMA] = K_COMMA;
    TranslatorTable[SDL_SCANCODE_PERIOD] = K_SEMICOLON;
    TranslatorTable[SDL_SCANCODE_SLASH] = K_COLON;
    TranslatorTable[SDL_SCANCODE_CAPSLOCK] = K_CAPSLOCK;
    TranslatorTable[SDL_SCANCODE_F1] = K_F1;
    TranslatorTable[SDL_SCANCODE_F2] = K_F2;
    TranslatorTable[SDL_SCANCODE_F3] = K_F3;
    TranslatorTable[SDL_SCANCODE_F4] = K_F4;
    TranslatorTable[SDL_SCANCODE_F5] = K_F5;
    TranslatorTable[SDL_SCANCODE_F6] = K_F6;
    TranslatorTable[SDL_SCANCODE_F7] = K_F7;
    TranslatorTable[SDL_SCANCODE_F8] = K_F8;
    TranslatorTable[SDL_SCANCODE_F9] = K_F9;
    TranslatorTable[SDL_SCANCODE_F10] = K_F10;
    TranslatorTable[SDL_SCANCODE_F11] = K_F11;
    TranslatorTable[SDL_SCANCODE_F12] = K_F12;
    TranslatorTable[SDL_SCANCODE_PRINTSCREEN] = K_PRNTSCR;
    TranslatorTable[SDL_SCANCODE_SCROLLLOCK] = K_SCROLLLOCK;
    TranslatorTable[SDL_SCANCODE_PAUSE] = K_PAUSE;
    TranslatorTable[SDL_SCANCODE_INSERT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_HOME] = K_ORIG;
    TranslatorTable[SDL_SCANCODE_PAGEUP] = K_PAGEUP;
    TranslatorTable[SDL_SCANCODE_DELETE] = K_SUPPR;
    TranslatorTable[SDL_SCANCODE_END] = K_END;
    TranslatorTable[SDL_SCANCODE_PAGEDOWN] = K_PAGEDOWN;
    TranslatorTable[SDL_SCANCODE_RIGHT] = K_RIGHT;
    TranslatorTable[SDL_SCANCODE_LEFT] = K_LEFT;
    TranslatorTable[SDL_SCANCODE_DOWN] = K_DOWN;
    TranslatorTable[SDL_SCANCODE_UP] = K_UP;
    TranslatorTable[SDL_SCANCODE_NUMLOCKCLEAR] = K_NUMPAD_VERRNUM;
    TranslatorTable[SDL_SCANCODE_KP_DIVIDE] = K_NUMPAD_SLASH;
    TranslatorTable[SDL_SCANCODE_KP_MULTIPLY] = K_NUMPAD_MULTIPLY;
    TranslatorTable[SDL_SCANCODE_KP_MINUS] = K_NUMPAD_MINUS;
    TranslatorTable[SDL_SCANCODE_KP_PLUS] = K_NUMPAD_PLUS;
    TranslatorTable[SDL_SCANCODE_KP_ENTER] = K_NUMPAD_ENTER;
    TranslatorTable[SDL_SCANCODE_KP_1] = K_NUMPAD_0;
    TranslatorTable[SDL_SCANCODE_KP_2] = K_NUMPAD_1;
    TranslatorTable[SDL_SCANCODE_KP_3] = K_NUMPAD_2;
    TranslatorTable[SDL_SCANCODE_KP_4] = K_NUMPAD_3;
    TranslatorTable[SDL_SCANCODE_KP_5] = K_NUMPAD_4;
    TranslatorTable[SDL_SCANCODE_KP_6] = K_NUMPAD_5;
    TranslatorTable[SDL_SCANCODE_KP_7] = K_NUMPAD_6;
    TranslatorTable[SDL_SCANCODE_KP_8] = K_NUMPAD_7;
    TranslatorTable[SDL_SCANCODE_KP_9] = K_NUMPAD_8;
    TranslatorTable[SDL_SCANCODE_KP_0] = K_NUMPAD_9;
    TranslatorTable[SDL_SCANCODE_KP_PERIOD] = K_NUMPAD_DOT;
    TranslatorTable[SDL_SCANCODE_NONUSBACKSLASH] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_APPLICATION] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_POWER] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_EQUALS] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F13] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F14] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F15] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F16] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F17] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F18] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F19] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F20] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F21] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F22] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F23] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_F24] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_EXECUTE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_HELP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_MENU] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_SELECT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_STOP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AGAIN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_UNDO] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CUT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_COPY] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_PASTE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_FIND] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_MUTE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_VOLUMEUP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_VOLUMEDOWN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_COMMA] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_EQUALSAS400] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL1] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL2] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL3] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL4] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL5] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL6] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL7] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL8] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_INTERNATIONAL9] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG1] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG2] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG3] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG4] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG5] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG6] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG7] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG8] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LANG9] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_ALTERASE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_SYSREQ] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CANCEL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CLEAR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_PRIOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_RETURN2] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_SEPARATOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_OUT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_OPER] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CLEARAGAIN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CRSEL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_EXSEL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_00] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_000] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_THOUSANDSSEPARATOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_DECIMALSEPARATOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CURRENCYUNIT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CURRENCYSUBUNIT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_LEFTPAREN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_RIGHTPAREN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_LEFTBRACE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_RIGHTBRACE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_TAB] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_BACKSPACE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_A] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_B] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_C] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_D] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_E] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_F] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_XOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_POWER] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_PERCENT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_LESS] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_GREATER] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_AMPERSAND] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_DBLAMPERSAND] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_VERTICALBAR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_DBLVERTICALBAR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_COLON] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_HASH] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_SPACE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_AT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_EXCLAM] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMSTORE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMRECALL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMCLEAR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMADD] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMSUBTRACT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMMULTIPLY] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_MEMDIVIDE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_PLUSMINUS] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_CLEAR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_CLEARENTRY] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_BINARY] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_OCTAL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_DECIMAL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KP_HEXADECIMAL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_LCTRL] = K_LCTRL;
    TranslatorTable[SDL_SCANCODE_LSHIFT] = K_LSHIFT;
    TranslatorTable[SDL_SCANCODE_LALT] = K_LALT;
    TranslatorTable[SDL_SCANCODE_LGUI] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_RCTRL] = K_RCTRL;
    TranslatorTable[SDL_SCANCODE_RSHIFT] = K_RSHIFT;
    TranslatorTable[SDL_SCANCODE_RALT] = K_RALT;
    TranslatorTable[SDL_SCANCODE_RGUI] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_MODE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AUDIONEXT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AUDIOPREV] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AUDIOSTOP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AUDIOPLAY] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AUDIOMUTE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_MEDIASELECT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_WWW] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_MAIL] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_CALCULATOR] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_COMPUTER] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_SEARCH] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_HOME] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_BACK] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_FORWARD] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_STOP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_REFRESH] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_AC_BOOKMARKS] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_BRIGHTNESSDOWN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_BRIGHTNESSUP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_DISPLAYSWITCH] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KBDILLUMTOGGLE] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KBDILLUMDOWN] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_KBDILLUMUP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_EJECT] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_SLEEP] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_APP1] = K_NULLKEY;
    TranslatorTable[SDL_SCANCODE_APP2] = K_NULLKEY;
  }
}