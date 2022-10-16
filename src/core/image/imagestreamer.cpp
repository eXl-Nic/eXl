/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/image/imagestreamer.hpp>
#include <core/image/image.hpp>

#include <core/log.hpp>

#include <fstream>

static void* reallocSized(void* oldPtr, size_t oldSize, size_t newSize)
{
  void* newAlloc = eXl_ALLOC(newSize);
  memcpy(newAlloc, oldPtr, oldSize);
  eXl_FREE(oldPtr);
  return newAlloc;
}


#define STB_IMAGE_IMPLEMENTATION

#define STBI_MALLOC(x) eXl_ALLOC(x)
#define STBI_REALLOC_SIZED(x,o,n) reallocSized(x,o,n)
#define STBI_FREE(x) eXl_FREE(x)

#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#define STBIW_MALLOC(x) eXl_ALLOC(x)
#define STBIW_REALLOC_SIZED(x,o,n) reallocSized(x,o,n)
#define STBIW_FREE(x) eXl_FREE(x)

#include <stb_image_write.h>

namespace eXl
{
  namespace
  {
    template <class T, unsigned int iNumCol>
    void SwapRB(Image::Size const& iSize, void* iData, unsigned int iRowStride)
    {
      unsigned char* dataPtr = reinterpret_cast<unsigned char*>(iData);
      for(unsigned int i = 0; i < iSize.y; ++i)
      {
        T* curRow = (T*)dataPtr;
        for(unsigned int j = 0; j<iSize.x; ++j)
        {
          T temp = curRow[2];
          curRow[2] = curRow[0];
          curRow[0] = temp;
          curRow += iNumCol;
        }
        dataPtr += iRowStride;
      }
    }
  }
#if 0
  struct FreeImageReadStruct
  {
    FreeImageReadStruct(uint8_t const* iBuffer, size_t iSize)
      : m_Buffer(iBuffer)
      , m_Size(iSize)
      , m_Offset(0)
    {}
    uint8_t const* m_Buffer;
    size_t         m_Size;
    long           m_Offset;
  };

  static unsigned DLL_CALLCONV Buffer_ReadProc (void *buffer, unsigned size, unsigned count, fi_handle handle)
  {
    
    FreeImageReadStruct* readStruct = (FreeImageReadStruct*)handle;
    
    size_t remainingBytes = readStruct->m_Size - readStruct->m_Offset;
    size_t availableCount = remainingBytes / size;
    size_t readCount = count > availableCount ? availableCount : count;
    size_t readSize = readCount * size;
    memcpy(buffer, readStruct->m_Buffer + readStruct->m_Offset, readSize);
    readStruct->m_Offset += readSize;
    return readCount;
  }
  /*
  unsigned DLL_CALLCONV DataSource_WriteProc (void *buffer, unsigned size, unsigned count, fi_handle handle)
  {
    
  }
  */
  static int DLL_CALLCONV Buffer_SeekProc (fi_handle handle, long offset, int origin)
  {
    FreeImageReadStruct* readStruct = (FreeImageReadStruct*)handle;
    switch(origin)
    {
    case SEEK_SET:
      readStruct->m_Offset = offset;
      break;
    case SEEK_CUR:
      readStruct->m_Offset += offset;
      break;
    case SEEK_END:
      readStruct->m_Offset = readStruct->m_Size;
      break;
    };

    if(readStruct->m_Offset >= readStruct->m_Size)
    {
      readStruct->m_Offset = 0;
      return -1;
    }
    return 0;
  }

  static long DLL_CALLCONV Buffer_TellProc (fi_handle handle)
  {
    FreeImageReadStruct* readStruct = (FreeImageReadStruct*)handle;
    return readStruct->m_Offset;
  }

  FIBITMAP* GetFIBitmap(AString const& iFileName)
  {
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(iFileName.c_str());
    if (FIF_UNKNOWN == fmt)
    {
      return nullptr;
    }
    FIBITMAP* bmp = FreeImage_Load(fmt,iFileName.c_str());
    return bmp;
  }

