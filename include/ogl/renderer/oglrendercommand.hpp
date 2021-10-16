/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//#include <cstdint>
#include <math/vector2.hpp>
#include <cstdlib>
#include <vector>

#include "ogltypes.hpp"

namespace eXl
{
  class OGLBuffer;
  class OGLShaderData;
  class OGLCompiledTechnique;
  class OGLRenderContext;

  struct OGLRenderCommand
  {
    enum Kind
    {
      StateCommand    = 0,
      DrawCommand     = 1,
      //CustomCommand   = 2,
      //OverrideCommand = 3,
      Mask = 7,
      Shift = 0
    };
    //0-7 -> bits 0,1,2
  };

  struct OGLStateCommand
  {
    enum
    {
      ScissorCommand  = 0<<3,
      DepthCommand    = 1<<3,
      ViewportCommand = 2<<3,
      BlendCommand    = 3<<3,
      Mask = (4 - 1)<<3,
      Shift = 3
    };
    //8-63 -> bits 3,4,5,6
  };

  struct OGLScissorCommand
  {
    enum
    {
      EnableScissor = 1<<7
    };

    inline bool operator != (OGLScissorCommand const& iOther) const
    {
      return m_Flag != iOther.m_Flag 
          || m_ScissorCoord[0] != iOther.m_ScissorCoord[0]
          || m_ScissorCoord[1] != iOther.m_ScissorCoord[1]
          || m_ScissorCoord[2] != iOther.m_ScissorCoord[2]
          || m_ScissorCoord[3] != iOther.m_ScissorCoord[3];
    }
    inline bool operator == (OGLScissorCommand const& iOther) const {return ! operator!=(iOther);}

    void Apply();

    uint16_t m_Flag = 0;
    uint16_t m_Padding = 0;
    unsigned int  m_ScissorCoord[4];

  };

  struct OGLDepthCommand
  {
    enum
    {
      WriteZ = 1<<7,
      ReadZ  = 1<<8
    };

    inline bool operator != (OGLDepthCommand const& iOther) const
    {
      return m_Flag != iOther.m_Flag ;
    }
    inline bool operator == (OGLDepthCommand const& iOther) const {return ! operator!=(iOther);}

    void Apply();

    uint16_t m_Flag = 0;
    uint16_t m_Padding = 0;
  };

  
  struct OGLViewportCommand
  {
    inline bool operator != (OGLViewportCommand const& iOther) const
    {
      return m_Flag != iOther.m_Flag || m_Orig != iOther.m_Orig || m_Size != iOther.m_Size;
    }
    inline bool operator == (OGLViewportCommand const& iOther) const {return ! operator!=(iOther);}

    void Apply();
    uint16_t m_Flag = 0;
    uint16_t m_Padding = 0;
    Vector2i m_Orig;
    Vector2i m_Size;
  };

  struct OGLBlendCommand
  {
    enum
    {
      Enabled = 1 << 7,
    };

    inline bool operator != (OGLBlendCommand const& iOther) const
    {
      return m_Flag != iOther.m_Flag
        || srcRGB != iOther.srcRGB
        || dstRGB != iOther.dstRGB
        || srcAlpha != iOther.srcAlpha
        || dstAlpha != iOther.dstAlpha;
    }
    inline bool operator == (OGLBlendCommand const& iOther) const { return !operator!=(iOther); }

    void Apply();

    uint16_t m_Flag = 0;
    uint16_t m_Padding = 0;
    OGLBlend srcRGB;
    OGLBlend dstRGB;
    OGLBlend srcAlpha;
    OGLBlend dstAlpha;
  };

  inline size_t hash_value(OGLScissorCommand const& iCmd)
  {
    size_t seed = iCmd.m_Flag;
    boost::hash_combine(seed, iCmd.m_ScissorCoord);

    return seed;
  }

  inline size_t hash_value(OGLDepthCommand const& iCmd)
  {
    size_t seed = iCmd.m_Flag;

    return seed;
  }

