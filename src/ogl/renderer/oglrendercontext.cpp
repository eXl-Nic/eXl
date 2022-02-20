/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglframebuffer.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltypesconv.hpp>
#include <core/type/tupletype.hpp>

namespace eXl
{
#ifdef EXL_WITH_OGL
  class OGLRenderContextImpl : public HeapObject
  {
  public:

    OGLRenderContextImpl(OGLSemanticManager& iSemantics)
    {
      m_CurrentDataSlot.resize(iSemantics.GetNumUniforms(), -1);
      m_CurrentAttribSlot.resize(iSemantics.GetNumAttribs(), -1);
      m_CurrentTextureSlot.resize(iSemantics.GetNumTextures(), -1);
      m_CurrentUBOSlot.resize(iSemantics.GetNumUniforms(), -1);

      m_BufferBindings.resize(iSemantics.GetNumAttribs());

      m_TextureCache.resize(iSemantics.GetNumTextures(), nullptr);

      //for(uint32_t i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  m_DataCache.push_back(OGLSemanticManager::GetData(i)->Build());
      //}
      m_DataCache.resize(iSemantics.GetNumUniforms(), nullptr);
      m_UBOCache.resize(iSemantics.GetNumUniforms(), nullptr);

      Clear();
    }

    ~OGLRenderContextImpl()
    {
      //for(uint32_t i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  OGLSemanticManager::GetData(i)->Destroy(m_DataCache[i]);
      //}
    }

    void Clear()
    {
      m_CurrentProgram = nullptr;
      m_CurrentArrayBuffer = nullptr;
      m_CurrentIndexBuffer = nullptr;
      m_MaxAttrib = 0;
      m_MaxData = 0;
      m_MaxUBO = 0;
      m_MaxTexture = 0;
      for (uint32_t i = 0; i < m_BufferBindings.size(); ++i)
      {
        m_BufferBindings[i].buffer = nullptr;
      }
      for (uint32_t i = 0; i < m_TextureCache.size(); ++i)
      {
        m_TextureCache[i] = nullptr;
      }
      //for(uint32_t i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  Type const* type = OGLSemanticManager::GetData(i);
      //  type->Destruct(m_DataCache[i]);
      //  type->Construct(m_DataCache[i]);
      //}
      m_CurFramebuffer = nullptr;
    }

    inline void SetFramebuffer(OGLFramebuffer* iFBO)
    {
      if (iFBO && iFBO != m_CurFramebuffer)
      {
        glBindFramebuffer(GL_FRAMEBUFFER, iFBO->GetId());
        eXl_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
      }
      else if (iFBO == nullptr && m_CurFramebuffer != nullptr)
      {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      }
      m_CurFramebuffer = iFBO;
    }

    void CleanupState()
    {
      if (m_CurFramebuffer)
      {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
      }

      for (uint32_t i = 0; i < 16; ++i)
      {
        if ((m_SetAttribs & 1 << i) != 0)
        {
          glDisableVertexAttribArray(i);
        }
      }

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      for (uint32_t i = 0; i < 16; ++i)
      {
        glBindBufferBase(GL_UNIFORM_BUFFER, i, 0);
      }

      for (uint32_t i = 0; i < m_MaxTexture; ++i)
      {

      }

      glUseProgram(0);

      Clear();
    }

