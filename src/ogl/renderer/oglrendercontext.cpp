/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogltechnique.hpp>
#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglframebuffer.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltypesconv.hpp>
#include <core/type/tupletype.hpp>


namespace eXl
{
  class OGLRenderContextImpl : public HeapObject
  {
  public:

    OGLRenderContextImpl()
    {
      m_CurrentDataSlot.resize(OGLSemanticManager::GetNumUniforms(),-1);
      m_CurrentAttribSlot.resize(OGLSemanticManager::GetNumAttribs(),-1);
      m_CurrentTextureSlot.resize(OGLSemanticManager::GetNumTextures(),-1);
      //m_BackupDataSlot.resize(OGLSemanticManager::GetNumUniforms(),-1);
      //m_BackupAttribSlot.resize(OGLSemanticManager::GetNumAttribs(),-1);

      m_BufferBindings.resize(OGLSemanticManager::GetNumAttribs());

      m_TextureCache.resize(OGLSemanticManager::GetNumTextures(),NULL);

      //for(unsigned int i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  m_DataCache.push_back(OGLSemanticManager::GetData(i)->Build());
      //}
      m_DataCache.resize(OGLSemanticManager::GetNumUniforms(),NULL);

      Clear();
    }

    ~OGLRenderContextImpl()
    {
      //for(unsigned int i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  OGLSemanticManager::GetData(i)->Destroy(m_DataCache[i]);
      //}
    }

    void Clear()
    {
      m_CurrentTechnique = NULL;
      m_CurrentArrayBuffer = NULL;
      m_CurrentIndexBuffer = NULL;
      m_MaxAttrib = 0;
      m_MaxData = 0;
      m_MaxTexture = 0;
      for(unsigned int i = 0; i < m_BufferBindings.size() ; ++i)
      {
        m_BufferBindings[i].buffer = NULL;
      }
      for(unsigned int i = 0; i < m_TextureCache.size(); ++i)
      {
        m_TextureCache[i] = NULL;
      }
      //for(unsigned int i = 0; i<OGLSemanticManager::GetNumUniforms();++i)
      //{
      //  Type const* type = OGLSemanticManager::GetData(i);
      //  type->Destruct(m_DataCache[i]);
      //  type->Construct(m_DataCache[i]);
      //}
      m_CurFramebuffer = NULL;
    }

    inline void SetFramebuffer(OGLFramebuffer* iFBO)
    {
      if(iFBO && iFBO != m_CurFramebuffer)
      {
        glBindFramebuffer(GL_FRAMEBUFFER, iFBO->GetId());
        eXl_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
      }
      else if(iFBO == NULL && m_CurFramebuffer != NULL)
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

      for (unsigned int i = 0; i < 16; ++i)
      {
        if ((m_SetAttribs & 1 << i) != 0)
        {
          glDisableVertexAttribArray(i);
        }
      }

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

      for (unsigned int i = 0; i < m_MaxTexture; ++i)
      {

      }

      glUseProgram(0);

      Clear();
    }

