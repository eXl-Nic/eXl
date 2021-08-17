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
    return NULL;
  }

  void OGLBuffer::SetData(size_t iOffset, size_t iSize, void* iData)
  {
    GLenum glUsage = GetGLUsage(m_Usage);
    glBindBuffer(glUsage,m_Id);
    glBufferSubData(glUsage,iOffset,iSize,iData);
  }

  void* OGLBuffer::MapBuffer(OGLBufferAccess iAccess)
  {
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
  }

  void OGLBuffer::UnmapBuffer()
  {
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
  }

  OGLBuffer::~OGLBuffer()
  {
    glDeleteBuffers(1,&m_Id);
  }

  OGLBuffer::OGLBuffer():m_Id(0)
  {

  }

}