  FIBITMAP* GetFIBitmap(uint8_t const* iBuffer, size_t iSize)
  {
    if (iBuffer == nullptr)
    {
      LOG_ERROR << "LoadImageBuffer : Null buffer";
      return nullptr;
    }

    if (iSize == 0)
    {
      LOG_ERROR << "LoadImageBuffer : zero size";
      return nullptr;
    }

    FreeImageIO fimIO;
    FreeImageReadStruct readStruct(iBuffer, iSize);
    fimIO.read_proc = &Buffer_ReadProc;
    fimIO.seek_proc = &Buffer_SeekProc;
    fimIO.tell_proc = &Buffer_TellProc;
    fimIO.write_proc = nullptr;
        
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromHandle(&fimIO, &readStruct);
    if (FIF_UNKNOWN == fmt)
    {
      //String debugStr;
      //for (uint32_t i = 0; i < iSize <= 16 ? iSize : 16; ++i)
      //{
      //  debugStr.push_back(iBuffer[i]);
      //}
      LOG_ERROR << "Could not find image format, Size : " << iSize;
      return nullptr;
    }
    readStruct.m_Offset = 0;
    FIBITMAP* bmp = FreeImage_LoadFromHandle(fmt, &fimIO, &readStruct);
    
    if (bmp == nullptr)
    {
      //String debugStr;
      //for (uint32_t i = 0; i < iSize <= 16 ? iSize : 16; ++i)
      //{
      //  debugStr.push_back(iBuffer[i]);
      //}
      LOG_ERROR << "Could not load image, Size : " << iSize;
    }

    return bmp;
  }
#endif
  template <class T>
  T GetMaxVal();

  template <>
  inline unsigned char GetMaxVal<unsigned char>(){return UINT8_MAX;}

  template <>
  inline unsigned short GetMaxVal<unsigned short>(){return UINT16_MAX;}

  template <>
  inline unsigned int GetMaxVal<unsigned int>(){return UINT32_MAX;}

  template <class T>
  void CopyBlackData(Image::Size const& iSize, unsigned char* iTexCopy, unsigned char* iOrigData)
  {
    T* texCopyIter = (T*)iTexCopy;
    unsigned int rowStride = (iSize.x + (iSize.x % 4))*sizeof(T);
    for(unsigned int i = 0; i < iSize.y; ++i)
    {
      T* curRow = (T*)iOrigData;
      for(unsigned int j = 0; j<iSize.x; ++j)
      {
        texCopyIter[0] = curRow[0];
        texCopyIter[1] = curRow[0];
        texCopyIter[2] = curRow[0];
        texCopyIter += 3;
        ++curRow;
      }
      iOrigData += rowStride;
    }
  }

  template <class T>
  void CopyWhiteData(Image::Size const& iSize, unsigned char* iTexCopy, unsigned char* iOrigData)
  {
    T* texCopyIter = (T*)iTexCopy;
    unsigned int rowStride = (iSize.x + (iSize.x % 4))*sizeof(T);
    for(unsigned int i = 0; i < iSize.y; ++i)
    {
      T* curRow = (T*)iOrigData;
      for(unsigned int j = 0; j<iSize.x; ++j)
      {
        texCopyIter[0] = GetMaxVal<T>() - curRow[0];
        texCopyIter[1] = GetMaxVal<T>() - curRow[0];
        texCopyIter[2] = GetMaxVal<T>() - curRow[0];
        texCopyIter += 3;
        ++curRow;
      }
      iOrigData += rowStride;
    }
  }

#if 0

//#ifdef __ANDROID__
//#define PLATFORM_RGBFORMAT Image::RGB
//#define PLATFORM_RGBAFORMAT Image::RGBA
//#else
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_RGB
#define PLATFORM_RGBFORMAT Image::RGB
#define PLATFORM_RGBAFORMAT Image::RGBA
#else
#define PLATFORM_RGBFORMAT Image::BGR
#define PLATFORM_RGBAFORMAT Image::BGRA
#endif
//#endif