    inline void SetTechnique(OGLCompiledTechnique const* iTechnique)
    {
      //m_BackupAttribSlot.swap(m_CurrentAttribSlot);
      //m_BackupDataSlot.swap(m_CurrentDataSlot);

      if(iTechnique == m_CurrentTechnique)
        return;

      if(iTechnique == NULL)
      {
        Clear();
        return;
      }

      m_CurrentTechnique = iTechnique;
      iTechnique->Setup();

      //Mettre tout ce qui suit dans setup?

      unsigned int oldAttribCount = m_MaxAttrib;
      unsigned int oldDataCount = m_MaxData;

      std::fill(m_CurrentAttribSlot.begin(),m_CurrentAttribSlot.end(),-1);
      std::fill(m_CurrentDataSlot.begin(),m_CurrentDataSlot.end(),-1);
      std::fill(m_CurrentTextureSlot.begin(),m_CurrentTextureSlot.end(),-1);

      unsigned int const* slotPtr = iTechnique->GetAttribSlots();
      unsigned int enabledAttribs = 0;
      for(unsigned int i = 0; i < iTechnique->GetMaxAttrib(); ++i)
      {
        m_CurrentAttribSlot[slotPtr[i]] = i;
        m_BufferBindings[i].buffer = NULL;
        unsigned int attribLoc = iTechnique->GetAttribLocation(i);
        //glEnableVertexAttribArray(attribLoc);
        enabledAttribs |= 1<<attribLoc;
      }

      for(unsigned int i = 0; i < 16; ++i)
      {
        if((m_SetAttribs & 1<<i) != 0)
        {
          glDisableVertexAttribArray(i);
        }
      }
      m_MaxAttrib = iTechnique->GetMaxAttrib();
      m_NeededAttribs = enabledAttribs;
      m_SetAttribs = 0;
      m_AttribFlags = (1 << m_MaxAttrib) - 1;

      slotPtr = iTechnique->GetUniformSlots();
      for(unsigned int i = 0; i < iTechnique->GetMaxUniform(); ++i)
      {
        m_CurrentDataSlot[slotPtr[i]] = i;
        //m_CurrentData[i] = m_DataCache[slotPtr[i]];
        m_CurrentData[i] = &m_DataCache[slotPtr[i]];
      }
      m_MaxData = iTechnique->GetMaxUniform();
      m_DataFlags = (1 << m_MaxData) - 1;

      slotPtr = iTechnique->GetTextureSlots();
      for(unsigned int i = 0; i < iTechnique->GetMaxTexture(); ++i)
      {
        m_CurrentTextureSlot[slotPtr[i]] = i;
        m_CurrentTexture[i] = &m_TextureCache[slotPtr[i]];
      }
      m_MaxTexture = iTechnique->GetMaxTexture();
      m_TexFlags = (1 << m_MaxTexture) - 1;

      glUseProgram(iTechnique->GetProgram()->GetProgName());
    }

