/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/ogltechnique.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <core/type/typemanager.hpp>
#include <math/math.hpp>
#include <core/type/fixedlengtharray.hpp>
#include <core/type/dynobject.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltypesconv.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>

namespace eXl
{
  namespace
  {
    void SetV1F(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      //LOG_INFO << "Set float at " << iLoc << ", " << ((float*)iData)[0];
      glUniform1fv(iLoc,iCount,(GLfloat const*)iData);
    }

    void SetV2F(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      float* fltData = ((float*)iData);
      //LOG_INFO << "Set vec2 at " << iLoc << ", " << fltData[0] << ", " << fltData[1];
      glUniform2fv(iLoc,iCount,(GLfloat const*)iData);
    }

    void SetV3F(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      float* fltData = ((float*)iData);
      //LOG_INFO << "Set vec3 at " << iLoc << ", " << fltData[0] << ", " << fltData[1] << ", " << fltData[2];
      glUniform3fv(iLoc,iCount,(GLfloat const*)iData);
    }

    void SetV4F(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      float* fltData = ((float*)iData);
      //LOG_INFO << "Set vec4 at " << iLoc << ", " << fltData[0] << ", " << fltData[1] << ", " << fltData[2] << ", " << fltData[3];
      glUniform4fv(iLoc,iCount,(GLfloat const*)iData);
    }

    void SetV1I(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      //LOG_INFO << "Set int at " << iLoc << ", " << ((int*)iData)[0];
      glUniform1iv(iLoc,iCount,(GLint const*)iData);
    }

    void SetV2I(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      glUniform2iv(iLoc,iCount,(GLint const*)iData);
    }

    void SetV3I(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      glUniform3iv(iLoc,iCount,(GLint const*)iData);
    }

    void SetV4I(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      glUniform4iv(iLoc,iCount,(GLint const*)iData);
    }

    void SetM4F(uint32_t iLoc, uint32_t iCount, void const* iData)
    {
      glUniformMatrix4fv(iLoc,iCount,GL_FALSE,(GLfloat const*)iData);
    }

    struct ExceptionList
    {
      void Init()
      {
        m_ListingException.insert(TypeManager::GetType<Vector4f>());
        m_ListingException.insert(TypeManager::GetType<Vector3f>());
        m_ListingException.insert(TypeManager::GetType<Vector2f>());
        m_ListingException.insert(TypeManager::GetType<Vector4i>());
        m_ListingException.insert(TypeManager::GetType<Vector3i>());
        m_ListingException.insert(TypeManager::GetType<Vector2i>());
      }

      bool IsException(Type const* iType)
      {
        return m_ListingException.find(iType) != m_ListingException.end();
      }

      boost::container::flat_set<Type const*> m_ListingException;
    };

    struct FieldTypeMap
    {
      //struct FieldType
      //{
      //  inline FieldType(GLenum iType):type(iType){}
      //  GLenum type;
      //  //uint32_t num;
      //};

      void Init()
      {
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector4f>(), GLenum(GL_FLOAT_VEC4)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector3f>(), GLenum(GL_FLOAT_VEC3)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector2f>(), GLenum(GL_FLOAT_VEC2)));
        m_Map.insert(std::make_pair(TypeManager::GetType<float>(),    GLenum(GL_FLOAT)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector4i>(), GLenum(GL_INT_VEC4)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector3i>(), GLenum(GL_INT_VEC3)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Vector2i>(), GLenum(GL_INT_VEC2)));
        m_Map.insert(std::make_pair(TypeManager::GetType<int>(),      GLenum(GL_INT)));
        m_Map.insert(std::make_pair(TypeManager::GetType<Matrix4f>(), GLenum(GL_FLOAT_MAT4)));
      }

      void GetType(Type const* iType, GLenum& oType)
      {
        boost::container::flat_map<Type const*, GLenum>::iterator iter = m_Map.find(iType);
        eXl_ASSERT_MSG(iter != m_Map.end(), "Missing type info");
        oType = iter->second;
      }

