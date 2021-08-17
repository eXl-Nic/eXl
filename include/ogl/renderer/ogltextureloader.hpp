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
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <core/image/image.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/ogltexture.hpp>

namespace eXl
{
  //class DataVault;
  class OGLTexture;
  class Image;

  class EXL_OGL_API OGLTextureLoader
  {
  public:

    enum Format
    {
      R8,
      RG8,
      RGB8,
      RGBA8,
      R32F,
      RG32F,
      RGB32F,
      RGBA32F
    };

    //OGLTextureLoader(std::list<DataVault*> const& iSource);

    static OGLTexture* Create(Image::Size const& iSize, Format iFormat);

    static OGLTexture* CreateFromBuffer(Image::Size const& iSize, Format iFormat, void const* iData, bool iGenMipMap);

    static OGLTexture* CreateFromImage(Image const* iImage, bool iGenMipMap);

    static OGLTexture* CreateCubeMap(Image const* const* iImage, bool iGenMipMap);

    static Err ReadTexture(OGLTexture* iTexture, Image*& oImage, int iFace = -1);

    //void ForgetTexture(OGLTexture const* iTexture);

  protected:

    //std::list<DataVault*> m_Source;

    //typedef std::map<String, OGLTexture*> TextureMap;
    //TextureMap m_TextureCache;
  };
}