/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/image/image.hpp>
#include <cstring>

#include <core/log.hpp>

namespace eXl
{
  namespace
  {
    size_t compTab[] = {1,2,3,3,4,4};
    size_t compSizeTab[] = {1,2,4,4};
                           //R, RG, RGB, BGR, RGBA, BGRA, 0, 0
    size_t pixelSizeTab[] = {1, 2,  3,   3,   4,    4   , 0, 0,     //Char
                             2, 4,  6,   6,   8,    8   , 0, 0,     //Short
                             4, 8,  12,  12,  16,   16  , 0, 0,     //Int
                             4, 8,  12,  12,  16,   16  , 0, 0};    //Float
    
    template <typename T, unsigned int numComp>
    void GenericMultiply(float iFloat, unsigned char const* iVal, unsigned char* oVal)
    {
      T const* origVal = reinterpret_cast<T const*>(iVal);
      T * result = reinterpret_cast<T *>(oVal);
      for(unsigned int i = 0; i<numComp; ++i)
      {
        result[i] += origVal[i] * iFloat;
      }
    }

    void (*multiplierTab [])(float iFloat, unsigned char const* iVal, unsigned char* oVal) =
    {
      &GenericMultiply<unsigned char,1>,  &GenericMultiply<unsigned char,2>,  &GenericMultiply<unsigned char,3>,  &GenericMultiply<unsigned char,3>,  &GenericMultiply<unsigned char,4>,  &GenericMultiply<unsigned char,4>,  nullptr,nullptr,
      &GenericMultiply<unsigned short,1>, &GenericMultiply<unsigned short,2>, &GenericMultiply<unsigned short,3>, &GenericMultiply<unsigned short,3>, &GenericMultiply<unsigned short,4>, &GenericMultiply<unsigned short,4>, nullptr,nullptr,
      &GenericMultiply<unsigned int,1>,   &GenericMultiply<unsigned int,2>,   &GenericMultiply<unsigned int,3>,   &GenericMultiply<unsigned int,3>,   &GenericMultiply<unsigned int,4>,   &GenericMultiply<unsigned int,4>,   nullptr,nullptr,
      &GenericMultiply<float,1>,          &GenericMultiply<float,2>,          &GenericMultiply<float,3>,          &GenericMultiply<float,3>,          &GenericMultiply<float,4>,          &GenericMultiply<float,4>,          nullptr,nullptr
    };
    
  }

  void Image::Alloc()
  {
    if (m_ImageAlloc)
    {
      eXl_FREE(m_ImageAlloc);
      m_ImageData = nullptr;
    }

    m_ImageAlloc = eXl_ALLOC(GetByteSize() + m_RowAlign);
    uint8_t* imageData = (uint8_t*)m_ImageAlloc;
    while (((ptrdiff_t)imageData) % m_RowAlign != 0)
    {
      ++imageData;
    }
    m_ImageData = imageData;
  }

  Image::Image(void* iData, Size const& iSize, Components iComp, Format iFormat, unsigned char iRowAlign, Storage iStorage)
    :m_ImageData(nullptr)
    ,m_Size(iSize)
    ,m_RowStride(0)
    ,m_RowAlign(iRowAlign)
    ,m_ImageFormat((iFormat << 3) | iComp)
    ,m_StorageKind(iStorage == Reference ? Reference : Adopt)
  {
    m_RowStride = iSize.X() * pixelSizeTab[m_ImageFormat];
    unsigned int padding = m_RowStride % iRowAlign;
    if(padding != 0)
    {
      padding = iRowAlign - padding;
    }
    m_RowStride += padding;
    if (iStorage == Copy && GetByteSize() > 0)
    {
      Alloc();
      if (iData != nullptr)
      {
        memcpy(m_ImageData, iData, GetByteSize());
      }
    }
    else if (iStorage == Adopt && GetByteSize() > 0 && iData == nullptr)
    {
      Alloc();
    }
    else
    {
      m_ImageAlloc = iData;
      m_ImageData = iData;
    }
  }

  Image::Image(Image const& iImage)
    :m_ImageData(nullptr)
    ,m_Size(iImage.m_Size)
    ,m_RowStride(iImage.m_RowStride)
    ,m_RowAlign(iImage.m_RowAlign)
    ,m_ImageFormat(iImage.m_ImageFormat)
    ,m_StorageKind(Adopt)
  {
    if(GetByteSize() > 0)
    {
      //m_ImageData = eXl_ALLOC(GetByteSize());
      Alloc();
      if(iImage.m_ImageData)
      {
        memcpy(m_ImageData,iImage.GetImageData(),GetByteSize());
      }
    }
  }

  Image::Image(Image&& iImage)
    : m_ImageData(iImage.m_ImageData)
    , m_ImageAlloc(iImage.m_ImageAlloc)
	  , m_Size(iImage.m_Size)
	  , m_RowStride(iImage.m_RowStride)
    , m_RowAlign(iImage.m_RowAlign)
	  , m_ImageFormat(iImage.m_ImageFormat)
	  , m_StorageKind(iImage.m_StorageKind == Reference ? Reference : Adopt)
  {
	  iImage.m_ImageData = nullptr;
    iImage.m_ImageAlloc = nullptr;
	  iImage.m_Size = Size(0,0);
	  iImage.m_StorageKind = Adopt;
  }

