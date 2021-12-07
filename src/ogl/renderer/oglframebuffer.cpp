/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/oglframebuffer.hpp>

namespace eXl
{
  OGLFramebuffer::OGLFramebuffer(Vector2i const& iSize)
    :m_Size(iSize)
  {
#ifndef __ANDROID__
    glGenFramebuffers(1, &m_Id);
#endif
  }

  OGLFramebuffer::~OGLFramebuffer()
  {
#ifndef __ANDROID__
    glDeleteFramebuffers(1, &m_Id);
    if(!m_DepthStencilAttachement.m_IsTexture)
      glDeleteRenderbuffers(1, &m_DepthStencilAttachement.m_RenderBuffer);

    for(auto const& colAttach : m_ColorAttachements)
    {
      if(!colAttach.m_IsTexture)
        glDeleteRenderbuffers(1, &colAttach.m_RenderBuffer);
    }
#endif
  }

  void OGLFramebuffer::AddColorAttachement(OGLTexture* iTexture)
  {
#ifndef __ANDROID__
    if(iTexture)
    {
      eXl_ASSERT(iTexture->GetSize() == m_Size 
              && (iTexture->GetGLElementFormat() == GL_RGB
               || iTexture->GetGLElementFormat() == GL_RGBA));

      glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
      glBindTexture(GL_TEXTURE_2D, iTexture->GetId());
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_ColorAttachements.size(), GL_TEXTURE_2D, iTexture->GetId(), 0);

      m_ColorAttachements.push_back(Attachement());
      m_ColorAttachements.back().m_IsTexture = true;
      m_ColorAttachements.back().m_Texture = iTexture;
    }
    else
    {
      GLuint renderBuffer;
      glGenRenderbuffers(1, &renderBuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, m_Size.X(), m_Size.Y());
      glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_ColorAttachements.size(), GL_RENDERBUFFER, renderBuffer);

      m_ColorAttachements.push_back(Attachement());
      m_ColorAttachements.back().m_IsTexture = false;
      m_ColorAttachements.back().m_RenderBuffer = renderBuffer;

    }
#endif
  }

  void OGLFramebuffer::AddDepthStencilAttachement(OGLTexture* iTexture)
  {
#ifndef __ANDROID__
    if(m_DepthStencilAttachement.m_Texture == NULL)
    {
      if(iTexture)
      {
        eXl_ASSERT(iTexture->GetSize() == m_Size 
                && (iTexture->GetGLElementType() == GL_DEPTH_STENCIL 
                  || iTexture->GetGLElementType() == GL_DEPTH_COMPONENT));

        glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
        glBindTexture(GL_TEXTURE_2D, iTexture->GetId());
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, iTexture->GetId(), 0);
        m_DepthStencilAttachement.m_IsTexture = true;
        m_DepthStencilAttachement.m_Texture = iTexture;
      }
      else
      {
        GLuint renderBuffer;
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Size.X(), m_Size.Y());
        glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
        m_DepthStencilAttachement.m_IsTexture = false;
        m_DepthStencilAttachement.m_RenderBuffer = renderBuffer;
      }
    }
#endif
  }
}