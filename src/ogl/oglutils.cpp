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

  void OGLUtils::Init()
  {
    static bool s_Initialized = false;
    if (!s_Initialized)
    {
#ifdef EXL_WITH_OGL
#ifndef __ANDROID__
      glewInit();
#endif
#endif

      //LOG_INFO << "GL version : " << (char*)glGetString(GL_VERSION) << "\n";
      s_Initialized = true;
    }
  }


  void OGLUtils::CheckOGLState(char const* iFile, int iLine)
  {
#ifdef EXL_WITH_OGL
    GLenum error;
    while((error = glGetError()) != GL_NO_ERROR)
    {
      LOG_INFO<<iFile<<","<<iLine<<" ogl error : "<<error;
    }
#endif
  }

  uint32_t OGLUtils::CompileVertexShader(char const* iSource)
  {
#ifdef EXL_WITH_OGL
    return CompileShader(GL_VERTEX_SHADER, iSource);
#else
    return 0;
#endif
  }

  uint32_t OGLUtils::CompileFragmentShader(char const* iSource)
  {
#ifdef EXL_WITH_OGL
    return CompileShader(GL_FRAGMENT_SHADER, iSource);
#else
    return 0;
#endif
  }

  uint32_t OGLUtils::CompileShader(uint32_t iType,char const* iSource)
  {
    uint32_t shaderName = 0;
#ifdef EXL_WITH_OGL
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
#endif
    return shaderName;
  }

  uint32_t OGLUtils::LinkProgram(uint32_t iVS, uint32_t iPS)
  {
    uint32_t progName = 0;
#ifdef EXL_WITH_OGL
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
#endif
		return progName;
  }
}