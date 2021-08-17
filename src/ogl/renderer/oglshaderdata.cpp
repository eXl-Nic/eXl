#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <core/type/tupletype.hpp>

namespace eXl
{

  OGLShaderData::TextureData::~TextureData()
  {
  }

  OGLShaderData::~OGLShaderData()
  {
    //for(unsigned int i = 0; i<m_Data.size(); ++i)
    //{
    //  ShaderData& data = m_Data[i];
    //
    //  TupleType const* iDataType = OGLSemanticManager::GetData(data.m_DataSlot);
    //  iDataType->Destroy(data.m_Data);
    //}
  }

  void OGLShaderData::AddData(unsigned int iSlot, void const* iData, unsigned int iFlags)
  {
    TupleType const* iDataType = OGLSemanticManager::GetData(iSlot);

    eXl_ASSERT_REPAIR_RET(iDataType != nullptr, );

    for(unsigned int i = 0; i<m_Data.size(); ++i)
    {
      if(m_Data[i].m_DataSlot == iSlot)
      {
        m_Data[i].m_DataFlags = iFlags;
        //void* dest = m_Data[i].m_Data;
        //iDataType->Copy(iData,dest);
        m_Data[i].m_Data = iData;
        return;
      }
    }

    m_Data.push_back(ShaderData());
    m_Data.back().m_DataSlot = iSlot;
    m_Data.back().m_DataFlags = iFlags;
    m_Data.back().m_Data = iData;

    //iDataType->Copy(iData,m_Data.back().m_Data);
  }

  void OGLShaderData::AddTexture(unsigned int iSlot, OGLTexture const* iTexture)
  {
    for(unsigned int i = 0; i<m_TexData.size(); ++i)
    {
      if(m_TexData[i].m_TextureSlot == iSlot)
      {
        m_TexData[i].m_Texture = iTexture;
      }
    }

    m_TexData.push_back(TextureData());
    m_TexData.back().m_TextureSlot = iSlot;
    m_TexData.back().m_Texture = iTexture;
  }
}