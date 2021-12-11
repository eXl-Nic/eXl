/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/string.hpp>
#include <ogl/renderer/ogltypes.hpp>
#include <ogl/oglexp.hpp>

namespace eXl
{
  class TupleType;

  struct OGLAttribDesc
  {
    AString m_Name;
    uint32_t m_Mult;
    OGLType m_Type;
    uint32_t m_Divisor;
  };

  struct OGLSamplerDesc
  {
    AString name;
    OGLTextureType samplerType;
    OGLMinFilter minFilter;
    OGLMagFilter maxFilter;
    OGLWrapMode wrapX;
    OGLWrapMode wrapY;
  };

  class EXL_OGL_API OGLSemanticManager
  {
  public:
    static uint32_t RegisterAttribute(AString const& iName, OGLType iAttribType, uint32_t iMult, uint32_t iDivisor = 0);

    static uint32_t RegisterUniformData(AString const& iName, TupleType const* iDataType);

    static uint32_t RegisterTexture(AString const& iTexName, OGLSamplerDesc const& iSampler);

    static OGLAttribDesc const& GetAttrib(uint32_t iName);

    static AString const* GetDataName(uint32_t iName);
    static TupleType const* GetDataType(uint32_t iName);

    static OGLSamplerDesc const& GetSampler(uint32_t iName);

    static uint32_t GetNumAttribs();

    static uint32_t GetNumUniforms();

    static uint32_t GetNumTextures();
  };
}

