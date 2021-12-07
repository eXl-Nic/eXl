/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include <core/heapobject.hpp>
#include <vector>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>

namespace eXl
{
  class OGLTexture;
  class OGLProgram;
  class OGLCompiledProgram;

  class OGLDataHandlerProgData : public HeapObject
  {
  public:
    typedef void (*SetValuefptr)(uint32_t ,uint32_t ,void const*);
    struct DataSetter
    {
      SetValuefptr setter;
      uint32_t location;
      uint32_t count;
      uint32_t offset;
    };

    std::vector<DataSetter>         m_UnifHandler[16];
    std::pair<uint32_t, uint32_t>   m_BlockHandler[16];
    std::pair<int, OGLSamplerDesc>  m_Samplers[4];
  };

  class EXL_OGL_API OGLProgramInterface : public HeapObject
  {
  public:

    OGLProgramInterface();

    static void InitStaticData();

    void AddAttrib(uint32_t iAttribName);
    void AddUniform(uint32_t iDataName);
    void AddTexture(uint32_t iTexName);

    OGLCompiledProgram* Compile (OGLProgram const* iProg);

  protected:
   
    std::vector<uint32_t>     m_AttribNames;
    std::vector<uint32_t>     m_UnifNames;
    std::vector<uint32_t>     m_Textures;
  };

  class OGLCompiledProgram : public HeapObject
  {
    friend OGLProgramInterface;
  public:

    void Setup() const;

    void HandleAttribute(uint32_t iAttribSlot, uint32_t iNum, size_t iStride, size_t iOffset) const;

    void HandleUniform(uint32_t iSlot, void const* iData) const;
    void HandleUniformBlock(uint32_t iSlot, uint32_t iBuffer) const;

    void HandleTexture(uint32_t iTexSlot, OGLTexture const* iTexture) const;

    uint32_t GetAttribLocation(uint32_t iAttribSlot) const;

    inline uint32_t const* GetAttribSlots() const{return m_AttribSlot;}
    inline uint32_t        GetMaxAttrib() const{return m_MaxAttrib;}

    inline uint32_t const* GetUniformSlots() const{return m_UnifSlot;}
    inline uint32_t        GetMaxUniform() const{return m_MaxUnif;}

    inline uint32_t const* GetUniformBlockSlots() const { return m_UnifBlockSlot; }
    inline uint32_t        GetMaxUniformBlocks() const { return m_MaxUnifBlock; }

    inline uint32_t const* GetTextureSlots() const{return m_TexSlot;}
    inline uint32_t        GetMaxTexture() const{return m_MaxTexture;}


    inline OGLProgram const* GetProgram() const{return m_Program;}

  protected:

    OGLCompiledProgram();

    OGLProgram const*             m_Program;
    OGLDataHandlerProgData        m_TechData;

    std::pair<uint32_t, OGLType> m_AttribDesc[16];

    uint32_t m_AttribSlot[16];
    uint32_t m_UnifSlot[16];
    uint32_t m_UnifBlockSlot[16];
    uint32_t m_TexSlot[4];

    uint32_t m_MaxAttrib = 0;
    uint32_t m_MaxUnif = 0;
    uint32_t m_MaxUnifBlock = 0;
    uint32_t m_MaxTexture = 0;
  };
}
