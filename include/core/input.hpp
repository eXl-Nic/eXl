/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/keys.hpp>
#include <core/containers.hpp>


namespace eXl
{
  enum class MouseButton
  {
    Left = 1,
    Middle = 2,
    Right = 3,
  };
  
  enum class TouchKind
  {
    Up = 1,
    Down = 2,
    Move = 3,
    Cancel = 0,
  };
  

  struct MouseEvent
  {
    bool operator ==(MouseEvent const& iOther)const{return button == iOther.button && pressed == iOther.pressed;}
    MouseButton button;
    bool pressed;
  };

  struct TouchEvent
  {
    bool operator ==(TouchEvent const& iOther)const
    {
      return (absX - iOther.absX | absY - iOther.absY | relX - iOther.relX | relY - iOther.relY | gesture - iOther.gesture | idx - iOther.idx) == 0;
    }
    uint32_t absX;
    uint32_t absY;
    int32_t relX;
    int32_t relY;
    uint8_t gesture;
    uint8_t idx;
  };

  struct MouseMoveEvent
  {
    bool operator ==(MouseMoveEvent const& iOther)const{
      return (absX - iOther.absX | absY - iOther.absY | relX - iOther.relX | relY - iOther.relY) == 0 &&  wheel == iOther.wheel;
    }
    uint32_t absX;
    uint32_t absY;
    int32_t relX;
    int32_t relY;
    bool wheel;
  };

  struct KeyboardEvent
  {
    inline bool operator ==(KeyboardEvent const& iOther)const{return text == iOther.text && key == iOther.key && pressed == iOther.pressed;}
    uint32_t text;
    Key key;
    bool pressed;
  };

  class EXL_CORE_API InputSystem
  {

  public:

    void InjectKeyEvent(uint32_t iText, Key iKey, bool iPressed)
    {
      KeyboardEvent evt = { iText, iKey, iPressed };
      m_KeyEvts.push_back(evt);
    }

    void InjectMouseEvent(MouseButton iButton, bool iPressed)
    {
      MouseEvent evt = { iButton, iPressed };
      m_MouseEvts.push_back(evt);
    }

    void InjectMouseMoveEvent(uint32_t iAbsX, uint32_t iAbsY, int32_t iRelX, int32_t iRelY, bool iWheel)
    {
      MouseMoveEvent evt = { iAbsX, iAbsY, iRelX, iRelY, iWheel };
      m_MouseMoveEvts.push_back(evt);
    }

    void Clear()
    {
      m_KeyEvts.clear();
      m_MouseEvts.clear();
      m_MouseMoveEvts.clear();
    }

    Vector<KeyboardEvent> m_KeyEvts;
    Vector<MouseEvent> m_MouseEvts;
    Vector<MouseMoveEvent> m_MouseMoveEvts;
        
  };
}