    inline void SetProgram(OGLCompiledProgram const* iProgram)
    {
      if (iProgram == m_CurrentProgram)
      {
        return;
      }

      if (iProgram == nullptr)
      {
        Clear();
        return;
      }

      m_CurrentProgram = iProgram;
      iProgram->Setup();

      uint32_t oldAttribCount = m_MaxAttrib;
      uint32_t oldDataCount = m_MaxData;

      std::fill(m_CurrentAttribSlot.begin(), m_CurrentAttribSlot.end(), -1);
      std::fill(m_CurrentDataSlot.begin(), m_CurrentDataSlot.end(), -1);
      std::fill(m_CurrentUBOSlot.begin(), m_CurrentUBOSlot.end(), -1);
      std::fill(m_CurrentTextureSlot.begin(), m_CurrentTextureSlot.end(), -1);

      uint32_t const* slotPtr = iProgram->GetAttribSlots();
      uint32_t enabledAttribs = 0;
      for (uint32_t i = 0; i < iProgram->GetMaxAttrib(); ++i)
      {
        m_CurrentAttribSlot[slotPtr[i]] = i;
        m_BufferBindings[i].buffer = nullptr;
        uint32_t attribLoc = iProgram->GetAttribLocation(i);
        //glEnableVertexAttribArray(attribLoc);
        enabledAttribs |= 1 << attribLoc;
      }

      for (uint32_t i = 0; i < 16; ++i)
      {
        if ((m_SetAttribs & 1 << i) != 0)
        {
          glDisableVertexAttribArray(i);
        }
      }
      m_MaxAttrib = iProgram->GetMaxAttrib();
      m_NeededAttribs = enabledAttribs;
      m_SetAttribs = 0;
      m_AttribFlags = (1 << m_MaxAttrib) - 1;

      slotPtr = iProgram->GetUniformSlots();
      for (uint32_t i = 0; i < iProgram->GetMaxUniform(); ++i)
      {
        m_CurrentDataSlot[slotPtr[i]] = i;
        //m_CurrentData[i] = m_DataCache[slotPtr[i]];
        m_CurrentData[i] = &m_DataCache[slotPtr[i]];
      }
      m_MaxData = iProgram->GetMaxUniform();
      m_DataFlags = (1 << m_MaxData) - 1;

      slotPtr = iProgram->GetUniformBlockSlots();
      for (uint32_t i = 0; i < iProgram->GetMaxUniformBlocks(); ++i)
      {
        m_CurrentUBOSlot[slotPtr[i]] = i;
        m_CurrentUBO[i] = &m_UBOCache[slotPtr[i]];
      }
      m_MaxUBO = iProgram->GetMaxUniformBlocks();
      m_UBOFlags = (1 << m_MaxUBO) - 1;

      slotPtr = iProgram->GetTextureSlots();
      for (uint32_t i = 0; i < iProgram->GetMaxTexture(); ++i)
      {
        m_CurrentTextureSlot[slotPtr[i]] = i;
        m_CurrentTexture[i] = &m_TextureCache[slotPtr[i]];
      }
      m_MaxTexture = iProgram->GetMaxTexture();
      m_TexFlags = (1 << m_MaxTexture) - 1;

      glUseProgram(iProgram->GetProgram()->GetProgName());
    }

    inline void SetVertexAttrib(uint32_t iAttribName, OGLBuffer const* iBuffer, uint32_t iNum, size_t iStride, size_t iOffset)
    {
      int currentSlot;
      if (iAttribName < m_CurrentAttribSlot.size() && (currentSlot = m_CurrentAttribSlot[iAttribName]) != -1)
      {
        if (iBuffer == nullptr || iBuffer->GetBufferUsage() != OGLBufferUsage::ARRAY_BUFFER)
        {
          if (iBuffer->GetBufferUsage() != OGLBufferUsage::ARRAY_BUFFER)
          {
            LOG_WARNING << "Invalid buffer for attrib" << "\n";
          }
          m_BufferBindings[iAttribName].buffer = nullptr;

        }
        else
        {
          m_BufferBindings[iAttribName].attribName = iAttribName;
          m_BufferBindings[iAttribName].buffer = iBuffer;
          m_BufferBindings[iAttribName].num = iNum;
          m_BufferBindings[iAttribName].stride = iStride;
          m_BufferBindings[iAttribName].offset = iOffset;
        }

        m_AttribFlags |= 1 << currentSlot;
      }
    }

    inline void SetUniformData(uint32_t iDataName, void const* iData)
    {
      //OGLSemanticManager::GetData(iDataName)->Copy(iData, m_DataCache[iDataName]);
      m_DataCache[iDataName] = iData;

      int currentSlot;
      if (iDataName < m_CurrentDataSlot.size() && (currentSlot = m_CurrentDataSlot[iDataName]) != -1)
      {
        m_DataFlags |= 1 << currentSlot;
      }
    }

    inline void SetUniformBuffer(uint32_t iDataName, OGLBuffer const* iBuffer)
    {
      m_UBOCache[iDataName] = iBuffer;

      int currentSlot;
      if (iDataName < m_CurrentUBOSlot.size() && (currentSlot = m_CurrentUBOSlot[iDataName]) != -1)
      {
        m_UBOFlags |= 1 << currentSlot;
      }
    }

    inline void SetTexture(uint32_t iTexName, OGLTexture const* iTex)
    {
      m_TextureCache[iTexName] = iTex;

      int currentSlot;
      if (iTexName < m_CurrentTextureSlot.size() && (currentSlot = m_CurrentTextureSlot[iTexName]) != -1)
      {
        m_TexFlags |= 1 << currentSlot;
      }
    }

    inline void CheckCache()
    {
      if ((m_AttribFlags | m_DataFlags | m_TexFlags | m_UBOFlags) != 0)
      {
        CommitCache();
      }
    }
    struct BitIter
    {
      BitIter(uint32_t& iFlags)
        : m_Flags(iFlags)
      {
        GetNext();
      }

      inline explicit operator bool()
      {
        return m_Cur.has_value();
      }

      inline BitIter& operator++()
      {
        GetNext();
        return *this;
      }

      inline uint32_t operator*() const
      {
        return *m_Cur;
      }

