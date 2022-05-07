/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <core/type/tupletype.hpp>

#include <ogl/renderer/oglinclude.hpp>

namespace eXl
{

  OGLShaderData::TextureData::~TextureData()
  {
  }

  OGLShaderData::OGLShaderData() = default;
  OGLShaderData::OGLShaderData(OGLShaderData&&) = default;

  OGLShaderData::~OGLShaderData()
  {
    //for(uint32_t i = 0; i<m_Data.size(); ++i)
    //{
    //  ShaderData& data = m_Data[i];
    //
    //  TupleType const* iDataType = OGLSemanticManager::GetData(data.m_DataSlot);
    //  iDataType->Destroy(data.m_Data);
    //}
  }

  void OGLShaderData::AddData(UniformName iName, void const* iData)
  {
    for(uint32_t i = 0; i<m_Data.size(); ++i)
    {
      if(m_Data[i].m_Name == iName)
      {
        //void* dest = m_Data[i].m_Data;
        //iDataType->Copy(iData,dest);
        m_Data[i].m_Data = iData;
        return;
      }
    }

    m_Data.push_back(ShaderData());
    m_Data.back().m_Name = iName;
    m_Data.back().m_Data = iData;

    //iDataType->Copy(iData,m_Data.back().m_Data);
  }

  void OGLShaderData::SetDataBuffer(UniformName iName, OGLBuffer const* iBuffer)
  {
    m_UBOData.push_back(UBOData());
    m_UBOData.back().m_Name = iName;
    m_UBOData.back().m_DataBuffer = iBuffer;
  }

  void OGLShaderData::AddTexture(TextureName iName, OGLTexture const* iTexture)
  {
    for(uint32_t i = 0; i<m_TexData.size(); ++i)
    {
      if(m_TexData[i].m_Name == iName)
      {
        m_TexData[i].m_Texture = iTexture;
      }
    }

    m_TexData.push_back(TextureData());
    m_TexData.back().m_Name = iName;
    m_TexData.back().m_Texture = iTexture;
  }
}