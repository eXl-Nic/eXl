/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/name.hpp>
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

  MAKE_NAME(AttributeName);
  MAKE_NAME(UniformName);
  MAKE_NAME(TextureName);

  class EXL_OGL_API OGLSemanticManager
  {
  public:
    uint32_t RegisterAttribute(AttributeName iName, OGLType iAttribType, uint32_t iMult, uint32_t iDivisor = 0);
    uint32_t RegisterUniformData(UniformName iName, TupleType const* iDataType);
    uint32_t RegisterTexture(TextureName iTexName, OGLSamplerDesc const& iSampler);

    uint32_t GetSlotForName(AttributeName iName) const
    {
      auto iter = m_AttributeMap.find(iName);
      eXl_ASSERT_MSG(iter != m_AttributeMap.end(), eXl_FORMAT("Unknown Attribute %s", iName.get().c_str()));
      return iter->second;
    }

    uint32_t GetSlotForName(UniformName iName) const
    {
      auto iter = m_UniformMap.find(iName);
      eXl_ASSERT_MSG(iter != m_UniformMap.end(), eXl_FORMAT("Unknown Uniform %s", iName.get().c_str()));
      return iter->second;;
    }

    uint32_t GetSlotForName(TextureName iName) const
    {
      auto iter = m_TextureMap.find(iName);
      eXl_ASSERT_MSG(iter != m_TextureMap.end(), eXl_FORMAT("Unknown Texture %s", iName.get().c_str()));
      return iter->second;
    }

    OGLAttribDesc const& GetAttrib(uint32_t iName);

    AString const* GetDataName(uint32_t iName);
    TupleType const* GetDataType(uint32_t iName);

    OGLSamplerDesc const& GetSampler(uint32_t iName);

    uint32_t GetNumAttribs();
    uint32_t GetNumUniforms();
    uint32_t GetNumTextures();
  protected:
    struct OGLUniformData
    {
      AString m_Name;
      TupleType const* m_Type;
    };

    UnorderedMap<AttributeName, uint32_t> m_AttributeMap;
    UnorderedMap<UniformName, uint32_t>   m_UniformMap;
    UnorderedMap<TextureName, uint32_t>   m_TextureMap;

    Vector<OGLUniformData> m_Uniforms;
    Vector<OGLSamplerDesc> m_SamplerDesc;
    Vector<OGLAttribDesc>  m_Attribs;
  };
}

