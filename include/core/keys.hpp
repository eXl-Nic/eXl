/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace eXl
{
  typedef unsigned char Key;
  static const unsigned char       K_NULLKEY         =0x00;
  static const unsigned char       K_ESCAPE          =0x01;
  static const unsigned char       K_KB_ONE          =0x02;
  static const unsigned char       K_KB_TWO          =0x03;
  static const unsigned char       K_KB_THREE        =0x04;
  static const unsigned char       K_KB_FOUR         =0x05;
  static const unsigned char       K_KB_FIVE         =0x06;
  static const unsigned char       K_KB_SIX          =0x07;
  static const unsigned char       K_KB_SEVEN        =0x08;
  static const unsigned char       K_KB_EIGHT        =0x09;
  static const unsigned char       K_KB_NINE         =0x0A;
  static const unsigned char       K_KB_ZERO         =0x0B;
  static const unsigned char       K_KB_ROUND        =0x0C;
  static const unsigned char       K_KB_PLUS         =0x0D;
  static const unsigned char       K_BKSP            =0x0E;
  static const unsigned char       K_TAB             =0x0F;
  static const unsigned char       K_A               =0x10;
  static const unsigned char       K_Z               =0x11;
  static const unsigned char       K_E               =0x12;
  static const unsigned char       K_R               =0x13;
  static const unsigned char       K_T               =0x14;
  static const unsigned char       K_Y               =0x15;
  static const unsigned char       K_U               =0x16;
  static const unsigned char       K_I               =0x17;
  static const unsigned char       K_O               =0x18;
  static const unsigned char       K_P               =0x19;
  static const unsigned char       K_TREMA           =0x1A;
  static const unsigned char       K_POUND           =0x1B;
  static const unsigned char       K_ENTER           =0x1C;
  static const unsigned char       K_LCTRL           =0x1D;
  static const unsigned char       K_Q               =0x1E;
  static const unsigned char       K_S               =0x1F;
  static const unsigned char       K_D               =0x20;
  static const unsigned char       K_F               =0x21;
  static const unsigned char       K_G               =0x22;
  static const unsigned char       K_H               =0x23;
  static const unsigned char       K_J               =0x24;
  static const unsigned char       K_K               =0x25;
  static const unsigned char       K_L               =0x26;
  static const unsigned char       K_M               =0x27;
  static const unsigned char       K_UACCENT         =0x28;
  static const unsigned char       K_SQUARE          =0x29;
  static const unsigned char       K_LSHIFT          =0x2A;
  static const unsigned char       K_ASTERISK        =0x2B;
  static const unsigned char       K_W               =0x2C;
  static const unsigned char       K_X               =0x2D;
  static const unsigned char       K_C               =0x2E;
  static const unsigned char       K_V               =0x2F;
  static const unsigned char       K_B               =0x30;
  static const unsigned char       K_N               =0x31;
  static const unsigned char       K_COMMA           =0x32;
  static const unsigned char       K_SEMICOLON       =0x33;
  static const unsigned char       K_COLON           =0x34;
  static const unsigned char       K_EXCLAMATIONMARK =0x35;
  static const unsigned char       K_RSHIFT          =0x36;
  static const unsigned char       K_NUMPAD_MULTIPLY =0x37;
  static const unsigned char       K_LALT            =0x38;
  static const unsigned char       K_SPACE           =0x39;
  static const unsigned char       K_CAPSLOCK        =0x3A;
  static const unsigned char       K_F1              =0x3B;
  static const unsigned char       K_F2              =0x3C;
  static const unsigned char       K_F3              =0x3D;
  static const unsigned char       K_F4              =0x3E;
  static const unsigned char       K_F5              =0x3F;
  static const unsigned char       K_F6              =0x40;
  static const unsigned char       K_F7              =0x41;
  static const unsigned char       K_F8              =0x42;
  static const unsigned char       K_F9              =0x43;
  static const unsigned char       K_F10             =0x44;
  static const unsigned char       K_NUMPAD_VERRNUM  =0x45;
  static const unsigned char       K_SCROLLLOCK      =0x46;
  static const unsigned char       K_NUMPAD_7        =0x47;
  static const unsigned char       K_NUMPAD_8        =0x48;
  static const unsigned char       K_NUMPAD_9        =0x49;
  static const unsigned char       K_NUMPAD_MINUS    =0x4A;
  static const unsigned char       K_NUMPAD_4        =0x4B;
  static const unsigned char       K_NUMPAD_5        =0x4C;
  static const unsigned char       K_NUMPAD_6        =0x4D;
  static const unsigned char       K_NUMPAD_PLUS     =0x4E;
  static const unsigned char       K_NUMPAD_1        =0x4F;
  static const unsigned char       K_NUMPAD_2        =0x50;
  static const unsigned char       K_NUMPAD_3        =0x51;
  static const unsigned char       K_NUMPAD_0        =0x52;
  static const unsigned char       K_NUMPAD_DOT      =0x53;

  static const unsigned char       K_INFERIOR        =0x56;
  static const unsigned char       K_F11             =0x57;
  static const unsigned char       K_F12             =0x58;
  
  static const unsigned char       K_NUMPAD_ENTER    =0x9C;
  static const unsigned char       K_RCTRL           =0x9D;

  static const unsigned char       K_NUMPAD_SLASH    =0xB5;

  static const unsigned char       K_PRNTSCR         =0xB7;
  static const unsigned char       K_RALT            =0xB8;

  static const unsigned char       K_PAUSE           =0xC5;

  static const unsigned char       K_ORIG            =0xC7;
  static const unsigned char       K_UP              =0xC8;
  static const unsigned char       K_PAGEUP          =0xC9;
  
  static const unsigned char       K_LEFT            =0xCB;
  
  static const unsigned char       K_RIGHT           =0xCD;

  static const unsigned char	     K_END             =0xCF;

  static const unsigned char       K_DOWN            =0xD0;
  static const unsigned char       K_PAGEDOWN        =0xD1;

  static const unsigned char       K_SUPPR           =0xD3;
  
}
