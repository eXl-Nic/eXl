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
  namespace
  {
    struct OGLUniformData
    {
      AString m_Name;
      TupleType const* m_Type;
    };

    Vector<OGLUniformData> s_Uniforms;
    Vector<OGLSamplerDesc> s_SamplerDesc;
    Vector<OGLAttribDesc>  s_Attribs;
  }

  uint32_t OGLSemanticManager::RegisterAttribute(AString const& iName, OGLType iAttribType, uint32_t iMult)
  {
    eXl_ASSERT_MSG(iName.size() > 0, "Empty Name");

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

    uint32_t attribName = s_Attribs.size();
    s_Attribs.push_back(OGLAttribDesc());
    s_Attribs.back().m_Mult = iMult;
    s_Attribs.back().m_Name = iName;
    s_Attribs.back().m_Type = iAttribType;

    return attribName;
  }

  uint32_t OGLSemanticManager::RegisterUniformData(AString const& iName, TupleType const* iDataType)
  {
    eXl_ASSERT_MSG(iName.size() > 0, "Empty Name");
    eXl_ASSERT_MSG(iDataType != NULL,"");

    uint32_t uniformName = s_Uniforms.size();
    s_Uniforms.push_back(OGLUniformData());
    s_Uniforms.back().m_Name = iName;
    s_Uniforms.back().m_Type = iDataType;

    return uniformName;
  }

  OGLAttribDesc const& OGLSemanticManager::GetAttrib(uint32_t iName)
  {
    static OGLAttribDesc s_EmtpyAttrib;
    if(iName < s_Attribs.size())
    {
      return s_Attribs[iName];
    }
    return s_EmtpyAttrib;
  }

  AString const* OGLSemanticManager::GetDataName(uint32_t iName)
  {
    if (iName < s_Uniforms.size())
    {
      return &s_Uniforms[iName].m_Name;
    }
    return NULL;
  }

  TupleType const* OGLSemanticManager::GetDataType(uint32_t iName)
  {
    if(iName < s_Uniforms.size())
    {
      return s_Uniforms[iName].m_Type;
    }
    return NULL;
  }

  uint32_t OGLSemanticManager::RegisterTexture(AString const& iTexName, OGLSamplerDesc const& iSampler)
  {
    eXl_ASSERT_MSG(iTexName.size() > 0, "Empty Name");

    uint32_t textureName = s_SamplerDesc.size();
    s_SamplerDesc.push_back(OGLSamplerDesc());
    s_SamplerDesc.back().name = iTexName;
    s_SamplerDesc.back().maxFilter = iSampler.maxFilter;
    s_SamplerDesc.back().minFilter = iSampler.minFilter;
    s_SamplerDesc.back().wrapX = iSampler.wrapX;
    s_SamplerDesc.back().wrapY = iSampler.wrapY;

    return textureName;
  }

  OGLSamplerDesc const& OGLSemanticManager::GetSampler(uint32_t iName)
  {
    static OGLSamplerDesc defaultSampler;
    if(iName < s_SamplerDesc.size())
    {
      return s_SamplerDesc[iName];
    }
    return defaultSampler;
  }

  uint32_t OGLSemanticManager::GetNumAttribs()
  {
    return s_Attribs.size();
  }

  uint32_t OGLSemanticManager::GetNumUniforms()
  {
    return s_Uniforms.size();
  }

  uint32_t OGLSemanticManager::GetNumTextures()
  {
    return s_SamplerDesc.size();
  }
}