      protected:
      uint32_t& m_Flags;
      Optional<uint32_t> m_Cur;
      inline void GetNext()
      {
        if (m_Flags == 0)
        {
          m_Cur.reset();
          return;
        }
#ifndef __ANDROID__
        m_Cur = _tzcnt_u32(m_Flags);
#else
        m_Cur = 31 - __builtin_clz(m_Flags);
#endif
        m_Flags &= ~(1 << *m_Cur);
      }
    };
    void CommitCache()
    {
      for(BitIter attribs(m_AttribFlags); attribs; ++attribs)
      {
        uint32_t attrib = *attribs;
        int slot = m_CurrentAttribSlot[attrib];
        eXl_ASSERT(slot >= 0);
        BufferBinding& binding = m_BufferBindings[attrib];
        if(binding.buffer != nullptr)
        {
          if((m_SetAttribs & 1<<attrib) == 0)
          {
            glEnableVertexAttribArray(attrib);
            m_SetAttribs |= 1<< attrib;
          }
          if(binding.buffer != m_CurrentArrayBuffer)
          {
            glBindBuffer(GL_ARRAY_BUFFER,binding.buffer->GetBufferId());
            m_CurrentArrayBuffer = binding.buffer;
          }
          m_CurrentProgram->HandleAttribute(attrib, binding.num, binding.stride, binding.offset);
        }
        else
        {
          if(m_SetAttribs & 1<< attrib)
          {
            glDisableVertexAttribArray(attrib);
            m_SetAttribs &= (~1u)<< attrib;
          }
        }
      }

      for (BitIter dataSlots(m_DataFlags); dataSlots; ++dataSlots)
      {
        uint32_t dataSlot = *dataSlots;
        
        if(void const* curData = *m_CurrentData[dataSlot])
        {
          m_CurrentProgram->HandleUniform(dataSlot, curData);
        }
      }

      for (BitIter uboSlots(m_UBOFlags); uboSlots; ++uboSlots)
      {
        uint32_t dataSlot = *uboSlots;
        if (OGLBuffer const* curBuffer = *m_CurrentUBO[dataSlot])
        {
          m_CurrentProgram->HandleUniformBlock(dataSlot, curBuffer->GetBufferId());
        }
      }

      for (BitIter texSlots(m_TexFlags); texSlots; ++texSlots)
      {
        uint32_t texSlot = *texSlots;
        if (OGLTexture const* curTexture = *m_CurrentTexture[texSlot])
        {
          m_CurrentProgram->HandleTexture(texSlot, curTexture);
        }
      }
    }

    OGLCompiledProgram const* m_CurrentProgram;

    OGLBuffer const* m_CurrentArrayBuffer;
    OGLBuffer const* m_CurrentIndexBuffer;

    struct BufferBinding
    {
      OGLBuffer const* buffer;
      uint32_t attribName;
      uint32_t num;
      size_t stride;
      size_t offset;
    };

    Vector<int> m_CurrentAttribSlot;
    Vector<int> m_CurrentDataSlot;
    Vector<int> m_CurrentUBOSlot;
    Vector<int> m_CurrentTextureSlot;
    
    Vector<BufferBinding>       m_BufferBindings;
    //std::vector<void*>               m_DataCache;
    Vector<void const*>         m_DataCache;
    Vector<OGLBuffer const*>    m_UBOCache;
    Vector<OGLTexture const*>   m_TextureCache;
    
    OGLTexture const**   m_CurrentTexture[8];
    void const**         m_CurrentData[16];
    OGLBuffer const**    m_CurrentUBO[16];

    OGLFramebuffer*      m_CurFramebuffer;
    
    //Type const* m_DataType[16];
    //void const* m_Data[16];

    uint32_t m_MaxData = 0;
    uint32_t m_MaxUBO = 0;
    uint32_t m_MaxAttrib = 0;
    uint32_t m_MaxTexture = 0;

    uint32_t m_AttribFlags = 0;
    uint32_t m_NeededAttribs = 0;
    uint32_t m_SetAttribs = 0;
    uint32_t m_DataFlags = 0;
    uint32_t m_UBOFlags = 0;
    uint32_t m_TexFlags = 0;

    //std::vector<Type const*> m_Types;
  };
#endif

  OGLRenderContext::OGLRenderContext(OGLSemanticManager& iSemantics)
#ifdef EXL_WITH_OGL
    : m_Impl(eXl_NEW OGLRenderContextImpl(iSemantics))
#endif
  {
  }

  void OGLRenderContext::SetFramebuffer(OGLFramebuffer* iFBO)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetFramebuffer(iFBO);
#endif
  }

  OGLRenderContext::~OGLRenderContext()
  {
#ifdef EXL_WITH_OGL
    eXl_DELETE m_Impl;
#endif
  }

  void OGLRenderContext::Clear()
  {
#ifdef EXL_WITH_OGL
    m_Impl->CleanupState();
#endif
  }

  void OGLRenderContext::SetProgram(OGLCompiledProgram const* iProgram)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetProgram(iProgram);
