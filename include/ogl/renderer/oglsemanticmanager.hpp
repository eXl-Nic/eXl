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

#include <core/string.hpp>
#include <ogl/renderer/ogltypes.hpp>
#include <ogl/oglexp.hpp>

namespace eXl
{
  class TupleType;

  struct OGLAttribDesc
  {
    AString m_Name;
    unsigned int m_Mult;
    OGLType m_Type;
  };

  struct OGLSamplerDesc
  {
    AString name;
    OGLMinFilter minFilter;
    OGLMagFilter maxFilter;
    OGLWrapMode wrapX;
    OGLWrapMode wrapY;
  };

  class EXL_OGL_API OGLSemanticManager
  {
  public:
    static unsigned int RegisterAttribute(AString const& iName, OGLType iAttribType, unsigned int iMult);

    static unsigned int RegisterUniformData(AString const& iName, TupleType const* iDataType);

    static unsigned int RegisterTexture(AString const& iTexName, OGLSamplerDesc const& iSampler);

    static OGLAttribDesc const& GetAttrib(unsigned int iName);

    static TupleType const* GetData(unsigned int iName);

    static OGLSamplerDesc const& GetSampler(unsigned int iName);

    static unsigned int GetNumAttribs();

    static unsigned int GetNumUniforms();

    static unsigned int GetNumTextures();
  };
}

