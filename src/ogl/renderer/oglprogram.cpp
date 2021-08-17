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

#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <core/log.hpp>
#include <core/coredef.hpp>

#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltypesconv.hpp>

namespace eXl
{
  namespace
  {
    void SplitAttrib(OGLType iType, OGLType& oType, unsigned int& oNum)
    {
      switch(iType)
      {
      case OGLType::FLOAT32_2:
        oType = OGLType::FLOAT32;
        oNum = 2;
        break;
      case OGLType::FLOAT32_3:
        oType = OGLType::FLOAT32;
        oNum = 3;
        break;
      case OGLType::FLOAT32_4:
        oType = OGLType::FLOAT32;
        oNum = 4;
        break;
      case OGLType::INT32_2:
        oType = OGLType::INT32;
        oNum = 2;
        break;
      case OGLType::INT32_3:
        oType = OGLType::INT32;
        oNum = 3;
        break;
      case OGLType::INT32_4:
        oType = OGLType::INT32;
        oNum = 4;
        break;
      default:
        oType = iType;
        oNum = 1;
      };
    }
  }

  OGLProgram::OGLProgram(GLuint iProgramName):m_ProgramName(iProgramName)
  {
    if(iProgramName != 0)
    {
      GLint numUnif;
      GLint numAttribs;
      GLint nameLength;
      GLint attribNameLength;
      glGetProgramiv(m_ProgramName,GL_ACTIVE_ATTRIBUTES,&numAttribs);
      glGetProgramiv(m_ProgramName,GL_ACTIVE_UNIFORMS,&numUnif);
      glGetProgramiv(m_ProgramName,GL_ACTIVE_UNIFORM_MAX_LENGTH,&nameLength);
      glGetProgramiv(m_ProgramName,GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,&attribNameLength);
      nameLength = nameLength > attribNameLength ? nameLength : attribNameLength;
      
      std::vector<char> nameBuff(nameLength);

      if(numAttribs > 0 && nameLength > 0)
      {
        m_AttribsMap.reserve(numAttribs);
        for(int i = 0; i < numAttribs; ++i)
        {
          GLint oNameLength;
          GLenum oType;
          GLint  oSize;
          glGetActiveAttrib(m_ProgramName,i,nameLength,&oNameLength,&oSize,&oType,&nameBuff[0]);
          GLint loc = glGetAttribLocation(m_ProgramName,&nameBuff[0]);
          //eXl_ASSERT_MSG(loc != -1, "Problem with attrib location");
         
          //LOG_INFO<<"Inserted attrib"<<String(nameBuff.begin(),nameBuff.begin() + oNameLength)<<"\n";
          OGLAttribDesc newDesc;
          newDesc.location = loc;
          newDesc.num = oSize;
          newDesc.type = ConvertGLType(oType);
          m_AttribsMap.insert(std::make_pair(AString(nameBuff.begin(),nameBuff.begin() + oNameLength),newDesc));
         
        }
      }
      if(numUnif > 0 && nameLength > 0)
      {
        m_UniformsMap.reserve(numUnif);
        for(int i = 0; i < numUnif; ++i)
        {
          GLint oNameLength;
          GLenum oType;
          GLint  oSize;
          glGetActiveUniform(m_ProgramName,i,nameLength,&oNameLength,&oSize,&oType,&nameBuff[0]);
          GLint loc = glGetUniformLocation(m_ProgramName,&nameBuff[0]);
          eXl_ASSERT_MSG(loc != -1, "Problem with uniform location");
          OGLUniformDesc newDesc;
          newDesc.location = loc;
          newDesc.num = oSize;
          newDesc.type = ConvertGLType(oType);
          //LOG_INFO<<"Inserted uniform"<<String(nameBuff.begin(),nameBuff.begin() + oNameLength)<<"\n";
          m_UniformsMap.insert(std::make_pair(AString(nameBuff.begin(),nameBuff.begin() + oNameLength),newDesc));
        }
      }
    }
  }

  GLint OGLProgram::GetAttribLocation(AString const& iName)const
  {
    AttribDescMap::const_iterator iter = m_AttribsMap.find(iName);
    if(iter != m_AttribsMap.end())
    {
      return iter->second.location;
    }
    return -1;
  }

  GLint OGLProgram::GetUniformLocation(AString const& iName)const
  {
    UniformDescMap::const_iterator iter = m_UniformsMap.find(iName);
    if(iter != m_UniformsMap.end())
    {
      return iter->second.location;
    }
    return -1;
  }

  Err OGLProgram::GetAttribDescription(AString const& iName, OGLType& oType, unsigned int& oNum)const
  {
    AttribDescMap::const_iterator iter = m_AttribsMap.find(iName);
    if(iter != m_AttribsMap.end())
    {
      SplitAttrib(iter->second.type,oType,oNum);
      oNum *= iter->second.num;
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }

  Err OGLProgram::GetUniformDescription(AString const& iName, OGLType& oType, unsigned int& oNum)const
  {
    UniformDescMap::const_iterator iter = m_UniformsMap.find(iName);
    if(iter != m_UniformsMap.end())
    {
      oType = iter->second.type;
      oNum = iter->second.num;
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }
}