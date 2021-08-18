/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

