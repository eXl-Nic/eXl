#include <ogl/renderer/ogltypesconv.hpp>
#include <core/coredef.hpp>

namespace eXl
{
  GLenum GetGLUsage(OGLBufferUsage iUsage)
  {
    switch(iUsage)
    {
    case OGLBufferUsage::ARRAY_BUFFER:
      return GL_ARRAY_BUFFER;
      break;
    case OGLBufferUsage::ELEMENT_ARRAY_BUFFER:
      return GL_ELEMENT_ARRAY_BUFFER;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unsupported buffer usage");
      return GL_ARRAY_BUFFER;
      break;
    }
  }

  GLenum GetGLAccess(OGLBufferAccess iAccess)
  {
#ifdef __ANDROID__
      return 0;
#else

    switch(iAccess)
    {
    case OGLBufferAccess::READ:
      return GL_READ_ONLY;
      break;
    case OGLBufferAccess::WRITE:
      return GL_WRITE_ONLY;
      break;
    case OGLBufferAccess::READWRITE:
      return GL_READ_WRITE;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unsupported buffer access");
      return GL_READ_ONLY;
      break;
    }
#endif
  }

  GLenum GetGLType(OGLType iType)
  {
    switch(iType)
    {
    case OGLType::FLOAT32:
      return GL_FLOAT;
      break;
    case OGLType::FLOAT32_2:
      return GL_FLOAT_VEC2;
      break;
    case OGLType::FLOAT32_3:
      return GL_FLOAT_VEC3;
      break;
    case OGLType::FLOAT32_4:
      return GL_FLOAT_VEC4;
      break;
    case OGLType::INT32:
      return GL_INT;
      break;
    case OGLType::INT32_2:
      return GL_INT_VEC2;
      break;
    case OGLType::INT32_3:
      return GL_INT_VEC3;
      break;
    case OGLType::INT32_4:
      return GL_INT_VEC4;
      break;
    case OGLType::MAT3:
      return GL_FLOAT_MAT3;
      break;
    case OGLType::MAT4:
      return GL_FLOAT_MAT4;
      break;
    case OGLType::SAMPLER_2D:
      return GL_SAMPLER_2D;
      break;
    case OGLType::SAMPLER_CUBE:
      return GL_SAMPLER_CUBE;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unsupported type");
      return GL_INT;
      break;
    }
  }

  OGLType ConvertGLType(GLenum iType)
  {
    switch(iType)
    {
    case GL_FLOAT:
      return OGLType::FLOAT32;
      break;
    case GL_FLOAT_VEC2:
      return OGLType::FLOAT32_2;
      break;
    case GL_FLOAT_VEC3:
      return OGLType::FLOAT32_3;
      break;
    case GL_FLOAT_VEC4:
      return OGLType::FLOAT32_4;
      break;
    case GL_INT:
      return OGLType::INT32;
      break;
    case GL_INT_VEC2:
      return OGLType::INT32_2;
      break;
    case GL_INT_VEC3:
      return OGLType::INT32_3;
      break;
    case GL_INT_VEC4:
      return OGLType::INT32_4;
      break;
    case GL_FLOAT_MAT3:
      return OGLType::MAT3;
      break;
    case GL_FLOAT_MAT4:
      return OGLType::MAT4;
      break;
    case GL_SAMPLER_2D:
      return OGLType::SAMPLER_2D;
      break;
    case GL_SAMPLER_CUBE:
      return OGLType::SAMPLER_CUBE;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unsupported type");
      return OGLType::INT32;
      break;
    }
  }

  GLenum GetGLConnectivity(OGLConnectivity iConn)
  {
    switch(iConn)
    {
    case OGLConnectivity::POINT:
      return GL_POINTS;
      break;
    case OGLConnectivity::LINELIST:
      return GL_LINES;
      break;
    case OGLConnectivity::LINESTRIP:
      return GL_LINE_STRIP;
      break;
    case OGLConnectivity::TRIANGLELIST:
      return GL_TRIANGLES;
      break;
    case OGLConnectivity::TRIANGLESTRIP:
      return GL_TRIANGLE_STRIP;
      break;
    default:
      eXl_ASSERT_MSG(false, "Invalid connectivity ");
      return GL_POINTS;
      break;
    }
  }

