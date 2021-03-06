/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/image/image.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/ogltexture.hpp>

namespace eXl
{
  class OGLTexture;
  class Image;

  class EXL_OGL_API OGLTextureLoader
  {
  public:

    enum Format
    {
      R8,
      RG8,
      RGB8,
      RGBA8,
      R32F,
      RG32F,
      RGB32F,
      RGBA32F
    };

    static OGLTexture* Create(Image::Size const& iSize, Format iFormat);

    static OGLTexture* CreateFromBuffer(Image::Size const& iSize, Format iFormat, void const* iData, bool iGenMipMap);

    static OGLTexture* CreateFromImage(Image const* iImage, bool iGenMipMap);

    static void SetFromImage(Image const* iImage, OGLTexture* iTexture, uint32_t iFace = -1);

    static OGLTexture* CreateCubeMap(Image const* const* iImage, bool iGenMipMap);

    static Err ReadTexture(OGLTexture* iTexture, Image*& oImage, int iFace = -1);
  
  };
}