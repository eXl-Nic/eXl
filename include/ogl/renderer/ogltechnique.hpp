/**

  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <core/heapobject.hpp>
#include <vector>
#include <ogl/oglexp.hpp>
#include <ogl/renderer/oglsemanticmanager.hpp>

namespace eXl
{
  class OGLTexture;
  class OGLProgram;
  class OGLCompiledTechnique;

  class OGLDataHandlerProgData : public HeapObject
  {
  public:
    typedef void (*SetValuefptr)(unsigned int ,unsigned int ,void const*);
    struct DataSetter
    {
      SetValuefptr setter;
      unsigned int location;
      unsigned int count;
      unsigned int offset;
    };

    std::vector<DataSetter>         m_UnifHandler[16];
    std::pair<int, OGLSamplerDesc>  m_Samplers[4];
  };

  class OGLDataHandlerOverride : public HeapObject
  {
  public:

    virtual void OnSetup(OGLDataHandlerProgData*) const{}

    virtual OGLDataHandlerProgData* CreateData() const {return NULL;}

    virtual bool DataOverriden(unsigned int iDataName, OGLProgram const* iProg, OGLDataHandlerProgData* iData) const{return false;}
    virtual bool TextureOverriden(unsigned int iTexName, OGLProgram const* iProg, OGLDataHandlerProgData* iData) const{return false;}
    virtual void HandleUniform(unsigned int iDataName, unsigned int iDataSlot, OGLDataHandlerProgData* iHandlerData, void const* iData) const{}
    virtual void HandleTexture(unsigned int iTexName, unsigned int iDataSlot, OGLDataHandlerProgData* iHandlerData, void const* iData) const{}
  };

  class EXL_OGL_API OGLTechnique : public HeapObject
  {
  public:

    OGLTechnique();

    static void InitStaticData();

    inline void SetOverrideHandler(OGLDataHandlerOverride const* iOverride){m_Override = iOverride;}
    
    void AddAttrib(unsigned int iAttribName);
    void AddUniform(unsigned int iDataName);
    void AddTexture(unsigned int iTexName);

    OGLCompiledTechnique* Compile (OGLProgram const* iProg);

  protected:
   
    std::vector<unsigned int>     m_AttribNames;
    std::vector<unsigned int>     m_UnifNames;
    std::vector<unsigned int>     m_Textures;
    OGLDataHandlerOverride const* m_Override;
  };

  class OGLCompiledTechnique : public HeapObject
  {
    friend OGLTechnique;
  public:

    void Setup() const;

    void HandleAttribute(unsigned int iAttribSlot, unsigned int iNum, size_t iStride, size_t iOffset) const;

    void HandleUniform(unsigned int iSlot, void const* iData) const;

    void HandleTexture(unsigned int iTexSlot, OGLTexture const* iTexture) const;

    unsigned int GetAttribLocation(unsigned int iAttribSlot) const;

    inline unsigned int const* GetAttribSlots() const{return m_AttribSlot;}
    inline unsigned int        GetMaxAttrib() const{return m_MaxAttrib;}

    inline unsigned int const* GetUniformSlots() const{return m_UnifSlot;}
    inline unsigned int        GetMaxUniform() const{return m_MaxUnif;}

    inline unsigned int const* GetTextureSlots() const{return m_TexSlot;}
    inline unsigned int        GetMaxTexture() const{return m_MaxTexture;}


    inline OGLProgram const* GetProgram() const{return m_Program;}

  protected:

    OGLCompiledTechnique();

    OGLDataHandlerOverride const* m_Override;
    OGLDataHandlerProgData*       m_OverrideData;
    OGLProgram const*             m_Program;
    OGLDataHandlerProgData        m_TechData;

    std::pair<unsigned int, OGLType> m_AttribDesc[16];

    unsigned int m_AttribSlot[16];
    unsigned int m_UnifSlot[16];
    unsigned int m_TexSlot[4];

    unsigned int m_MaxAttrib;
    unsigned int m_MaxUnif;
    unsigned int m_MaxTexture;
  };
}