#endif
  }

  void OGLRenderContext::SetVertexAttrib(uint32_t iAttribName, OGLBuffer const* iBuffer, uint32_t iNum, size_t iStride, size_t iOffset)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetVertexAttrib(iAttribName, iBuffer, iNum, iStride, iOffset);
#endif
  }

  void OGLRenderContext::SetUniformData(uint32_t iDataName, void const* iData)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetUniformData(iDataName,iData);
#endif
  }

  void OGLRenderContext::SetUniformBuffer(uint32_t iDataName, OGLBuffer const* iBuffer)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetUniformBuffer(iDataName, iBuffer);
#endif
  }

  void OGLRenderContext::SetTexture(uint32_t iTexName, OGLTexture const* iTex)
  {
#ifdef EXL_WITH_OGL
    m_Impl->SetTexture(iTexName,iTex);
#endif
  }

  void OGLRenderContext::Draw(OGLConnectivity iTopo, uint32_t iFirstVertex, uint32_t iNumVertices)
  {
#ifdef EXL_WITH_OGL
    m_Impl->CheckCache();
    if(m_Impl->m_CurrentIndexBuffer != nullptr)
    {
      m_Impl->m_CurrentIndexBuffer = nullptr;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    }

    glDrawArrays(GetGLConnectivity(iTopo), iFirstVertex, iNumVertices);
#endif
  }

  void OGLRenderContext::DrawIndexed(OGLBuffer const* iBuffer,OGLConnectivity iTopo, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumIndices)
  {
#ifdef EXL_WITH_OGL
    if(iBuffer == nullptr || iBuffer->GetBufferUsage() != OGLBufferUsage::ELEMENT_ARRAY_BUFFER)
    {
      LOG_WARNING<<"Invalid buffer for DrawIndexed"<<"\n";
      return;
    }
    m_Impl->CheckCache();
    if(m_Impl->m_CurrentIndexBuffer != iBuffer)
    {
      m_Impl->m_CurrentIndexBuffer = iBuffer;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,iBuffer->GetBufferId());
    }

    glDrawElementsBaseVertex(GetGLConnectivity(iTopo), iNumIndices, GL_UNSIGNED_INT, ((uint8_t*)0) + iOffset, iBaseVertex);
#endif
  }
#ifndef __ANDROID__
  void OGLRenderContext::DrawInstanced(OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iBaseInstance, uint32_t iFirstVertex, uint32_t iNumVertices)
#else
  void OGLRenderContext::DrawInstanced(OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iFirstVertex, uint32_t iNumVertices)
#endif
  {
#ifdef EXL_WITH_OGL
    m_Impl->CheckCache();
    if (m_Impl->m_CurrentIndexBuffer != nullptr)
    {
      m_Impl->m_CurrentIndexBuffer = nullptr;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
#ifndef __ANDROID__
    glDrawArraysInstancedBaseInstance(GetGLConnectivity(iTopo), iFirstVertex, iNumVertices, iNumInstances, iBaseInstance);
#else
    glDrawArraysInstanced(GetGLConnectivity(iTopo), iFirstVertex, iNumVertices, iNumInstances);
#endif
#endif
  }

#ifndef __ANDROID__
  void OGLRenderContext::DrawIndexedInstanced(OGLBuffer const* iBuffer, OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iBaseinstance, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumIndices)
#else
  void OGLRenderContext::DrawIndexedInstanced(OGLBuffer const* iBuffer, OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumIndices)
#endif
  {
#ifdef EXL_WITH_OGL
    if (iBuffer == nullptr || iBuffer->GetBufferUsage() != OGLBufferUsage::ELEMENT_ARRAY_BUFFER)
    {
      LOG_WARNING << "Invalid buffer for DrawIndexed" << "\n";
      return;
    }
    m_Impl->CheckCache();
    if (m_Impl->m_CurrentIndexBuffer != iBuffer)
    {
      m_Impl->m_CurrentIndexBuffer = iBuffer;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBuffer->GetBufferId());
    }
#ifndef __ANDROID__
    glDrawElementsInstancedBaseVertexBaseInstance(GetGLConnectivity(iTopo), iNumIndices, GL_UNSIGNED_INT, ((uint8_t*)0) + iOffset, iNumInstances, iBaseVertex, iBaseinstance);
#else
    glDrawElementsInstancedBaseVertex(GetGLConnectivity(iTopo), iNumIndices, GL_UNSIGNED_INT, ((uint8_t*)0) + iOffset, iNumInstances, iBaseVertex);
#endif
#endif
  }
}
