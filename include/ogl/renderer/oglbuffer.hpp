/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/heapobject.hpp>
#include <core/refcobject.hpp>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/ogltypes.hpp>

namespace eXl
{

  class EXL_OGL_API OGLBuffer : public HeapObject
  {
    DECLARE_RefC;
  public:

    static OGLBuffer* CreateBuffer(OGLBufferUsage iUsage, size_t iSize, void* iData = NULL);

    void SetData(size_t iOffset, size_t iSize, void* iData);

    void* MapBuffer(OGLBufferAccess iAccess);

    void UnmapBuffer();

    inline uint32_t GetBufferId()const{return m_Id;}
    inline OGLBufferUsage GetBufferUsage()const{return m_Usage;}
    inline size_t GetBufferSize()const{return m_Size;}

    ~OGLBuffer();

  protected:

    OGLBuffer();

    OGLBufferUsage m_Usage;
    uint32_t m_Id;
    size_t m_Size;
  };

}