  inline size_t hash_value(OGLViewportCommand const& iCmd)
  {
    size_t seed = iCmd.m_Flag;
    boost::hash_combine(seed, iCmd.m_Orig);
    boost::hash_combine(seed, iCmd.m_Size);
    return seed;
  }

  inline size_t hash_value(OGLBlendCommand const& iCmd)
  {
    size_t seed = iCmd.m_Flag;
    boost::hash_combine(seed, (uint32_t)iCmd.srcRGB);
    boost::hash_combine(seed, (uint32_t)iCmd.dstRGB);
    boost::hash_combine(seed, (uint32_t)iCmd.srcAlpha);
    boost::hash_combine(seed, (uint32_t)iCmd.dstAlpha);
    return seed;
  }

  struct OGLVAssembly
  {
    struct VtxAttrib
    {
      OGLBuffer const* m_VBuffer;
      uint32_t m_AttribId;
      uint32_t m_Num;
      uint32_t m_Stride;
      uint32_t m_Offset;
    };
    std::vector<VtxAttrib> m_Attribs;

    void AddAttrib(OGLBuffer const* iBuffer, unsigned int iAttribId, unsigned int iNum, unsigned int iStride, unsigned int iOffset)
    {
      VtxAttrib newAttrib = {iBuffer,iAttribId,iNum,iStride,iOffset};
      m_Attribs.push_back(newAttrib);
    }

    void AddAttrib(IntrusivePtr<OGLBuffer const> const& iBuffer, unsigned int iAttribId, unsigned int iNum, unsigned int iStride, unsigned int iOffset)
    { AddAttrib(iBuffer.get(), iAttribId, iNum, iStride, iOffset); }

    void AddAttrib(IntrusivePtr<OGLBuffer> const& iBuffer, unsigned int iAttribId, unsigned int iNum, unsigned int iStride, unsigned int iOffset)
    { AddAttrib(iBuffer.get(), iAttribId, iNum, iStride, iOffset); }

    OGLBuffer const* m_IBuffer;
    unsigned int m_IOffset;

    void Apply(OGLRenderContext* ) const;
  };

  struct OGLShaderDataSet
  {
    OGLShaderDataSet(OGLShaderData const* iAdditionalData, uint32_t iPrevSet, uint32_t iId)
      : additionalData(iAdditionalData)
      , prevSet(iPrevSet)
      , m_Id(iId)
    {}

    inline bool operator ==(OGLShaderDataSet const& iOther) const
    {
      return prevSet == iOther.prevSet && additionalData == iOther.additionalData;
    }

    OGLShaderData const* additionalData;
    uint32_t prevSet = -1;
    uint32_t m_Id = 0;
  };

  inline size_t hash_value(OGLShaderDataSet const& Set)
  {
    size_t value = (ptrdiff_t)Set.additionalData;
    if (Set.prevSet)
    {
      boost::hash_combine(value, Set.prevSet);
    }
    return value;
  }

  struct OGLDraw
  {
    enum Command
    {
      Draw = 0<<3,
      Clear = 1<<3,

      MaskDraw = 24,
      ShiftDraw = 3,

      DrawGroup = 1<<6,

      Point =         0/*<<4*/,
      LineList =      1/*<<4*/,
      LineStrip =     2/*<<4*/,
      TriangleList =  3/*<<4*/,
      TriangleStrip = 4/*<<4*///,

      //MaskTopo = 7<<4,
      //ShiftTopo = 4
    };

    uint16_t m_Flags;
    uint8_t m_StateId;
    uint8_t m_Topo;
    //1 char avail (5 in x64)
    OGLVAssembly const* m_VDecl;
    OGLCompiledTechnique const* m_Prog;
  };

  struct OGLGeometry
  {
    uint32_t m_Mat;
    uint32_t m_Offset;
    uint32_t m_Num;
  };

  struct OGLClear
  {
    enum
    {
      Color = 1<<6,
      Depth = 2<<6,

      Mask = 128 + 64,
      Shift = 6
    };
    uint16_t m_Flags;
    uint8_t m_StateId;
    uint8_t m_Padding;

    Vector4f m_ClearColor;
    float m_ClearDepth;
  };

}