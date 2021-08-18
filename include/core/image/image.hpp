/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/corelibexp.hpp>
#include <math/vector2.hpp>
#include <core/heapobject.hpp>

namespace eXl
{
  class EXL_CORE_API Image : public HeapObject
  {
  public:

    enum Components
    {
      R    = 0,
      RG   = 1,
      RGB  = 2,
      BGR  = 3,
      RGBA = 4,
      BGRA = 5,
    };

    enum Format
    {
      Char  = 0,
      Short = 1,
      Int   = 2,
      Float = 3
    };

    enum Storage
    {
      Copy      = 0,
      Adopt     = 1,
      Reference = 2

    };

    typedef Vector2<uint32_t> Size;

    Image(void* iData, Size const& iSize, Components iComp, Format iFormat, unsigned char iRowAlign, Storage iStorage = Copy);
    Image(Image const& iImage);
	  Image(Image&& iImage);
    Image& operator=(Image const& iImage)
    {
      this->~Image();
      new (this) Image(iImage);
      return *this;
    }
    Image& operator=(Image&& iImage)
    {
      this->~Image();
      new (this) Image(std::move(iImage));
      return *this;
    }
    ~Image();

    //iFilter must be a Float:R image
    void Convolve(Image const& iFilter, Vector2i const& iFilterOffset);

    size_t GetByteSize() const;
    size_t GetPixelSize() const;

    inline void*           GetRow(unsigned int iRow)                            {return I_GetRow(iRow);}
    inline void const*     GetRow(unsigned int iRow)                      const {return I_GetRow(iRow);}
    inline void*           GetPixel(unsigned int iRow, unsigned int iCol)       {return I_GetPixel(iRow,iCol);}
    inline void const*     GetPixel(unsigned int iRow, unsigned int iCol) const {return I_GetPixel(iRow,iCol);}

    inline Size const&     GetSize()        const {return m_Size;}
    inline Components      GetComponents()  const {return Components(m_ImageFormat & 7);}
    inline Format          GetFormat()      const {return Format((m_ImageFormat>>3) & 3);}
    inline unsigned int    GetRowStride()   const {return m_RowStride;}
    inline void const*     GetImageData()   const {return m_ImageData;}
    inline void *          GetImageData()         {return m_ImageData;}

  protected:

    void* I_GetRow(unsigned int iRow) const;
    void* I_GetPixel(unsigned int iRow, unsigned int iCol) const;

    void Alloc();

    void*         m_ImageAlloc = nullptr;
    // Aligned;
    void*         m_ImageData = nullptr;
    Size          m_Size;
    unsigned int  m_RowStride;
    unsigned char m_ImageFormat;
    unsigned char m_StorageKind;
    unsigned char m_RowAlign;
    
  };
}
