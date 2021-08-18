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

    Pattern(Vector2i const& iSize) : m_Size(iSize)
    {
      m_Bitmap.resize(m_Size.X() * m_Size.Y());
    }

    void SetSize(Vector2i const& iSize, T const& iInit = T())
    {
      if(iSize != m_Size)
      {
        m_Size = iSize;
        m_Bitmap.resize(m_Size.X() * m_Size.Y(), iInit);
      }
    }

    Pattern Rotate() const
    {
      Pattern newPattern;
      newPattern.m_Size = Vector2i(m_Size.Y(), m_Size.X());
      newPattern.m_Bitmap.resize(m_Bitmap.size());
      unsigned int localOffset = 0;
      for(unsigned int i = 0; i<m_Size.Y(); ++i)
      {
        for(unsigned int j = 0; j<m_Size.X(); ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vector2i(m_Size.Y() - 1 - i, j))] = m_Bitmap[localOffset];
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
      for(unsigned int i = 0; i<m_Size.Y(); ++i)
      {
        for(unsigned int j = 0; j<m_Size.X(); ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vector2i(m_Size.X() - 1 - j, i))] = m_Bitmap[localOffset];
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
      for(unsigned int i = 0; i<m_Size.Y(); ++i)
      {
        for(unsigned int j = 0; j<m_Size.X(); ++j)
        {
          newPattern.m_Bitmap[GetOffset(Vector2i(j, m_Size.Y() - 1 - i))] = m_Bitmap[localOffset];
          ++localOffset;
        }
      }
      return newPattern;
    }

    Pattern Upsample(unsigned int iMult) const
    {
      Pattern result(GetSize() * iMult);

      for(unsigned int y = 0; y<GetSize().Y(); ++y)
      {
        for(unsigned int i = 0; i<iMult; ++i)
        {
          unsigned int localOffset = y * GetSize().X();
          unsigned int destOffset = (y * iMult + i) * GetSize().X() * iMult;
          for(unsigned int x = 0; x<GetSize().X(); ++x)
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

    Vector2i& GetSize() {return m_Size;}
    Vector<T>& GetBitmap() { return m_Bitmap;}

    T& operator[](Vector2i const& iPos) { return m_Bitmap[GetOffset(iPos)]; }
    T const& operator[](Vector2i const& iPos) const { return m_Bitmap[GetOffset(iPos)]; }
    

    T Get(Vector2i const& iCoord) const {return m_Bitmap[GetOffset(iCoord)];}
    void Set(Vector2i const& iCoord, T const& iValue){m_Bitmap[GetOffset(iCoord)] = iValue;}

    Vector2i const& GetSize() const {return m_Size;}
    Vector<T> const& GetBitmap() const { return m_Bitmap;}

    inline unsigned int GetOffset(Vector2i const& iCoord) const
    {
      return m_Size.X() * iCoord.Y() + iCoord.X();
    }

  protected:

    Vector2i  m_Size;
    Vector<T> m_Bitmap;
  };
}