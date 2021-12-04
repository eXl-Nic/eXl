/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
namespace eXl
{
  enum class OGLBufferUsage
  {
    ARRAY_BUFFER,
    ELEMENT_ARRAY_BUFFER,
    UNIFORM_BUFFER
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
    SAMPLER_2D,
    SAMPLER_CUBE
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
}
