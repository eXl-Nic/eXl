/**

Copyright Nicolas Colombe
2009-2019

This file is part of eXl_Gen.

eXl_Gen is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

eXl_Gen is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with eXl_Gen.  If not, see <http://www.gnu.org/licenses/>.
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