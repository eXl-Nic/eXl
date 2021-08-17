/**

  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
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
              && (iTexture->GetFormat() == GL_RGB
               || iTexture->GetFormat() == GL_RGBA));

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
                && (iTexture->GetType() == GL_DEPTH_STENCIL || iTexture->GetType() == GL_DEPTH_COMPONENT));

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