  GLenum GetGLMinFilter(OGLMinFilter iMinFilter)
  {
    switch(iMinFilter)
    {
    case OGLMinFilter::NEAREST:
      return GL_NEAREST;
      break;
    case OGLMinFilter::LINEAR:
      return GL_LINEAR;
      break;
    case OGLMinFilter::NEAREST_MIPMAP_NEAREST:
      return GL_NEAREST_MIPMAP_NEAREST;
      break;
    case OGLMinFilter::NEAREST_MIPMAP_LINEAR:
      return GL_NEAREST_MIPMAP_LINEAR;
      break;
    case OGLMinFilter::LINEAR_MIPMAP_NEAREST:
      return GL_LINEAR_MIPMAP_NEAREST;
      break;
    case OGLMinFilter::LINEAR_MIPMAP_LINEAR:
      return GL_LINEAR_MIPMAP_LINEAR;
      break;
    default:
      eXl_ASSERT_MSG(false, "Invalid min filter");
      return GL_NEAREST;
      break;
    }
  }
  GLenum GetGLMagFilter(OGLMagFilter iMagFilter)
  {
    switch(iMagFilter)
    {
    case OGLMagFilter::NEAREST:
      return GL_NEAREST;
      break;
    case OGLMagFilter::LINEAR:
      return GL_LINEAR;
      break;
    default:
      eXl_ASSERT_MSG(false, "Invalid mag filter");
      return GL_NEAREST;
      break;
    }
  }

  GLenum GetGLWrapMode(OGLWrapMode iMode)
  {
    switch(iMode)
    {
    case OGLWrapMode::CLAMP_TO_EDGE:
      return GL_CLAMP_TO_EDGE;
      break;
    case OGLWrapMode::REPEAT:
      return GL_REPEAT;
      break;
    case OGLWrapMode::MIRRORED_REPEAT:
      return GL_MIRRORED_REPEAT;
      break;
    default:
      eXl_ASSERT_MSG(false, "Invalid wrap mode filter");
      return GL_CLAMP_TO_EDGE;
      break;
    }
  }

  GLenum GetGLBlend(OGLBlend iMode)
  {
    switch (iMode)
    {
    case OGLBlend::ZERO:
      return GL_ZERO;
      break;
    case OGLBlend::ONE:
      return GL_ONE;
      break;
    case OGLBlend::SRC_COLOR:
      return GL_SRC_COLOR;
      break;
    case OGLBlend::ONE_MINUS_SRC_COLOR:
      return GL_ONE_MINUS_SRC_COLOR;
      break;
    case OGLBlend::DST_COLOR:
      return GL_DST_COLOR;
      break;
    case OGLBlend::ONE_MINUS_DST_COLOR:
      return GL_ONE_MINUS_DST_COLOR;
      break;
    case OGLBlend::SRC_ALPHA:
      return GL_SRC_ALPHA;
      break;
    case OGLBlend::ONE_MINUS_SRC_ALPHA:
      return GL_ONE_MINUS_SRC_ALPHA;
      break;
    case OGLBlend::DST_ALPHA:
      return GL_DST_ALPHA;
      break;
    case OGLBlend::ONE_MINUS_DST_ALPHA:
      return GL_ONE_MINUS_DST_ALPHA;
      break;
    case OGLBlend::CONSTANT_COLOR:
      return GL_CONSTANT_COLOR;
      break;
    case OGLBlend::ONE_MINUS_CONSTANT_COLOR:
      return GL_ONE_MINUS_CONSTANT_COLOR;
      break;
    case OGLBlend::CONSTANT_ALPHA:
      return GL_CONSTANT_ALPHA;
      break;
    case OGLBlend::ONE_MINUS_CONSTANT_ALPHA:
      return GL_ONE_MINUS_CONSTANT_ALPHA;
      break;
    case OGLBlend::SRC_ALPHA_SATURATE:
      return GL_SRC_ALPHA_SATURATE;
      break;
#ifndef __ANDROID__
    case OGLBlend::SRC1_COLOR:
      return GL_SRC1_COLOR;
      break;
    case OGLBlend::ONE_MINUS_SRC1_COLOR:
      return GL_ONE_MINUS_SRC1_COLOR;
      break;
    case OGLBlend::SRC1_ALPHA:
      return GL_SRC1_ALPHA;
      break;
    case OGLBlend::ONE_MINUS_SRC1_ALPHA:
      return GL_ONE_MINUS_SRC1_ALPHA;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  };
}
