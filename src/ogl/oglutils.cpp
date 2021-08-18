/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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