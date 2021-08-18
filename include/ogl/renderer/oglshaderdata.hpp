/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <ogl/oglexp.hpp>
#include <core/heapobject.hpp>
#include <core/intrusiveptr.hpp>
#include <core/type/dynobject.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <vector>

namespace eXl
{
  class OGLTexture;

  class EXL_OGL_API OGLShaderData : public HeapObject
  {
  public:

    struct ShaderData
    {
      void const*  m_Data;
      unsigned int m_DataSlot;
      unsigned int m_DataFlags;
    };

    class TextureData
    {
    public:
      ~TextureData();
      IntrusivePtr<OGLTexture const> m_Texture;
      unsigned int      m_TextureSlot;
    };

    ~OGLShaderData();

    void AddData(unsigned int iSlot, void const* iData, unsigned int iFlags = 0);

    void AddTexture(unsigned int iSlot, OGLTexture const* iTexture);
    void AddTexture(unsigned int iSlot, IntrusivePtr<OGLTexture const> const& iTexture) { AddTexture(iSlot, iTexture.get()); }
    void AddTexture(unsigned int iSlot, IntrusivePtr<OGLTexture> const& iTexture) { AddTexture(iSlot, iTexture.get()); }

    inline unsigned int GetNumData() const{return m_Data.size();}
    ShaderData const* GetDataDescPtr() const{return &m_Data[0];}

    inline void const* GetDataPtr(unsigned int iSlot)
    {
      for(unsigned int i = 0; i<m_Data.size(); ++i)
      {
        if(m_Data[i].m_DataSlot == iSlot)
        {
          return m_Data[i].m_Data;
        }
      }
      return nullptr;
    }

    template <class T>
    inline T const* CastBuffer(unsigned int iSlot) const
    {
      for(unsigned int i = 0; i<m_Data.size(); ++i)
      {
        if(m_Data[i].m_DataSlot == iSlot)
        {
          Type const* type = OGLSemanticManager::GetData(iSlot);
          ConstDynObject tempObj(type, m_Data[i].m_Data);
          return tempObj.CastBuffer<T>();
        }
      }
      return nullptr;
    }

    inline unsigned int GetNumTexture() const{return m_TexData.size();}
    TextureData const* GetTexturePtr() const{return &m_TexData[0];}

  protected:
    std::vector<ShaderData>  m_Data;
    std::vector<TextureData> m_TexData;
  };
}
