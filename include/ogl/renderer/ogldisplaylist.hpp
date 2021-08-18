/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "oglrendercommand.hpp"
#include <ogl/oglexp.hpp>
#include <math/vector2.hpp>
#include <vector>
#include <map>
#include <set>

#include <boost/cstdint.hpp>

#include "ogltypes.hpp"
#include "oglstatecollection.hpp"

namespace eXl
{
  class OGLRenderContext;
  class OGLFramebuffer;
  
  class EXL_OGL_API OGLDisplayList
  {
  public:

    void InitForPush();

    void InitForRender();

    void SetDefaultViewport(Vector2i const& iOrig, Vector2i const& iSize);

    void SetDefaultDepth(bool iWriteZ, bool iReadZ);

    void SetDefaultScissor(Vector2i const& iScissorOrig, Vector2i const& iScissorSize);

    void SetDefaultBlend(bool iEnabled, OGLBlend iSrc, OGLBlend iDst);

    void SetViewport(Vector2i const& iOrig, Vector2i const& iSize);

    void SetScissor(Vector2i const& iScissorOrig, Vector2i const& iScissorSize);

    void SetDepth(bool iWriteZ, bool iReadZ);

    void SetBlend(bool iEnabled, OGLBlend iSrc, OGLBlend iDst);

    void SetTechnique(OGLCompiledTechnique const* iTechnique);

    void SetVAssembly(OGLVAssembly const* iAssembly);

    void PushData(OGLShaderData const*);

    void PopData();

    void PushDraw(uint16_t iKey, unsigned char iTopo, unsigned int iNum, unsigned int iOffset);

    void Clear(uint16_t iKey, bool iClearColor, bool iClearDepth, Vector4f const& iColor = Vector4f::ZERO, float iDepth = 1.0);

    void Render(OGLRenderContext* iCtx, OGLFramebuffer* iFBO = NULL);

  protected:

    void FlushDraws();

    void HandleDataSet(OGLRenderContext* iCtx, OGLShaderDataSet const* iSet);

    struct PendingDraw
    {
      //OGLShaderDataSet const* data;
      uint32_t data;
      uint32_t num;
      uint32_t offset;
      uint16_t key;
      uint8_t topo;
    };

    std::vector<PendingDraw> m_PendingDraws;

    OGLStateCollection<OGLDepthCommand, OGLScissorCommand, OGLViewportCommand, OGLBlendCommand> m_States;
    
    OGLCompiledTechnique const* m_CurTechnique;
    OGLVAssembly const* m_CurAssembly;
    //OGLShaderDataSet const* m_CurDataSet;
    uint32_t m_CurDataSet = -1;

    //Need custom allocator
    Vector<OGLShaderDataSet> m_DataSetStore;
    UnorderedSet<OGLShaderDataSet> m_DataSetSeek;

    struct CommandKey
    {
      inline bool operator<(CommandKey const& iOther)const
      {
        return memcmp(this, &iOther, sizeof(CommandKey)) < 0;
      }
      //High level + Low level
      uint64_t m_Key;
      size_t   m_Offset;
    };
    std::vector<CommandKey>    m_Keys;
    std::vector<unsigned char> m_Commands;

    unsigned int m_Timestamp;
    //Data set for each slot.
    struct DataSetup
    {
      inline DataSetup():dataSet(NULL),timestamp(0){}
      OGLShaderDataSet const* dataSet;
      unsigned int timestamp;
    };
    std::vector<DataSetup> m_CurrentSetupData;
    std::vector<DataSetup> m_CurrentSetupTexture;
  };
}
