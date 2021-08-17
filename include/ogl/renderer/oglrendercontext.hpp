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

#include <core/coredef.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/ogltypes.hpp>

namespace eXl
{
  class OGLBuffer;
  class OGLTexture;
  class OGLFramebuffer;
  class OGLCompiledTechnique;
  class OGLRenderContextImpl;

  class EXL_OGL_API OGLRenderContext
  {
  public:

    OGLRenderContext();

    ~OGLRenderContext();

    void Clear();

    void SetFramebuffer(OGLFramebuffer* iFBO);

    void SetTechnique(OGLCompiledTechnique const* iTechnique);

    //void BindBuffer(OGLBuffer* iBuffer);

    void SetVertexAttrib(unsigned int iAttribName, OGLBuffer const* iBuffer, unsigned int iNum, size_t iStride, size_t iOffset);

    void SetUniformData(unsigned int iDataName, void const* iData);

    void SetTexture(unsigned int iTexName, OGLTexture const* iTex);

    void Draw(OGLConnectivity iTopo, unsigned int iFirstVertex, unsigned int iNumVertices);

    void DrawIndexed(OGLBuffer const* iBuffer, OGLConnectivity iTopo, unsigned int iOffset, unsigned int iNumIndices);

  protected:
    OGLRenderContextImpl* m_Impl;
    //OGLTechnique* m_CurTechnique;
  };
}
