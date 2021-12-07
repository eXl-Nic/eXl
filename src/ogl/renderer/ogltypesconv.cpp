/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
    case OGLBufferUsage::UNIFORM_BUFFER:
      return GL_UNIFORM_BUFFER;
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

  GLenum GetGLTextureType(OGLTextureType iTex)
  {
    switch (iTex)
    {
    case OGLTextureType::TEXTURE_1D:
      return GL_TEXTURE_1D;
      break;
    case OGLTextureType::TEXTURE_1D_ARRAY:
      return GL_TEXTURE_1D_ARRAY;
      break;
    case OGLTextureType::TEXTURE_2D:
      return GL_TEXTURE_2D;
      break;
    case OGLTextureType::TEXTURE_2D_ARRAY:
      return GL_TEXTURE_2D_ARRAY;
      break;
    case OGLTextureType::TEXTURE_3D:
      return GL_TEXTURE_3D;
      break;
    case OGLTextureType::TEXTURE_CUBE_MAP:
      return GL_TEXTURE_CUBE_MAP;
      break;
    case OGLTextureType::TEXTURE_BUFFER:
      return GL_TEXTURE_BUFFER;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }

  GLenum GetGLInternalTextureFormat(OGLInternalTextureFormat iFormat)
  {
    switch (iFormat)
    {
    case OGLInternalTextureFormat::DEPTH_COMPONENT:
      return GL_DEPTH_COMPONENT;
      break;
    case OGLInternalTextureFormat::DEPTH_STENCIL:
      return GL_DEPTH_STENCIL;
      break;
    case OGLInternalTextureFormat::RED:
      return GL_RED;
      break;
    case OGLInternalTextureFormat::RG:
      return GL_RG;
      break;
    case OGLInternalTextureFormat::RGB:
      return GL_RGB;
      break;
    case OGLInternalTextureFormat::RGBA:
      return GL_RGBA;
      break;
    case OGLInternalTextureFormat::R32F:
      return GL_R32F;
      break;
    case OGLInternalTextureFormat::RG32F:
      return GL_RG32F;
      break;
    case OGLInternalTextureFormat::RGB32F:
      return GL_RGB32F;
      break;
    case OGLInternalTextureFormat::RGBA32F:
      return GL_RGBA32F;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }

  GLenum GetGLTextureElementType(OGLTextureElementType iType)
  {
    switch (iType)
    {
      case OGLTextureElementType::UNSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
        break;
      case OGLTextureElementType::BYTE:
        return GL_BYTE;
        break;
      case OGLTextureElementType::UNSIGNED_SHORT:
        return GL_UNSIGNED_SHORT;
        break;
      case OGLTextureElementType::SHORT:
        return GL_SHORT;
        break;
      case OGLTextureElementType::UNSIGNED_INT:
        return GL_UNSIGNED_INT;
        break;
      case OGLTextureElementType::INT:
        return GL_INT;
        break;
      case OGLTextureElementType::HALF_FLOAT:
        return GL_HALF_FLOAT;
        break;
      case OGLTextureElementType::FLOAT:
        return GL_FLOAT;
        break;
      case OGLTextureElementType::UNSIGNED_SHORT_5_6_5:
        return GL_UNSIGNED_SHORT_5_6_5;
        break;
      case OGLTextureElementType::UNSIGNED_SHORT_5_6_5_REV:
        return GL_UNSIGNED_SHORT_5_6_5_REV;
        break;
      case OGLTextureElementType::UNSIGNED_SHORT_4_4_4_4:
        return GL_UNSIGNED_SHORT_4_4_4_4;
        break;
      case OGLTextureElementType::UNSIGNED_SHORT_5_5_5_1:
        return GL_UNSIGNED_SHORT_5_5_5_1;
        break;
      case OGLTextureElementType::UNSIGNED_INT_2_10_10_10_REV:
        return GL_UNSIGNED_INT_2_10_10_10_REV;
        break;
      default:
        eXl_ASSERT_MSG(false, "Unrecognized GL constant");
        return 0;
        break;
    }
  }

  GLenum GetGLTextureFormat(OGLTextureFormat iFormat)
  {
    switch (iFormat)
    {
    case OGLTextureFormat::RED:
      return GL_RED;
      break;
    case OGLTextureFormat::RG:
      return GL_RG;
      break;
    case OGLTextureFormat::RGB:
      return GL_RGB;
      break;
    case OGLTextureFormat::BGR:
      return GL_BGR;
      break;
    case OGLTextureFormat::RGBA:
      return GL_RGBA;
      break;
    case OGLTextureFormat::BGRA:
      return GL_BGRA;
      break;
    case OGLTextureFormat::RED_INTEGER:
      return GL_RED_INTEGER;
      break;
    case OGLTextureFormat::RG_INTEGER:
      return GL_RG_INTEGER;
      break;
    case OGLTextureFormat::RGB_INTEGER:
      return GL_RGB_INTEGER;
      break;
    case OGLTextureFormat::BGR_INTEGER:
      return GL_BGR_INTEGER;
      break;
    case OGLTextureFormat::RGBA_INTEGER:
      return GL_RGBA_INTEGER;
      break;
    case OGLTextureFormat::BGRA_INTEGER:
      return GL_BGRA_INTEGER;
      break;
    case OGLTextureFormat::STENCIL_INDEX:
      return GL_STENCIL_INDEX;
      break;
    case OGLTextureFormat::DEPTH_COMPONENT:
      return GL_DEPTH_COMPONENT;
      break;
    case OGLTextureFormat::DEPTH_STENCIL:
      return GL_DEPTH_STENCIL;
      break;
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }
}
