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

#include <boost/container/flat_map.hpp>
#include <core/heapobject.hpp>
#include <ogl/renderer/ogltypes.hpp>
#include <ogl/oglexp.hpp>

#include <string>

namespace eXl
{
  class EXL_OGL_API OGLProgram : public HeapObject
  {
  public:
    OGLProgram(unsigned int iProgramName);

    int GetAttribLocation(AString const& iName)const;
    int GetUniformLocation(AString const& iName)const;

    Err GetAttribDescription(AString const& iName, OGLType& oType, unsigned int& oNum)const;
    Err GetUniformDescription(AString const& iName, OGLType& oType, unsigned int& oNum)const;

    inline unsigned int GetProgName()const {return m_ProgramName;}

  protected:
    
    struct OGLAttribDesc
    {
      uint32_t location;
      OGLType type;
      uint32_t num;
    };

    struct OGLUniformDesc
    {
      uint32_t location;
      OGLType type;
      uint32_t num;
    };

    typedef boost::container::flat_map<AString,OGLAttribDesc> AttribDescMap;
    typedef boost::container::flat_map<AString,OGLUniformDesc> UniformDescMap;

    UniformDescMap m_UniformsMap;
    AttribDescMap m_AttribsMap;
    unsigned int m_ProgramName;
  };
}

