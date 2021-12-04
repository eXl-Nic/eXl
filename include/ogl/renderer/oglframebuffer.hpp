/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
    inline uint32_t GetId() const{return m_Id;}

  protected:

    struct Attachement
    {
      Attachement() : m_Texture(NULL){}
      union
      {
        OGLTexture*  m_Texture;
        uint32_t m_RenderBuffer;
      };
      bool m_IsTexture;
    };

    std::vector<Attachement> m_ColorAttachements;
    Attachement              m_DepthStencilAttachement;
    Vector2<uint32_t>        m_Size;
    uint32_t                 m_Id;
  };
}

