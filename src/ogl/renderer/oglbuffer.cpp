/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/oglutils.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltypesconv.hpp>

namespace eXl
{
  IMPLEMENT_RefC(OGLBuffer);
#ifdef __ANDROID__
  static void*  s_TempBuffer = NULL;
  static size_t s_TempBufferSize = 0;
#endif

  OGLBuffer* OGLBuffer::CreateBuffer(OGLBufferUsage iUsage, size_t iSize, void* iData)
  {
#ifdef EXL_WITH_OGL
    GLenum glUsage = GetGLUsage(iUsage);
    GLuint newBufferId;
    glGenBuffers(1,&newBufferId);
    if(newBufferId != 0)
    {
      glBindBuffer(glUsage,newBufferId);
      while(GL_NO_ERROR != glGetError()){}
      
      glBufferData(glUsage,iSize,iData,GL_STATIC_DRAW);

      GLenum error = glGetError();
      if(error == GL_NO_ERROR)
      {
        OGLBuffer* newBuffer = eXl_NEW OGLBuffer;

        newBuffer->m_Id = newBufferId;
        newBuffer->m_Size = iSize;
        newBuffer->m_Usage = iUsage;

        return newBuffer;
      }
    }
#endif
    return nullptr;
  }

  void OGLBuffer::SetData(size_t iOffset, size_t iSize, void* iData)
  {
#ifdef EXL_WITH_OGL
    GLenum glUsage = GetGLUsage(m_Usage);
    glBindBuffer(glUsage,m_Id);
    glBufferSubData(glUsage,iOffset,iSize,iData);
#endif
  }

  void* OGLBuffer::MapBuffer(OGLBufferAccess iAccess)
  {
#ifdef EXL_WITH_OGL
    GLenum glUsage = GetGLUsage(m_Usage);
    GLenum glAccess = GetGLAccess(iAccess);
#ifndef __ANDROID__
    glBindBuffer(glUsage,GetBufferId());
    return glMapBuffer(glUsage,glAccess);
#else
      //return glMapBufferOES(GL_ARRAY_BUFFER,GL_WRITE_ONLY_OES);
    if(GetBufferSize() > s_TempBufferSize)
    {
      s_TempBuffer = realloc(s_TempBuffer,GetBufferSize());
    }
    return s_TempBuffer;
#endif
#else
    return nullptr;
#endif
  }

  void OGLBuffer::UnmapBuffer()
  {
#ifdef EXL_WITH_OGL
    GLenum glUsage = GetGLUsage(m_Usage);
#ifndef __ANDROID__
    glBindBuffer(glUsage,m_Id);
    glUnmapBuffer(glUsage);
#else
    //glUnmapBufferOES(GL_ARRAY_BUFFER);
    glBindBuffer(glUsage,GetBufferId());
    glBufferSubData(glUsage,0,GetBufferSize(),s_TempBuffer);
#endif
    CHECKOGL();
#endif
  }

  OGLBuffer::~OGLBuffer()
  {
#ifdef EXL_WITH_OGL
    glDeleteBuffers(1,&m_Id);
#endif
  }

  OGLBuffer::OGLBuffer():m_Id(0)
  {

  }

}