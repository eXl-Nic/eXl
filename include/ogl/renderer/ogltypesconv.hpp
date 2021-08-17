#pragma once

#include <ogl/renderer/ogltypes.hpp>
#include <ogl/renderer/oglinclude.hpp>

namespace eXl
{
  GLenum GetGLUsage(OGLBufferUsage iUsage);
  GLenum GetGLAccess(OGLBufferAccess iAccess);
  GLenum GetGLType(OGLType iType);
  OGLType ConvertGLType(GLenum iType);
  GLenum GetGLConnectivity(OGLConnectivity iConn);
  GLenum GetGLMinFilter(OGLMinFilter iMinFilter);
  GLenum GetGLMagFilter(OGLMagFilter iMagFilter);
  GLenum GetGLWrapMode(OGLWrapMode iMode);
  GLenum GetGLBlend(OGLBlend iMode);
}
