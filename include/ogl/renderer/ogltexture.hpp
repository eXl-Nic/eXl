/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <ogl/oglexp.hpp>

#include <math/vector2.hpp>
#include <math/aabb2d.hpp>
#include <core/heapobject.hpp>
#include <core/refcobject.hpp>
#include <core/image/image.hpp>

namespace eXl
{
  class OGLTextureLoader;

  class EXL_OGL_API OGLTexture : public HeapObject
  {
    friend class OGLTextureLoader;
    DECLARE_RefC;
  public:
    OGLTexture(Image::Size const& iSize, unsigned int iType);

    ~OGLTexture();

    //void OnNullRefC() const;

    //void DropTextureData();

    void Update(AABB2Di iBox, void const* iData, unsigned int iMip = 0, unsigned int iFace = 0);

    inline unsigned int    GetId()      const {return m_TexId;}
    inline Image::Size const& GetSize() const {return m_Size;}
    inline unsigned int    GetType()    const {return m_TextureType;}
    inline unsigned int    GetFormat()  const {return m_TextureFormat;}

    inline bool IsCubeMap() const {return m_CubeMap;}

    uint32_t GetAPIHandle() const { return m_TexId; }

  protected:

    OGLTextureLoader* m_Loader;

    Image::Size    m_Size;
    unsigned int   m_TexId;
    unsigned short m_TextureType;   //GL_UNSIGNED_CHAR,GL_UNSIGNED_SHORT,GL_UNSIGNED_INT
    unsigned short m_TextureFormat; //GL_R,GL_RG,GL_RGB,GL_RGBA
    bool           m_CubeMap;
    //void*          m_TextureData;
  };

  //FUNC_RefC(OGLTexture)
}