/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <ogl/oglexp.hpp>

namespace eXl
{
  enum class OGLBufferUsage
  {
    ARRAY_BUFFER,
    ELEMENT_ARRAY_BUFFER,
    UNIFORM_BUFFER,
    TEXTURE_BUFFER
  };

  enum class OGLBufferAccess
  {
    READ,
    WRITE,
    READWRITE
  };

  enum class OGLType
  {
    FLOAT32,
    FLOAT32_2,
    FLOAT32_3,
    FLOAT32_4,
    INT32,
    INT32_2,
    INT32_3,
    INT32_4,
    MAT3,
    MAT4,
    SAMPLER_1D,
    SAMPLER_1D_ARRAY,
    SAMPLER_2D,
    SAMPLER_2D_ARRAY,
    SAMPLER_3D,
    SAMPLER_CUBE,
    SAMPLER_BUFFER,
    INT_SAMPLER_1D,
    INT_SAMPLER_1D_ARRAY,
    INT_SAMPLER_2D,
    INT_SAMPLER_2D_ARRAY,
    INT_SAMPLER_3D,
    INT_SAMPLER_CUBE,
    INT_SAMPLER_BUFFER,
  };

  enum class OGLConnectivity
  {
    POINT,
    LINELIST,
    LINESTRIP,
    TRIANGLELIST,
    TRIANGLESTRIP
  };
  enum class OGLMinFilter
  {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_NEAREST,
    LINEAR_MIPMAP_LINEAR,
  };

  enum class OGLMagFilter
  {
    NEAREST,
    LINEAR
  };

  enum class OGLWrapMode
  {
    CLAMP_TO_EDGE,
    REPEAT,
    MIRRORED_REPEAT
  };

  enum class OGLBlend
  {
    ZERO, 
    ONE, 
    SRC_COLOR, 
    ONE_MINUS_SRC_COLOR, 
    DST_COLOR, 
    ONE_MINUS_DST_COLOR, 
    SRC_ALPHA, 
    ONE_MINUS_SRC_ALPHA, 
    DST_ALPHA, 
    ONE_MINUS_DST_ALPHA, 
    CONSTANT_COLOR, 
    ONE_MINUS_CONSTANT_COLOR, 
    CONSTANT_ALPHA, 
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE, 
    SRC1_COLOR, 
    ONE_MINUS_SRC1_COLOR, 
    SRC1_ALPHA, 
    ONE_MINUS_SRC1_ALPHA
  };

  enum class OGLTextureType
  {
#ifndef __ANDROID__
    TEXTURE_1D,
    TEXTURE_1D_ARRAY,
#endif
    TEXTURE_2D,
    TEXTURE_2D_ARRAY,
    TEXTURE_3D,
    TEXTURE_CUBE_MAP,
    TEXTURE_BUFFER
  };

  enum class OGLInternalTextureFormat
  {
    DEPTH_COMPONENT,
    DEPTH_STENCIL,
    RED,
    RG,
    RGB,
    RGBA,

    R8,
    RG8,
    RGB8,
    RGBA8,

    R32F,
    RG32F,
    RGB32F,
    RGBA32F,

    R32I,
    RG32I,
    RGB32I,
    RGBA32I,
#if 0
    R8_SNORM,
    R16F,
    R8UI,
    R8I,
    R16UI,
    R16I,
    R32UI,
    RG8_SNORM,
    RG16F,
    RG8UI,
    RG8I,
    RG16UI,
    RG16I,
    RG32UI,
    SRGB8,
    RGB565,
    RGB8_SNORM,
    RGB16F,
    RGB8UI,
    RGB8I,
    RGB16UI,
    RGB16I,
    RGB32UI,
    SRGB8_ALPHA8,
    RGBA8_SNORM,
    RGB5_A1,
    RGBA4,
    RGB10_A2,
    RGBA16F,
    RGBA8UI,
    RGBA8I,
    RGB10_A2UI,
    RGBA16UI,
    RGBA16I,
    RGBA32UI,
    
    DEPTH_COMPONENT16,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT32F,
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8
#endif
  };

  enum class OGLTextureFormat
  {
    RED,
    RG,
    RGB,
    RGBA,

    BGR,
    BGRA,

    RED_INTEGER,
    RG_INTEGER,
    RGB_INTEGER,
    BGR_INTEGER,
    RGBA_INTEGER,
    BGRA_INTEGER,
    STENCIL_INDEX,
    DEPTH_COMPONENT,
    DEPTH_STENCIL
  };

  enum class OGLTextureElementType
  {
    UNSIGNED_BYTE,
    BYTE,
    UNSIGNED_SHORT,
    SHORT,
    UNSIGNED_INT,
    INT,
    HALF_FLOAT,
    FLOAT,
#ifndef __ANDROID__
    UNSIGNED_SHORT_5_6_5,
    UNSIGNED_SHORT_5_6_5_REV,
#endif
    UNSIGNED_SHORT_4_4_4_4,
    UNSIGNED_SHORT_5_5_5_1,
    UNSIGNED_INT_2_10_10_10_REV
  };

  EXL_OGL_API bool IsSampler(OGLType iType);
  EXL_OGL_API OGLTextureType GetSamplerTextureType(OGLType iType);
}
