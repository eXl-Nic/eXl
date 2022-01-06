/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <cstdint>
#include <ogl/renderer/ogltypes.hpp>
#include <ogl/renderer/oglinclude.hpp>

namespace eXl
{
  uint32_t GetGLUsage(OGLBufferUsage iUsage);
  uint32_t GetGLAccess(OGLBufferAccess iAccess);
  uint32_t GetGLType(OGLType iType);
  OGLType ConvertGLType(uint32_t iType);
  uint32_t GetGLConnectivity(OGLConnectivity iConn);
  uint32_t GetGLMinFilter(OGLMinFilter iMinFilter);
  uint32_t GetGLMagFilter(OGLMagFilter iMagFilter);
  uint32_t GetGLWrapMode(OGLWrapMode iMode);
  uint32_t GetGLBlend(OGLBlend iMode);
  uint32_t GetGLTextureType(OGLTextureType iTex);
  uint32_t GetGLInternalTextureFormat(OGLInternalTextureFormat iFOrmat);
  uint32_t GetGLTextureElementType(OGLTextureElementType iType);
  uint32_t GetGLTextureFormat(OGLTextureFormat iTex);
}