  Image* BuildImage(FIBITMAP* iBmp, Image::Storage iStorage)
  {
    if(iBmp != nullptr)
    {
      FREE_IMAGE_TYPE imType = FreeImage_GetImageType(iBmp);
      Image::Size imgSize (FreeImage_GetWidth(iBmp),FreeImage_GetHeight(iBmp));

      eXl_ASSERT_MSG(imType == FIT_BITMAP,"Unsupported file type");
      
      //Bottom-left origin in FreeType...
      FreeImage_FlipVertical(iBmp);

      FREE_IMAGE_COLOR_TYPE colType = FreeImage_GetColorType(iBmp);
      unsigned int bpp = FreeImage_GetBPP(iBmp);
      eXl_ASSERT_MSG(bpp % 8 == 0,"Exotic format");

      unsigned int bytesPP = bpp/8;
      size_t totSize = imgSize.x*imgSize.y*bytesPP;

      unsigned char* pixelsData = FreeImage_GetBits(iBmp);

      Image::Format format;
      Image::Components components;

      if(colType == FIC_PALETTE)
      {
        RGBQUAD bgColor;
        FreeImage_GetBackgroundColor(iBmp, &bgColor);
        RGBQUAD* palette = FreeImage_GetPalette(iBmp);
        bool isTransparent = FreeImage_IsTransparent(iBmp) == 0 ? false : true;

        format = Image::Char;

        int transparencyCount = 0;
        BYTE *transparencyTable = nullptr;

        if(isTransparent)
        {
          transparencyCount = FreeImage_GetTransparencyCount(iBmp);
          transparencyTable = FreeImage_GetTransparencyTable(iBmp);

          components = Image::RGBA;
        }
        else
        {
          components = Image::RGB;
        }

        unsigned int rowStride = FreeImage_GetPitch(iBmp);

        Image* newImage = eXl_NEW Image(nullptr, imgSize, components, format, 4, Image::Adopt);

        unsigned char* imageData = reinterpret_cast<unsigned char*>(newImage->GetImageData());
        for(unsigned int i = 0; i < imgSize.y; ++i)
        {
          unsigned char* rowData = imageData + i * newImage->GetRowStride();
          unsigned char* pixData = pixelsData + i * rowStride;
          for(unsigned int j = 0; j < imgSize.x; ++j)
          {
            rowData[0] = palette[*pixData].rgbRed;
            rowData[1] = palette[*pixData].rgbGreen;
            rowData[2] = palette[*pixData].rgbBlue;
            if(isTransparent)
            {
              if(*pixData < transparencyCount)
              {
                rowData[3] = transparencyTable[*pixData];
              }
              else
              {
                rowData[3] = 255;
              }
              rowData += 4;
            }
            else
            {
              rowData += 3;
            }
            ++pixData;
          }
        }

        return newImage;
      }

      unsigned int oldPixSize = 0;
      unsigned int newPixSize = 0;
      switch(colType)
      {
        case FIC_MINISWHITE :
        case FIC_MINISBLACK :

          components = Image::RGB;
          switch(bytesPP)
          {
          case 1:   format = Image::Char; break;
          case 2:   format = Image::Short; break;
          case 4:   format = Image::Int; break;
          default : LOG_WARNING << "Strange format for Luminance texture : "<< bytesPP << " bytes per component" << "\n"; break;
          }
          break;
        case FIC_RGB :
          components = PLATFORM_RGBFORMAT;
          switch(bytesPP)
          {
          case 3:
            format = Image::Char;  break;
          case 4:
            oldPixSize = 4;
            newPixSize = 3;
            format = Image::Char;  break;
          case 6:
            format = Image::Short; break;
          case 8:
            oldPixSize = 8;
            newPixSize = 6;
            format = Image::Short; break;
          case 12:
            format = Image::Int;   break;
          case 16:
            oldPixSize = 16;
            newPixSize = 12;
            format = Image::Int;   break;
          default : 
            LOG_WARNING << "Strange format for RGB texture : "<< float(bytesPP) / 3 << " bytes per component" << "\n"; 
            return nullptr;
            break;
          }
          break;
        case FIC_RGBALPHA:
          components = PLATFORM_RGBAFORMAT;

          switch(bytesPP)
          {
          case 4:   format = Image::Char;  break;
          case 8:   format = Image::Short; break;
          case 16:  format = Image::Int;   break;
          default : 
            LOG_WARNING << "Strange format for RGBA texture : "<< float(bytesPP) / 4 << " bytes per component" << "\n"; 
            return nullptr;
            break;
          }
          break;
      };

      if(colType == FIC_RGB || colType == FIC_RGBALPHA)
      {
        unsigned int rowStride = FreeImage_GetPitch(iBmp);
        if(newPixSize > 0)
        {
          Image* newImage = eXl_NEW Image(nullptr,imgSize,components,format,1,Image::Adopt);
          unsigned char* imageData = reinterpret_cast<unsigned char*>(newImage->GetImageData());
          for(unsigned int i = 0; i < imgSize.y; ++i)
          {
            unsigned char* rowData = imageData + i* newImage->GetRowStride();
            unsigned char* pixData = pixelsData + i * rowStride;
            for(unsigned int j = 0; j < imgSize.x; ++j)
            {
              memcpy(rowData,pixData,newPixSize);
              rowData += newPixSize;
              pixData += oldPixSize;
            }
          }
          return newImage;
        }
        else
        {
          Image* newImage = eXl_NEW Image(pixelsData,imgSize,components,format,4,iStorage);
          return newImage;
        }
      }
      else
      {
        Image* newImage = eXl_NEW Image(nullptr,imgSize,components,format,1,Image::Adopt);
        //texCopy = (unsigned char*) eXl_ALLOC(3*totSize);
        unsigned char* imageData = reinterpret_cast<unsigned char*>(newImage->GetImageData());
        switch(colType)
        {
        case FIC_MINISWHITE :
          switch(bytesPP)
          {
          case 1:
            CopyWhiteData<unsigned char>(imgSize,imageData,pixelsData);
            break;
          case 2:
            CopyWhiteData<unsigned short>(imgSize,imageData,pixelsData);
            break;
          case 4:
            CopyWhiteData<unsigned int>(imgSize,imageData,pixelsData);
            break;
          }
          break;
        case FIC_MINISBLACK :
          switch(bytesPP)
          {
          case 1:
            CopyBlackData<unsigned char>(imgSize,imageData,pixelsData);
            break;
          case 2:
            CopyBlackData<unsigned short>(imgSize,imageData,pixelsData);
            break;
          case 4:
            CopyBlackData<unsigned int>(imgSize,imageData,pixelsData);
            break;
          }
          break;
        }
        return newImage;
      }
    }
    return nullptr;
  }

