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

#include <ogl/oglutils.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <core/log.hpp>
#include <vector>

namespace eXl
{

  void OGLUtils::CheckOGLState(char const* iFile, int iLine)
  {
    GLenum error;
    while((error = glGetError()) != GL_NO_ERROR)
    {
      LOG_INFO<<iFile<<","<<iLine<<" ogl error : "<<error;
    }
  }

  //void OGLUtils::MakeOGLMatrix(float (&oMatrix) [16], Quaternionf const& iOrient, Vector3f const& iPos)
  void OGLUtils::MakeOGLMatrix(float* oMatrix, Quaternionf const& iOrient, Vector3f const& iPos)
  {
    oMatrix[3] = oMatrix[7] = oMatrix[11] = 0.0;
    oMatrix[15] = 1.0;
    oMatrix[12]=iPos.X();
    oMatrix[13]=iPos.Y();
    oMatrix[14]=iPos.Z();
    Vector3f rotMat[3];
    iOrient.ToRotationMatrix(rotMat);
    *((Vector3f*)(oMatrix + 0)) = rotMat[0];
    *((Vector3f*)(oMatrix + 4)) = rotMat[1];
    *((Vector3f*)(oMatrix + 8)) = rotMat[2];
  }

  GLuint OGLUtils::CompileShader(GLenum iType,char const* iSource)
  {
    GLuint shaderName = 0;
    if(iSource != NULL)
    {
      shaderName = glCreateShader(iType);
      if(shaderName != 0)
      {
        glShaderSource(shaderName, 1, &iSource, NULL);
        glCompileShader(shaderName);

        GLint result = GL_FALSE;
		    glGetShaderiv(shaderName, GL_COMPILE_STATUS, &result);
        if(result == GL_FALSE)
        {
		      LOG_ERROR <<"Compiling shader\n" << iSource << "\n";
		      int infoLogLength;
		      glGetShaderiv(shaderName, GL_INFO_LOG_LENGTH, &infoLogLength);
		      if(infoLogLength > 0)
		      {
			      std::vector<char> buffer(infoLogLength);
			      glGetShaderInfoLog(shaderName, infoLogLength, NULL, &buffer[0]);
			      LOG_ERROR << &buffer[0] << "\n";
		      }
          glDeleteShader(shaderName);
          shaderName = 0;
        }
      }
      else
        LOG_ERROR<<"glCreateShader failed"<<"\n";
    }
    return shaderName;
  }

  GLuint OGLUtils::LinkProgram(GLuint iVS, GLuint iPS)
  {
    GLuint progName = 0;
    if(iVS != 0 && iPS != 0)
    {
      progName = glCreateProgram();
      glAttachShader(progName,iVS);
      glAttachShader(progName,iPS);
      glLinkProgram(progName);

		  GLint result = GL_FALSE;
		  glGetProgramiv(progName, GL_LINK_STATUS, &result);

      if(result == GL_FALSE)
      {
		    LOG_ERROR << "Linking program" << "\n";
		    int infoLogLength;
		    glGetProgramiv(progName, GL_INFO_LOG_LENGTH, &infoLogLength);
		    if(infoLogLength > 0)
		    {
			    std::vector<char> buffer(infoLogLength);
			    glGetProgramInfoLog(progName, infoLogLength, NULL, &buffer[0]);
			    LOG_ERROR << &buffer[0] << "\n";
		    }
        glDeleteProgram(progName);
        progName = 0;
      }
    }
		return progName;
  }
}