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
  uint32_t GetGLUsage(OGLBufferUsage iUsage)
  {
    switch(iUsage)
    {
#ifdef EXL_WITH_OGL
    case OGLBufferUsage::ARRAY_BUFFER:
      return GL_ARRAY_BUFFER;
      break;
    case OGLBufferUsage::ELEMENT_ARRAY_BUFFER:
      return GL_ELEMENT_ARRAY_BUFFER;
      break;
    case OGLBufferUsage::UNIFORM_BUFFER:
      return GL_UNIFORM_BUFFER;
      break;
    case OGLBufferUsage::TEXTURE_BUFFER:
      return GL_TEXTURE_BUFFER;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Unsupported buffer usage");
      return 0;
      break;
    }
  }

  uint32_t GetGLAccess(OGLBufferAccess iAccess)
  {
#ifdef __ANDROID__
      return 0;
#else

    switch(iAccess)
    {
#ifdef EXL_WITH_OGL
    case OGLBufferAccess::READ:
      return GL_READ_ONLY;
      break;
    case OGLBufferAccess::WRITE:
      return GL_WRITE_ONLY;
      break;
    case OGLBufferAccess::READWRITE:
      return GL_READ_WRITE;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Unsupported buffer access");
      return 0;
      break;
    }
#endif
  }

  uint32_t GetGLType(OGLType iType)
  {
    switch(iType)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Unsupported type");
      return 0;
      break;
    }
  }

  OGLType ConvertGLType(uint32_t iType)
  {
    switch(iType)
    {
#ifdef EXL_WITH_OGL
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
    case GL_SAMPLER_1D:
      return OGLType::SAMPLER_1D;
      break;
    case GL_SAMPLER_1D_ARRAY:
      return OGLType::SAMPLER_1D_ARRAY;
      break;
    case GL_SAMPLER_2D:
      return OGLType::SAMPLER_2D;
      break;
    case GL_SAMPLER_2D_ARRAY:
      return OGLType::SAMPLER_2D_ARRAY;
      break;
    case GL_SAMPLER_3D:
      return OGLType::SAMPLER_3D;
      break;
    case GL_SAMPLER_CUBE:
      return OGLType::SAMPLER_CUBE;
      break;
    case GL_SAMPLER_BUFFER:
      return OGLType::SAMPLER_BUFFER;
      break;
    case GL_INT_SAMPLER_1D:
      return OGLType::INT_SAMPLER_1D;
      break;
    case GL_INT_SAMPLER_1D_ARRAY:
      return OGLType::INT_SAMPLER_1D_ARRAY;
      break;
    case GL_INT_SAMPLER_2D:
      return OGLType::INT_SAMPLER_2D;
      break;
    case GL_INT_SAMPLER_2D_ARRAY:
      return OGLType::INT_SAMPLER_2D_ARRAY;
      break;
    case GL_INT_SAMPLER_3D:
      return OGLType::INT_SAMPLER_3D;
      break;
    case GL_INT_SAMPLER_CUBE:
      return OGLType::INT_SAMPLER_CUBE;
      break;
    case GL_INT_SAMPLER_BUFFER:
      return OGLType::INT_SAMPLER_BUFFER;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Unsupported type");
      return OGLType::INT32;
      break;
    }
  }

  uint32_t GetGLConnectivity(OGLConnectivity iConn)
  {
    switch(iConn)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Invalid connectivity ");
      return 0;
      break;
    }
  }

  uint32_t GetGLMinFilter(OGLMinFilter iMinFilter)
  {
    switch(iMinFilter)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Invalid min filter");
      return 0;
      break;
    }
  }
  uint32_t GetGLMagFilter(OGLMagFilter iMagFilter)
  {
    switch(iMagFilter)
    {
#ifdef EXL_WITH_OGL
    case OGLMagFilter::NEAREST:
      return GL_NEAREST;
      break;
    case OGLMagFilter::LINEAR:
      return GL_LINEAR;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Invalid mag filter");
      return 0;
      break;
    }
  }

  uint32_t GetGLWrapMode(OGLWrapMode iMode)
  {
    switch(iMode)
    {
#ifdef EXL_WITH_OGL
    case OGLWrapMode::CLAMP_TO_EDGE:
      return GL_CLAMP_TO_EDGE;
      break;
    case OGLWrapMode::REPEAT:
      return GL_REPEAT;
      break;
    case OGLWrapMode::MIRRORED_REPEAT:
      return GL_MIRRORED_REPEAT;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Invalid wrap mode filter");
      return 0;
      break;
    }
  }

  uint32_t GetGLBlend(OGLBlend iMode)
  {
    switch (iMode)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  };

  uint32_t GetGLTextureType(OGLTextureType iTex)
  {
    switch (iTex)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }

  uint32_t GetGLInternalTextureFormat(OGLInternalTextureFormat iFormat)
  {
    switch (iFormat)
    {
#ifdef EXL_WITH_OGL
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
    case OGLInternalTextureFormat::R8:
      return GL_R8;
      break;
    case OGLInternalTextureFormat::RG8:
      return GL_RG8;
      break;
    case OGLInternalTextureFormat::RGB8:
      return GL_RGB8;
      break;
    case OGLInternalTextureFormat::RGBA8:
      return GL_RGBA8;
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
    case OGLInternalTextureFormat::R32I:
      return GL_R32I;
      break;
    case OGLInternalTextureFormat::RG32I:
      return GL_RG32I;
      break;
    case OGLInternalTextureFormat::RGB32I:
      return GL_RGB32I;
      break;
    case OGLInternalTextureFormat::RGBA32I:
      return GL_RGBA32I;
      break;
#endif
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }

  uint32_t GetGLTextureElementType(OGLTextureElementType iType)
  {
    switch (iType)
    {
#ifdef EXL_WITH_OGL
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
#endif
      default:
        eXl_ASSERT_MSG(false, "Unrecognized GL constant");
        return 0;
        break;
    }
  }

  uint32_t GetGLTextureFormat(OGLTextureFormat iFormat)
  {
    switch (iFormat)
    {
#ifdef EXL_WITH_OGL
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
#endif
    default:
      eXl_ASSERT_MSG(false, "Unrecognized GL constant");
      return 0;
      break;
    }
  }

  bool IsSampler(OGLType iType)
  {
    switch (iType)
    {
    case OGLType::FLOAT32:
    case OGLType::FLOAT32_2:
    case OGLType::FLOAT32_3:
    case OGLType::FLOAT32_4:
    case OGLType::INT32:
    case OGLType::INT32_2:
    case OGLType::INT32_3:
    case OGLType::INT32_4:
    case OGLType::MAT3:
    case OGLType::MAT4:
      return false;
      break;
    case OGLType::SAMPLER_1D:
    case OGLType::SAMPLER_1D_ARRAY:
    case OGLType::SAMPLER_2D:
    case OGLType::SAMPLER_2D_ARRAY:
    case OGLType::SAMPLER_3D:
    case OGLType::SAMPLER_CUBE:
    case OGLType::SAMPLER_BUFFER:
    case OGLType::INT_SAMPLER_1D:
    case OGLType::INT_SAMPLER_1D_ARRAY:
    case OGLType::INT_SAMPLER_2D:
    case OGLType::INT_SAMPLER_2D_ARRAY:
    case OGLType::INT_SAMPLER_3D:
    case OGLType::INT_SAMPLER_CUBE:
    case OGLType::INT_SAMPLER_BUFFER:
      return true;
      break;
    default:
      eXl_FAIL_MSG_RET("Invalid type constant", false);
      break;
    }
  }

  OGLTextureType GetSamplerTextureType(OGLType iType)
  {
    switch (iType)
    {
    case OGLType::SAMPLER_1D:
    case OGLType::INT_SAMPLER_1D:
      return OGLTextureType::TEXTURE_1D;
      break;
    case OGLType::SAMPLER_1D_ARRAY:
    case OGLType::INT_SAMPLER_1D_ARRAY:
      return OGLTextureType::TEXTURE_1D_ARRAY;
      break;
    case OGLType::SAMPLER_2D:
    case OGLType::INT_SAMPLER_2D:
      return OGLTextureType::TEXTURE_2D;
      break;
    case OGLType::SAMPLER_2D_ARRAY:
    case OGLType::INT_SAMPLER_2D_ARRAY:
      return OGLTextureType::TEXTURE_2D_ARRAY;
      break;
    case OGLType::SAMPLER_3D:
    case OGLType::INT_SAMPLER_3D:
      return OGLTextureType::TEXTURE_3D;
      break;
    case OGLType::SAMPLER_CUBE:
    case OGLType::INT_SAMPLER_CUBE:
      return OGLTextureType::TEXTURE_CUBE_MAP;
      break;
    case OGLType::SAMPLER_BUFFER:
    case OGLType::INT_SAMPLER_BUFFER:
      return OGLTextureType::TEXTURE_BUFFER;
      break;
    default:
      eXl_FAIL_MSG_RET("Invalid type constant, expected a sampler", OGLTextureType::TEXTURE_1D);
      break;
    }
  }
}
