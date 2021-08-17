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
