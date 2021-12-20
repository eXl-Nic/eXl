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
  class OGLCompiledProgram;
  class OGLRenderContextImpl;
  class OGLSemanticManager;

  class EXL_OGL_API OGLRenderContext
  {
  public:

    OGLRenderContext(OGLSemanticManager& iSemantics);

    ~OGLRenderContext();

    void Clear();

    void SetFramebuffer(OGLFramebuffer* iFBO);

    void SetProgram(OGLCompiledProgram const* iProgram);

    //void BindBuffer(OGLBuffer* iBuffer);

    void SetVertexAttrib(uint32_t iAttribName, OGLBuffer const* iBuffer, uint32_t iNum, size_t iStride, size_t iOffset);

    void SetUniformData(uint32_t iDataName, void const* iData);

    void SetUniformBuffer(uint32_t iDataName, OGLBuffer const* iBuffer);

    void SetTexture(uint32_t iTexName, OGLTexture const* iTex);

    void Draw(OGLConnectivity iTopo, uint32_t iFirstVertex, uint32_t iNumVertices);

    void DrawIndexed(OGLBuffer const* iBuffer, OGLConnectivity iTopo, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumIndices);

    void DrawInstanced(OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iBaseInstance, uint32_t iFirstVertex, uint32_t iNumVertices);

    void DrawIndexedInstanced(OGLBuffer const* iBuffer, OGLConnectivity iTopo, uint32_t iNumInstances, uint32_t iBaseInstance, uint32_t iOffset, uint32_t iBaseVertex, uint32_t iNumVertices);

  protected:
    OGLRenderContextImpl* m_Impl;
  };
}
