/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <ogl/oglexp.hpp>
#include <ogl/renderer/ogltypes.hpp>

#include <math/vector2.hpp>
#include <math/aabb2d.hpp>
#include <core/heapobject.hpp>
#include <core/refcobject.hpp>
#include <core/image/image.hpp>

namespace eXl
{
  class OGLBuffer;
  class OGLTextureLoader;

  class EXL_OGL_API OGLTexture : public HeapObject
  {
    friend class OGLTextureLoader;
    DECLARE_RefC;
  public:

    OGLTexture(OGLBuffer* iBuffer, OGLInternalTextureFormat iFormat);
    OGLTexture(IntrusivePtr<OGLBuffer> const& iBuffer, OGLInternalTextureFormat iFormat);
    OGLTexture(Image::Size const& iSize, OGLTextureType iTextureType, OGLInternalTextureFormat iFormat, uint32_t iNumSlices = 0);

    void AllocateTexture();

    ~OGLTexture();

    //void OnNullRefC() const;

    //void DropTextureData();

    void Update(AABB2Di iBox, OGLTextureElementType iType, OGLTextureFormat iFormat, void const* iData, uint32_t iMip = 0, uint32_t iSlice = 0);

    uint32_t           GetId() const { return m_TexId; }
    Image::Size const& GetSize() const { return m_Size; }
    uint32_t           GetNumSlices() const { return m_NumSlices; }
    OGLTextureType GetTextureType() const { return m_TextureType; }
    OGLTextureElementType GetElementType() const;
    OGLTextureFormat GetElementFormat() const;
    uint32_t GetGLElementType() const;
    uint32_t GetGLElementFormat() const;

    inline bool IsCubeMap() const {return m_TextureType == OGLTextureType::TEXTURE_CUBE_MAP;}

    uint32_t GetAPIHandle() const { return m_TexId; }

  protected:

    IntrusivePtr<OGLBuffer> m_Buffer;

    Image::Size              m_Size;
    uint32_t                 m_NumSlices = 0;
    uint32_t                 m_TexId = 0;
    OGLTextureType           m_TextureType;
    OGLInternalTextureFormat m_InternalFormat;
  };
}