  Image::~Image()
  {
    if(m_StorageKind == Adopt && m_ImageAlloc != nullptr)
    {
      eXl_FREE(m_ImageAlloc);
    }
    m_ImageData = nullptr;
    m_ImageAlloc = nullptr;
  }

  size_t Image::GetByteSize() const
  {
    return m_Size.Y() * m_RowStride;
  }

  size_t Image::GetPixelSize() const
  {
    return pixelSizeTab[m_ImageFormat];
  }

  void* Image::I_GetRow(unsigned int iRow) const
  {
    if(iRow < m_Size.Y())
    {
      return reinterpret_cast<char*>(m_ImageData) + iRow * m_RowStride; 
    }
    return nullptr;
  }

  void* Image::I_GetPixel(unsigned int iRow, unsigned int iCol) const
  {
    if(iRow < m_Size.Y() && iCol < m_Size.X())
    {
      return reinterpret_cast<char*>(m_ImageData) + iRow * m_RowStride + iCol * GetPixelSize(); 
    }
    return nullptr;
  }

  void Image::Convolve(Image const& iFilter, Vector2i const& iFilterOffset)
  {
    if(iFilter.GetComponents() != R || iFilter.GetFormat() != Float)
      return;

    Vector2i minFilter = iFilterOffset;
    Vector2i maxFilter = Vector2i(iFilter.GetSize().X(), iFilter.GetSize().Y()) + iFilterOffset;
    Size filterSize = iFilter.GetSize();

    Vector2i minImage = Vector2i(Mathi::Max(0,-minFilter.X()), Mathi::Max(0,-minFilter.Y()));
    Vector2i maxImage = Vector2i(Mathi::Min(m_Size.X(),m_Size.X() - (maxFilter.X() - 1)), Mathi::Min(m_Size.Y(),m_Size.Y() - (maxFilter.Y() - 1)));

    void (*multiplier)(float , unsigned char const* , unsigned char* ) = multiplierTab[m_ImageFormat];

    unsigned char* newImage = (unsigned char*)eXl_ALLOC(GetByteSize());

    memcpy(newImage,m_ImageData,GetByteSize());

    //for(unsigned int i = 0; i < minImage.Y(); ++i)
    //{
    //  unsigned char const* pixels = reinterpret_cast<unsigned char*>(m_ImageData) + i*m_RowStride;
    //  unsigned char*       newVal = reinterpret_cast<unsigned char*>(newImage)    + i*m_RowStride;
    //  memcpy(newVal,pixels,m_RowStride);
    //  
    //}
    //
    //for(unsigned int i = minImage.Y(); i < maxImage.Y(); ++i)
    //{
    //  unsigned char const* pixels = reinterpret_cast<unsigned char*>(m_ImageData) + i*m_RowStride;
    //  unsigned char*       newVal = reinterpret_cast<unsigned char*>(newImage)    + i*m_RowStride;
    //  for(unsigned int j = 0; j< minImage.X(); ++j)
    //  {
    //    memcpy(newVal,pixels,GetPixelSize());
    //    //_CrtCheckMemory();
    //    pixels += GetPixelSize();
    //    newVal += GetPixelSize();
    //  }
    //  pixels = reinterpret_cast<unsigned char*>(m_ImageData) + i*m_RowStride + maxImage.X() * GetPixelSize();
    //  newVal = reinterpret_cast<unsigned char*>(newImage)    + i*m_RowStride + maxImage.X() * GetPixelSize();
    //  for(unsigned int j = maxImage.X(); j< m_Size.X(); ++j)
    //  {
    //    memcpy(newVal,pixels,GetPixelSize());
    //    //_CrtCheckMemory();
    //    pixels += GetPixelSize();
    //    newVal += GetPixelSize();
    //  }
    //}
    //
    //for(unsigned int i = maxImage.Y(); i < m_Size.Y(); ++i)
    //{
    //  unsigned char const* pixels = reinterpret_cast<unsigned char*>(m_ImageData) + i*m_RowStride;
    //  unsigned char*       newVal = reinterpret_cast<unsigned char*>(newImage)    + i*m_RowStride;
    //  memcpy(newVal,pixels,m_RowStride);
    //}

    for(int i = minImage.Y(); i< maxImage.Y(); ++i)
    {
      for(int j = minImage.X(); j< maxImage.X(); ++j)
      {
        unsigned char* value = newImage + i*m_RowStride + j * GetPixelSize();
        unsigned char const* pixels = reinterpret_cast<unsigned char const*>(m_ImageData) + i *m_RowStride + j * GetPixelSize();
        memset(value,0,GetPixelSize());
        for(int l = minFilter.Y(); l<maxFilter.Y(); ++l)
        {
          float const* filterData = reinterpret_cast<float const*>(reinterpret_cast<unsigned char const*>(iFilter.GetImageData()) + iFilter.GetRowStride() * (l - minFilter.Y()));
          for(int m = minFilter.X(); m<maxFilter.X(); ++m)
          {
            multiplier(*filterData,pixels + l * GetRowStride() + m * GetPixelSize(),value);
            //_CrtCheckMemory();
            //value[k] += *filterData * pixels[ (l * GetRowStride() + m)*GetPixelSize() + k];
            ++filterData;
          }
        } 
      }
    }

    if(m_StorageKind == Adopt)
    {
      eXl_FREE(m_ImageData);
    }
    m_StorageKind = Adopt;
    m_ImageData = newImage;
  }
}