    inline void SetVertexAttrib(unsigned int iAttribName, OGLBuffer const* iBuffer, unsigned int iNum, size_t iStride, size_t iOffset)
    {
      int currentSlot;
      if(iAttribName < m_CurrentAttribSlot.size() && (currentSlot = m_CurrentAttribSlot[iAttribName]) != -1)
      {
        if(iBuffer == NULL || iBuffer->GetBufferUsage() != OGLBufferUsage::ARRAY_BUFFER)
        {
          if(iBuffer->GetBufferUsage() != OGLBufferUsage::ARRAY_BUFFER)
          {
            LOG_WARNING<<"Invalid buffer for attrib"<<"\n";
          }
          m_BufferBindings[iAttribName].buffer = NULL;
          
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

    inline void SetUniformData(unsigned int iDataName, void const* iData)
    {
      //OGLSemanticManager::GetData(iDataName)->Copy(iData, m_DataCache[iDataName]);
      m_DataCache[iDataName] = iData;

      int currentSlot;
      if(iDataName < m_CurrentDataSlot.size() && (currentSlot = m_CurrentDataSlot[iDataName]) != -1)
      {
        m_DataFlags |= 1 << currentSlot;
      }
    }

    inline void SetTexture(unsigned int iTexName, OGLTexture const* iTex)
    {
      m_TextureCache[iTexName] = iTex;

      int currentSlot;
      if(iTexName < m_CurrentTextureSlot.size() && (currentSlot = m_CurrentTextureSlot[iTexName]) != -1)
      {
        m_TexFlags |= 1 << currentSlot;
      }
    }

    inline void CheckCache()
    {
      if(m_AttribFlags != 0 || m_DataFlags != 0 || m_TexFlags != 0)
      {
        CommitCache();
      }
    }

    void CommitCache()
    {
      for(unsigned int i = 0; i<m_CurrentAttribSlot.size(); ++i)
      {
        int slot = m_CurrentAttribSlot[i];
        if(slot >= 0 && m_AttribFlags & 1<<slot)
        {
          BufferBinding& binding = m_BufferBindings[i];
          if(binding.buffer != NULL)
          {
            if((m_SetAttribs & 1<<slot) == 0)
            {
              glEnableVertexAttribArray(slot);
              m_SetAttribs |= 1<<slot;
            }
            if(binding.buffer != m_CurrentArrayBuffer)
            {
              glBindBuffer(GL_ARRAY_BUFFER,binding.buffer->GetBufferId());
              m_CurrentArrayBuffer = binding.buffer;
            }
            m_CurrentTechnique->HandleAttribute(slot,binding.num,binding.stride,binding.offset);
          }
          else
          {
            if(m_SetAttribs & 1<<slot)
            {
              glDisableVertexAttribArray(slot);
              m_SetAttribs &= ~1<<slot;
            }
          }
        }
      }
      for(unsigned int i = 0; i<m_MaxData; ++i)
      {
        void const* curData;
        if(m_DataFlags & 1<<i && (curData = *m_CurrentData[i]) != NULL)
        {
          m_CurrentTechnique->HandleUniform(i,curData);
        }
      }
      for(unsigned int i = 0; i<m_MaxTexture; ++i)
      {
        OGLTexture const* curTexture;
        if(m_TexFlags & 1<<i && (curTexture = *m_CurrentTexture[i]) != NULL)
        {
          m_CurrentTechnique->HandleTexture(i,curTexture);
        }
      }

      m_AttribFlags = 0;
      m_DataFlags = 0;
      m_TexFlags = 0;
    }

    OGLCompiledTechnique const* m_CurrentTechnique;

    OGLBuffer const* m_CurrentArrayBuffer;
    OGLBuffer const* m_CurrentIndexBuffer;

    struct BufferBinding
    {
      OGLBuffer const* buffer;
      unsigned int attribName;
      unsigned int num;
      size_t stride;
      size_t offset;
    };

    std::vector<int> m_CurrentAttribSlot;
    std::vector<int> m_CurrentDataSlot;
    std::vector<int> m_CurrentTextureSlot;
    
    //std::vector<int> m_BackupDataSlot;
    //std::vector<int> m_BackupAttribSlot;
    std::vector<BufferBinding>       m_BufferBindings;
    //std::vector<void*>               m_DataCache;
    std::vector<void const*>         m_DataCache;
    std::vector<OGLTexture const*>   m_TextureCache;
    
    OGLTexture const**   m_CurrentTexture[8];
    void const**         m_CurrentData[16];

    OGLFramebuffer*      m_CurFramebuffer;
    
    //Type const* m_DataType[16];
    //void const* m_Data[16];

    unsigned int m_MaxData = 0;
    unsigned int m_MaxAttrib = 0;
    unsigned int m_MaxTexture = 0;

    unsigned int m_AttribFlags = 0;
    unsigned int m_NeededAttribs = 0;
    unsigned int m_SetAttribs = 0;
    unsigned int m_DataFlags = 0;
    unsigned int m_TexFlags = 0;

    //std::vector<Type const*> m_Types;
  };

  OGLRenderContext::OGLRenderContext():m_Impl(eXl_NEW OGLRenderContextImpl)
  {
  }

  void OGLRenderContext::SetFramebuffer(OGLFramebuffer* iFBO)
  {
    m_Impl->SetFramebuffer(iFBO);
  }

  OGLRenderContext::~OGLRenderContext()
  {
    eXl_DELETE m_Impl;
  }

  void OGLRenderContext::Clear()
  {
    m_Impl->CleanupState();
  }

  void OGLRenderContext::SetTechnique(OGLCompiledTechnique const* iTechnique)
  {
    m_Impl->SetTechnique(iTechnique);
  }

  void OGLRenderContext::SetVertexAttrib(unsigned int iAttribName, OGLBuffer const* iBuffer, unsigned int iNum, size_t iStride, size_t iOffset)
  {
    m_Impl->SetVertexAttrib(iAttribName,iBuffer,iNum,iStride,iOffset);
  }

  void OGLRenderContext::SetUniformData(unsigned int iDataName, void const* iData)
  {
    m_Impl->SetUniformData(iDataName,iData);
  }

  void OGLRenderContext::SetTexture(unsigned int iTexName, OGLTexture const* iTex)
  {
    m_Impl->SetTexture(iTexName,iTex);
  }

  void OGLRenderContext::Draw(OGLConnectivity iTopo, unsigned int iFirstVertex, unsigned int iNumVertices)
  {
    m_Impl->CheckCache();
    if(m_Impl->m_CurrentIndexBuffer != NULL)
    {
      m_Impl->m_CurrentIndexBuffer = NULL;
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    }

    glDrawArrays(GetGLConnectivity(iTopo), iFirstVertex, iNumVertices);
  }

  void OGLRenderContext::DrawIndexed(OGLBuffer const* iBuffer,OGLConnectivity iTopo, unsigned int iOffset, unsigned int iNumIndices)
  {
    if(iBuffer == NULL ||iBuffer->GetBufferUsage() != OGLBufferUsage::ELEMENT_ARRAY_BUFFER)
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
    glDrawElements(GetGLConnectivity(iTopo), iNumIndices, GL_UNSIGNED_INT, (void*)iOffset);
  }

}