  //void ImageStreamer::TransientLoad(String const& iPath, TransientLoader* iLoader) const
  //{
  //  FIBITMAP* bmp = GetFIBitmap(iPath,m_Sources);
  //  if(bmp != nullptr)
  //  {
  //    Image* newImage = BuildImage(bmp,Image::Reference);
  //
  //    iLoader->Load(newImage);
  //
  //    eXl_DELETE newImage;
  //    FreeImage_Unload(bmp);
  //  }
  //}
  FIBITMAP* ImageStreamer_Save(Image const* iImage)
  {
    if(iImage == nullptr)
      return nullptr;

    Image::Size imgSize = iImage->GetSize();
    FIBITMAP* bmp = FreeImage_Allocate(imgSize.x,imgSize.y,iImage->GetPixelSize() * 8);

    if(iImage->GetComponents() == Image::RGBA || iImage->GetComponents() == Image::BGRA)
    {
      FreeImage_SetTransparent(bmp,TRUE);
    }

    unsigned int oRowStride = FreeImage_GetPitch(bmp);//imgSize.x * iImage->GetPixelSize();
    //if(oRowStride % 16 != 0)
    //{
    //  oRowStride += 16 - (oRowStride % 16);
    //}
    unsigned char* oData = FreeImage_GetBits(bmp);
    unsigned char const* iData = reinterpret_cast<unsigned char const*>(iImage->GetImageData());
    for(unsigned int i = 0; i<imgSize.y; ++i)
    {
      memcpy(oData,iData,iImage->GetPixelSize() * imgSize.x);
      oData += oRowStride;
      iData += iImage->GetRowStride();
    }
    if(iImage->GetComponents() == Image::RGBA || iImage->GetComponents() == Image::BGRA)
    {
      if(iImage->GetComponents() != PLATFORM_RGBAFORMAT)
      {
        switch(iImage->GetFormat())
        {
        case Image::Char:
          SwapRB<unsigned char,4>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        case Image::Short:
          SwapRB<unsigned short,4>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        case Image::Int:
          SwapRB<unsigned int,4>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        default:
          LOG_ERROR<<"Incorrect format for image"<<"\n";
          break;
        }
      }
    }
    else if(iImage->GetComponents() == Image::RGB || iImage->GetComponents() == Image::BGR)
    {
      if(iImage->GetComponents() != PLATFORM_RGBFORMAT)
      {
        switch(iImage->GetFormat())
        {
        case Image::Char:
          SwapRB<unsigned char,3>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        case Image::Short:
          SwapRB<unsigned short,3>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        case Image::Int:
          SwapRB<unsigned int,3>(imgSize,FreeImage_GetBits(bmp),oRowStride);
          break;
        default:
          LOG_ERROR<<"Incorrect format for image"<<"\n";
          break;
        }
      }
    }
    return bmp;
  }
#endif

