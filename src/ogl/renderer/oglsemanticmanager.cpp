/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <vector>
#include <core/coredef.hpp>

namespace eXl
{
  uint32_t OGLSemanticManager::RegisterAttribute(AttributeName iName, OGLType iAttribType, uint32_t iMult, uint32_t iDivisor)
  {
    uint32_t attribSlot = m_Attribs.size();

    auto insertRes = m_AttributeMap.insert(std::make_pair(iName, attribSlot));
    if (!insertRes.second)
    {
      return insertRes.first->second;
    }

    switch(iAttribType)
    {
    case OGLType::FLOAT32:
    case OGLType::INT32:
      break;
    default:
      eXl_ASSERT_MSG(false, "Incorrect attrib");
      break;
    };

    eXl_ASSERT_MSG(iMult >= 1 || iMult <= 4, "Wrong multiplicity");

    m_Attribs.push_back(OGLAttribDesc());
    m_Attribs.back().m_Mult = iMult;
    m_Attribs.back().m_Name = iName.get();
    m_Attribs.back().m_Type = iAttribType;
    m_Attribs.back().m_Divisor = iDivisor;

    return attribSlot;
  }

  uint32_t OGLSemanticManager::RegisterUniformData(UniformName iName, TupleType const* iDataType)
  {
    uint32_t uniformSlot = m_Uniforms.size();

    auto insertRes = m_UniformMap.insert(std::make_pair(iName, uniformSlot));
    if (!insertRes.second)
    {
      return insertRes.first->second;
    }

    eXl_ASSERT_MSG(iDataType != NULL,"");

    m_Uniforms.push_back(OGLUniformData());
    m_Uniforms.back().m_Name = iName.get();
    m_Uniforms.back().m_Type = iDataType;

    return uniformSlot;
  }

  OGLAttribDesc const& OGLSemanticManager::GetAttrib(uint32_t iName)
  {
    static OGLAttribDesc s_EmtpyAttrib;
    if(iName < m_Attribs.size())
    {
      return m_Attribs[iName];
    }
    return s_EmtpyAttrib;
  }

  AString const* OGLSemanticManager::GetDataName(uint32_t iName)
  {
    if (iName < m_Uniforms.size())
    {
      return &m_Uniforms[iName].m_Name;
    }
    return nullptr;
  }

  TupleType const* OGLSemanticManager::GetDataType(uint32_t iName)
  {
    if(iName < m_Uniforms.size())
    {
      return m_Uniforms[iName].m_Type;
    }
    return NULL;
  }

  uint32_t OGLSemanticManager::RegisterTexture(TextureName iTexName, OGLSamplerDesc const& iSampler)
  {
    uint32_t textureSlot = m_SamplerDesc.size();

    auto insertRes = m_TextureMap.insert(std::make_pair(iTexName, textureSlot));
    if (!insertRes.second)
    {
      return insertRes.first->second;
    }

    m_SamplerDesc.push_back(iSampler);
    m_SamplerDesc.back().name = iTexName.get();

    return textureSlot;
  }

  OGLSamplerDesc const& OGLSemanticManager::GetSampler(uint32_t iName)
  {
    static OGLSamplerDesc defaultSampler;
    if(iName < m_SamplerDesc.size())
    {
      return m_SamplerDesc[iName];
    }
    return defaultSampler;
  }

  uint32_t OGLSemanticManager::GetNumAttribs()
  {
    return m_Attribs.size();
  }

  uint32_t OGLSemanticManager::GetNumUniforms()
  {
    return m_Uniforms.size();
  }

  uint32_t OGLSemanticManager::GetNumTextures()
  {
    return m_SamplerDesc.size();
  }
}