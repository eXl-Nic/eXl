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