  namespace ImageStreamer
  {
    namespace
    {
      class StreamerInstance
      {
      public:
        StreamerInstance()
        {
        }

        Image* Load(AString const& iPath);
        Image* Load(uint8_t const* iBuffer, size_t iSize);

        void TransientLoad(AString const& iPath, ImageStreamer::TransientLoader* iLoader);

        void Save(Image const* iImage, AString const& iPath);

        void Save(Image const* iImage, Codec iCodec, size_t& oSize, void*& oBuffer);

        static StreamerInstance& Get()
        {
          static StreamerInstance s_Init;
          return s_Init;
        }
      };
    }

    Image* StreamerInstance::Load(AString const& iPath)
    {
      int x, y, comp;

      FILE* f = fopen(iPath.c_str(), "rb");
      if(f != nullptr)
      {
        uint8_t* data = stbi_load_from_file(f, &x, &y, &comp, 0);
        if (data != nullptr)
        {
          Image::Components imgComp;
          switch (comp)
          {
          case 1:
            imgComp = Image::R;
            break;
          case 2:
            imgComp = Image::RG;
            break;
          case 3:
            imgComp = Image::RGB;
            break;
          case 4:
            imgComp = Image::RGBA;
            break;
          }
          return eXl_NEW Image(data, Image::Size(x, y), imgComp, Image::Char, 1, Image::Adopt);
        }
        fclose(f);
        f = nullptr;
      }
      return nullptr;
    }