      boost::container::flat_map<Type const*, GLenum> m_Map;
    };

    struct SetterMap
    {
      SetterMap()
      {
        m_Map.insert(std::make_pair(GLenum(GL_FLOAT),&SetV1F));
        m_Map.insert(std::make_pair(GLenum(GL_FLOAT_VEC2),&SetV2F));
        m_Map.insert(std::make_pair(GLenum(GL_FLOAT_VEC3),&SetV3F));
        m_Map.insert(std::make_pair(GLenum(GL_FLOAT_VEC4),&SetV4F));

        m_Map.insert(std::make_pair(GLenum(GL_INT),&SetV1I));
        m_Map.insert(std::make_pair(GLenum(GL_INT_VEC2),&SetV2I));
        m_Map.insert(std::make_pair(GLenum(GL_INT_VEC3),&SetV3I));
        m_Map.insert(std::make_pair(GLenum(GL_INT_VEC4),&SetV4I));

        m_Map.insert(std::make_pair(GLenum(GL_FLOAT_MAT4),&SetM4F));
      }

      OGLDataHandlerProgData::SetValuefptr FindSetter(GLenum iType)
      {
        boost::container::flat_map<GLenum, OGLDataHandlerProgData::SetValuefptr>:: iterator iter = m_Map.find(iType);
        if (iter != m_Map.end())
        {
          return iter->second;
        }
        return nullptr;
      }

      boost::container::flat_map<GLenum, OGLDataHandlerProgData::SetValuefptr> m_Map;
    };

    ExceptionList s_Exception;
    FieldTypeMap  s_TypeMap;
    SetterMap     s_SetterMap;

    struct FieldsList
    {
      TypeFieldName m_Name;
      Type const* m_Type;
      uint32_t m_Offset;
      uint32_t m_Multi;
    };

    void ListLeavesField(TupleType const* iType, std::vector<FieldsList>& oList)
    {
      size_t numFields = iType->GetNumField();
      for(uint32_t i = 0; i<numFields; ++i)
      {
        TypeFieldName name;
        Type const* fieldType = iType->GetFieldDetails(i,name);
        if(fieldType != nullptr)
        {
          uint32_t oOffset;
          Err err = iType->ResolveFieldPath(name,oOffset,fieldType);
          if(fieldType->IsTuple() && !s_Exception.IsException(fieldType))
          {
            FixedLengthArray const* arrayType = FixedLengthArray::DynamicCast(fieldType);

            if(arrayType != nullptr)
            {
              Type const* fieldType = arrayType->GetArrayType();
              if(s_Exception.IsException(fieldType))
              {
                oList.push_back(FieldsList());
                oList.back().m_Multi = arrayType->GetNumField();
                oList.back().m_Name = name+"[0]";
                oList.back().m_Offset = oOffset;
                oList.back().m_Type = fieldType;
                //Next field
                continue;
              }
              else
              {

              }
            }

            uint32_t curPos = oList.size();
            ListLeavesField(fieldType->IsTuple(),oList);
            for(uint32_t j = curPos;j<oList.size();++j)
            {
              oList[j].m_Name = name + "." + oList[j].m_Name;
              oList[j].m_Offset += oOffset;
            }
          }
          else
          {
            oList.push_back(FieldsList());
            oList.back().m_Multi = 1;
            oList.back().m_Name = name;
            oList.back().m_Offset = oOffset;
            oList.back().m_Type = fieldType;
          }
        }
      }
    }
  }

  void OGLTechnique::InitStaticData()
  {
    s_Exception.Init();
    s_TypeMap.Init();
  }

  OGLTechnique::OGLTechnique()
  {

  }

  void OGLTechnique::AddAttrib(uint32_t iAttribName)
  {
    m_AttribNames.push_back(iAttribName);
  }

  void OGLTechnique::AddUniform(uint32_t iDataName)
  {
    m_UnifNames.push_back(iDataName);
  }

  void OGLTechnique::AddTexture(uint32_t iTexName)
  {
    m_Textures.push_back(iTexName);
  }
  
  OGLCompiledTechnique* OGLTechnique::Compile(OGLProgram const* iProg)
  { 
    if(iProg == nullptr)
      return nullptr;

    OGLCompiledTechnique tempTech;

    for(uint32_t i = 0; i<m_AttribNames.size(); ++i)
    {
      OGLAttribDesc const& attrib = OGLSemanticManager::GetAttrib(m_AttribNames[i]);
      eXl_ASSERT_MSG(!attrib.m_Name.empty(),"Wrong attrib name");
      {
        int loc = iProg->GetAttribLocation(attrib.m_Name);
        if(loc > -1)
        {
          OGLType oType;
          uint32_t oNum;
          iProg->GetAttribDescription(attrib.m_Name,oType,oNum);
                                            //Pas nécessaire selon OGL
          //eXl_ASSERT_MSG(oType == attrib.m_Type /*&& oNum == attrib.m_Mult*/,"Incompatible layout");
          if(oType == attrib.m_Type && tempTech.m_MaxAttrib < 16)
          {
            tempTech.m_AttribSlot[tempTech.m_MaxAttrib] = m_AttribNames[i];
            tempTech.m_AttribDesc[tempTech.m_MaxAttrib] = std::make_pair(loc,oType);
            tempTech.m_MaxAttrib++;
          }
        }
      }
    }

    OGLDataHandlerProgData* progData = nullptr;

    std::vector<FieldsList> oList;

    for(uint32_t i = 0; i<m_UnifNames.size(); ++i)
    {
      oList.clear();

      TupleType const* dataType = OGLSemanticManager::GetDataType(m_UnifNames[i]);
      eXl_ASSERT_MSG(dataType != nullptr,"Wrong data name");

      AString const* dataName = OGLSemanticManager::GetDataName(m_UnifNames[i]);
      eXl_ASSERT(dataName != nullptr);

      uint32_t unifSize;
      if (iProg->GetUniformBlockDescription(*dataName, unifSize))
      {
        uint32_t currentSlot = tempTech.m_MaxUnifBlock;
        eXl_ASSERT(unifSize == dataType->GetSize());

        tempTech.m_TechData.m_BlockHandler[currentSlot] = std::make_pair(iProg->GetUniformBlockLocation(*dataName), unifSize);

        tempTech.m_UnifBlockSlot[currentSlot] = m_UnifNames[i];
        ++tempTech.m_MaxUnifBlock;
        continue;
      }
      
      uint32_t currentSlot = tempTech.m_MaxUnif;
      if(tempTech.m_MaxUnif < 16)
      {
        ListLeavesField(dataType, oList);

        for(uint32_t j = 0; j < oList.size(); ++j)
        {
          AString unifName = StringUtil::ToASCII(oList[j].m_Name);
          int loc = iProg->GetUniformLocation(unifName);
          //eXl_ASSERT_MSG(loc != -1, "Missing location");
          //LOG_INFO << "Uniform " << unifName << " to loc " << loc << "\n";
          if(loc != -1)
          {
            OGLType progType;
            uint32_t progNum;
            iProg->GetUniformDescription(unifName,progType,progNum);
            GLenum oType;
            s_TypeMap.GetType(oList[j].m_Type,oType);

            if(GetGLType(progType) == oType && progNum == oList[j].m_Multi)
            {
              OGLDataHandlerProgData::SetValuefptr setter = s_SetterMap.FindSetter(oType);
              OGLDataHandlerProgData::DataSetter setterStruct;
              setterStruct.count = oList[j].m_Multi;
              setterStruct.location = loc;
              setterStruct.offset = oList[j].m_Offset;
              setterStruct.setter = setter;
              tempTech.m_TechData.m_UnifHandler[currentSlot].push_back(setterStruct);
            }
          }
        }
        if(tempTech.m_TechData.m_UnifHandler[currentSlot].size() > 0)
        {
          tempTech.m_UnifSlot[currentSlot] = m_UnifNames[i];
          tempTech.m_MaxUnif++;
        }
      }
    }

    for(uint32_t i = 0; i<m_Textures.size(); ++i)
    {
      if(tempTech.m_MaxTexture < 16)
      {
        uint32_t currentTexSlot = tempTech.m_MaxTexture;
        OGLSamplerDesc const& sampler = OGLSemanticManager::GetSampler(m_Textures[i]);
        if(!sampler.name.empty())
        {
          int texLoc = iProg->GetUniformLocation(sampler.name);
          if(texLoc > 0)
          {
            OGLType oType;
            uint32_t oNum;
            iProg->GetUniformDescription(sampler.name,oType,oNum);

            if(oType == OGLType::SAMPLER_2D || oType == OGLType::SAMPLER_CUBE)
            {
              tempTech.m_TechData.m_Samplers[currentTexSlot].first = texLoc;
              tempTech.m_TechData.m_Samplers[currentTexSlot].second.maxFilter = sampler.maxFilter;
              tempTech.m_TechData.m_Samplers[currentTexSlot].second.minFilter = sampler.minFilter;
              tempTech.m_TechData.m_Samplers[currentTexSlot].second.wrapX = sampler.wrapX;
              tempTech.m_TechData.m_Samplers[currentTexSlot].second.wrapY = sampler.wrapY;
              tempTech.m_TexSlot[currentTexSlot] = m_Textures[i];
              tempTech.m_MaxTexture++;
            }
            else
            {
              LOG_WARNING<<"Wrong type for texture."<<"\n";
            }
          }
        }
      }
    }

    OGLCompiledTechnique* newTech = eXl_NEW OGLCompiledTechnique;

    newTech->m_Program = iProg;

    newTech->m_MaxAttrib = tempTech.m_MaxAttrib;
    newTech->m_MaxUnif = tempTech.m_MaxUnif;
    newTech->m_MaxUnifBlock = tempTech.m_MaxUnifBlock;
    newTech->m_MaxTexture = tempTech.m_MaxTexture;
    
    memcpy(&newTech->m_AttribDesc,&tempTech.m_AttribDesc,tempTech.m_MaxAttrib*sizeof(std::pair<uint32_t, GLenum>));
    memcpy(&newTech->m_AttribSlot,&tempTech.m_AttribSlot,tempTech.m_MaxAttrib*sizeof(uint32_t));

    memcpy(&newTech->m_TexSlot,&tempTech.m_TexSlot,tempTech.m_MaxTexture*sizeof(uint32_t));
    for(uint32_t i = 0; i<tempTech.m_MaxTexture; ++i)
    {
      newTech->m_TechData.m_Samplers[i].first = tempTech.m_TechData.m_Samplers[i].first;
      newTech->m_TechData.m_Samplers[i].second.maxFilter = tempTech.m_TechData.m_Samplers[i].second.maxFilter;
      newTech->m_TechData.m_Samplers[i].second.minFilter = tempTech.m_TechData.m_Samplers[i].second.minFilter;
      newTech->m_TechData.m_Samplers[i].second.wrapX = tempTech.m_TechData.m_Samplers[i].second.wrapX;
      newTech->m_TechData.m_Samplers[i].second.wrapY = tempTech.m_TechData.m_Samplers[i].second.wrapY;
    }
    //memcpy(&newTech->m_TechData.m_Samplers,&tempTech.m_TechData.m_Samplers,tempTech.m_MaxTexture*sizeof(std::pair<int, OGLSamplerDesc>));

    memcpy(&newTech->m_UnifSlot,&tempTech.m_UnifSlot,tempTech.m_MaxUnif*sizeof(uint32_t));
    
    for(uint32_t i = 0; i < tempTech.m_MaxUnif; ++i)
    {
      newTech->m_TechData.m_UnifHandler[i].swap(tempTech.m_TechData.m_UnifHandler[i]);
    }

    memcpy(&newTech->m_UnifBlockSlot, &tempTech.m_UnifBlockSlot, tempTech.m_MaxUnifBlock * sizeof(uint32_t));

    for (uint32_t i = 0; i < tempTech.m_MaxUnifBlock; ++i)
    {
      newTech->m_TechData.m_BlockHandler[i] = tempTech.m_TechData.m_BlockHandler[i];
    }

    return newTech;
  }

  OGLCompiledTechnique::OGLCompiledTechnique()
  {
  }

  void OGLCompiledTechnique::HandleAttribute(uint32_t iAttribSlot, uint32_t iNum, size_t iStride, size_t iOffset)const
  {
    glVertexAttribPointer(m_AttribDesc[iAttribSlot].first,iNum, GetGLType(m_AttribDesc[iAttribSlot].second),false,iStride,(void*)iOffset);
  }

  void OGLCompiledTechnique::HandleTexture(uint32_t iTexSlot, OGLTexture const* iTexture)const
  {
    glActiveTexture(GL_TEXTURE0 + iTexSlot);

    GLenum textureTarget = iTexture->IsCubeMap() ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    glBindTexture(textureTarget,iTexture->GetId());
    //LOG_INFO << "Set texture at " << iTexSlot << ", " << iTexture;
    glUniform1i(m_TechData.m_Samplers[iTexSlot].first,iTexSlot);

    glTexParameteri(textureTarget,GL_TEXTURE_MIN_FILTER, GetGLMinFilter(m_TechData.m_Samplers[iTexSlot].second.minFilter));
    glTexParameteri(textureTarget,GL_TEXTURE_MAG_FILTER, GetGLMagFilter(m_TechData.m_Samplers[iTexSlot].second.maxFilter));
    glTexParameteri(textureTarget,GL_TEXTURE_WRAP_S,     GetGLWrapMode(m_TechData.m_Samplers[iTexSlot].second.wrapX));
    glTexParameteri(textureTarget,GL_TEXTURE_WRAP_T,     GetGLWrapMode(m_TechData.m_Samplers[iTexSlot].second.wrapY));
  }

  void OGLCompiledTechnique::HandleUniform(uint32_t iSlot, void const* iData) const
  {
    uint32_t numFields = m_TechData.m_UnifHandler[iSlot].size();
    uint8_t const* iterData = reinterpret_cast<uint8_t const*>(iData);
    OGLDataHandlerProgData::DataSetter const* setter = &m_TechData.m_UnifHandler[iSlot][0];
    for(uint32_t i = 0; i< numFields; ++i)
    {
      setter->setter(setter->location, setter->count, iterData + setter->offset);
      setter++;
    }
  }

  void OGLCompiledTechnique::HandleUniformBlock(uint32_t iSlot, uint32_t iBuffer) const
  {
    //glBindBufferRange(GL_UNIFORM_BUFFER, m_TechData.m_BlockHandler[iSlot].first, iBuffer, 0, m_TechData.m_BlockHandler[iSlot].second);
    glBindBufferBase(GL_UNIFORM_BUFFER, m_TechData.m_BlockHandler[iSlot].first, iBuffer);
  }

  uint32_t OGLCompiledTechnique::GetAttribLocation(uint32_t iAttribSlot) const
  {
    return m_AttribDesc[iAttribSlot].first;
  }

  void OGLCompiledTechnique::Setup()const
  {
    
  }
}
