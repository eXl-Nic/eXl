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

#pragma once

#include <math/vector2.hpp>
#include <math/aabb2d.hpp>
#include <ogl/oglexp.hpp>

namespace eXl
{
  class OGLTexture;

  class EXL_OGL_API OGLFramebuffer
  {
  public:
    OGLFramebuffer(Vector2i const& iSize);

    ~OGLFramebuffer();

    void AddColorAttachement(OGLTexture* iTexture);

    void AddDepthStencilAttachement(OGLTexture* iTexture);

    inline Vector2<uint32_t> const& GetSize() const {return m_Size;}
    inline unsigned int GetId() const{return m_Id;}

  protected:

    struct Attachement
    {
      Attachement() : m_Texture(NULL){}
      union
      {
        OGLTexture*  m_Texture;
        unsigned int m_RenderBuffer;
      };
      bool m_IsTexture;
    };

    std::vector<Attachement> m_ColorAttachements;
    Attachement              m_DepthStencilAttachement;
    Vector2<uint32_t>        m_Size;
    unsigned int             m_Id;
  };
}

