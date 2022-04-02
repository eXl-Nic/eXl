/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <math/vector2.hpp>

namespace eXl
{
  template <typename T>
  class Pattern
  {
  public:

    Pattern()
    {}

    Pattern(Vec2i const& iSize) : m_Size(iSize)
    {
      m_Bitmap.resize(m_Size.x * m_Size.y);
    }

    void SetSize(Vec2i const& iSize, T const& iInit = T())
    {
      if(iSize != m_Size)
      {
        m_Size = iSize;
        m_Bitmap.resize(m_Size.x * m_Size.y, iInit);
      }
    }

    Pattern Rotate() const
    {
      Pattern newPattern;
      newPattern.m_Size = Vec2i(m_Size.y, m_Size.x);
      newPattern.m_Bitmap.resize(m_Bitmap.size());
      unsigned int localOffset = 0;
      for(unsigned int i = 0; i<m_Size.y; ++i)
      {
        for(unsigned int j = 0; j<m_Size.x; ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vec2i(m_Size.y - 1 - i, j))] = m_Bitmap[localOffset];
          ++localOffset;
        }
      }
      return newPattern;
    }

    Pattern ReflectX() const
    {
      Pattern newPattern;
      newPattern.m_Size = m_Size;
      newPattern.m_Bitmap.resize(m_Bitmap.size());
      unsigned int localOffset = 0;
      for(unsigned int i = 0; i<m_Size.y; ++i)
      {
        for(unsigned int j = 0; j<m_Size.x; ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vec2i(m_Size.x - 1 - j, i))] = m_Bitmap[localOffset];
          ++localOffset;
        }
      }
      return newPattern;
    }

    Pattern ReflectY() const
    {
      Pattern newPattern;
      newPattern.m_Size = m_Size;
      newPattern.m_Bitmap.resize(m_Bitmap.size());
      unsigned int localOffset = 0;
      for(unsigned int i = 0; i<m_Size.y; ++i)
      {
        for(unsigned int j = 0; j<m_Size.x; ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vec2i(j, m_Size.y - 1 - i))] = m_Bitmap[localOffset];
          ++localOffset;
        }
      }
      return newPattern;
    }

    Pattern Upsample(unsigned int iMult) const
    {
      Pattern result(GetSize() * iMult);

      for(unsigned int y = 0; y<GetSize().y; ++y)
      {
        for(unsigned int i = 0; i<iMult; ++i)
        {
          unsigned int localOffset = y * GetSize().x;
          unsigned int destOffset = (y * iMult + i) * GetSize().x * iMult;
          for(unsigned int x = 0; x<GetSize().x; ++x)
          {
            for(unsigned int j = 0; j<iMult; ++j)
            {
              result.GetBitmap()[destOffset] = GetBitmap()[localOffset];
              ++destOffset;
            }
            ++localOffset;
          }
        }
      }
      return result;
    }

    Vec2i& GetSize() {return m_Size;}
    Vector<T>& GetBitmap() { return m_Bitmap;}

    T& operator[](Vec2i const& iPos) { return m_Bitmap[GetOffset(iPos)]; }
    T const& operator[](Vec2i const& iPos) const { return m_Bitmap[GetOffset(iPos)]; }
    

    T Get(Vec2i const& iCoord) const {return m_Bitmap[GetOffset(iCoord)];}
    void Set(Vec2i const& iCoord, T const& iValue){m_Bitmap[GetOffset(iCoord)] = iValue;}

    Vec2i const& GetSize() const {return m_Size;}
    Vector<T> const& GetBitmap() const { return m_Bitmap;}

    inline unsigned int GetOffset(Vec2i const& iCoord) const
    {
      return m_Size.x * iCoord.y + iCoord.x;
    }

  protected:

    Vec2i  m_Size;
    Vector<T> m_Bitmap;
  };
}