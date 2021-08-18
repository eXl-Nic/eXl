/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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