    Image* StreamerInstance::Load(uint8_t const* iBuffer, size_t iSize)
    {
      int x, y, comp;

      uint8_t* data = stbi_load_from_memory(iBuffer, iSize, &x, &y, &comp, 0);
      if (data != nullptr)
      {
        Image::Components imgComp;
        switch (comp) 
        {
        case 1:
          imgComp = Image::R;
          break;
        case 2:
          imgComp = Image::RG;
          break;
        case 3:
          imgComp = Image::RGB;
          break;
        case 4:
          imgComp = Image::RGBA;
          break;
        }

        return eXl_NEW Image(data, Image::Size(x, y), imgComp, Image::Char, 1, Image::Adopt);
      }
      return nullptr;
    }

    void StreamerInstance::Save(Image const* iImage, AString const& iPath)
    {
      size_t bufferSize;
      void* dataPtr = nullptr;
      Save(iImage, Png, bufferSize, dataPtr);
      {
        std::ofstream outFile(iPath, std::ios::binary);
        outFile.write((char const*)dataPtr, bufferSize);
      }
      eXl_FREE(dataPtr);
    }


    void StreamerInstance::Save(Image const* iImage, Codec iCodec, size_t& oSize, void*& oData)
    {
      oSize = 0;
      oData = nullptr;
      Image::Size size = iImage->GetSize();

      iImage->GetFormat();
      Image::Components comps = iImage->GetComponents();

      Image tmpImg(nullptr, Vec2u(0,0), Image::RGBA, Image::Char, 1);
      uint8_t const* dataBuffer = (uint8_t const*)iImage->GetImageData();
      uint32_t stride = iImage->GetRowStride();
      uint32_t numComps = 0;

      switch (comps) {
      case Image::R:
        numComps = 1;
        break;
      case Image::RG:
        numComps = 2;
        break;
      case Image::RGB:
        numComps = 3;
        break;
      case Image::BGR:
        numComps = 3;
        tmpImg = *iImage;

        SwapRB<char, 3>(size, tmpImg.GetImageData(), stride);
        dataBuffer = (uint8_t const*)tmpImg.GetImageData();
        break;
      case Image::RGBA:
        numComps = 4;
        break;
      case Image::BGRA:
        numComps = 4;
        tmpImg = *iImage;
        SwapRB<char, 4>(size, tmpImg.GetImageData(), stride);
        dataBuffer = (uint8_t const*)tmpImg.GetImageData();
        break;
      }

      switch (iCodec)
       {
       case Jpg:
         
         break;
       case Png:
         int len;
         oData = stbi_write_png_to_mem((uint8_t*)iImage->GetImageData(), iImage->GetRowStride(), size.x, size.y, iImage->GetComponents(), &len);
         break;
       case Bmp:
         
         break;
       default:
         return;
         break;
       }
      

      //FIBITMAP* bmp = ImageStreamer_Save(iImage);
      //if (bmp)
      //{
      //  FREE_IMAGE_FORMAT fmt;
      //  
      //
      //
      //  FIMEMORY* hmem = FreeImage_OpenMemory();
      //  FreeImage_SaveToMemory(fmt, bmp, hmem);
      //  FreeImage_Unload(bmp);
      //  long file_size = FreeImage_TellMemory(hmem);
      //
      //  unsigned char* data;
      //  DWORD size;
      //  if (FreeImage_AcquireMemory(hmem, &data, &size))
      //  {
      //    oSize = size;
      //    oData = malloc(size);
      //    memcpy(oData, data, size);
      //  }
      //
      //  FreeImage_CloseMemory(hmem);
      //}
    }

    Image* Load(AString const& iPath)
    {
      return StreamerInstance::Get().Load(iPath);
    }

    Image* Load(uint8_t const* iBuffer, size_t iSize)
    {
      return StreamerInstance::Get().Load(iBuffer, iSize);
    }

    void Save(Image const* iImage, AString const& iPath)
    {
      StreamerInstance::Get().Save(iImage, iPath);
    }

    void Save(Image const* iImage, Codec iCodec, size_t& oSize, void*& oBuffer)
    {
      StreamerInstance::Get().Save(iImage, iCodec, oSize, oBuffer);
    }
  }
}