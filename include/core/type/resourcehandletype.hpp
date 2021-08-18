/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/coretype.hpp>
#include <core/resource/resource.hpp>

namespace eXl
{
  class ResourceLoader;
  namespace ResourceManager
  {
    EXL_CORE_API void AddLoader(ResourceLoader* , Rtti const& );
  }

  class EXL_CORE_API ResourceHandleType : public CoreType
  {
    friend void ResourceManager::AddLoader(ResourceLoader*, Rtti const&);
    DECLARE_RTTI(ResourceHandleType, Type);
  public:

    virtual void* Alloc()const;

    virtual void Free(void* iObj)const;

    virtual void* Construct(void* iObj)const;

    virtual void Destruct(void* iObj)const;

    virtual Err Copy_Uninit(void const* iData, void* oData) const;

    virtual Err Stream(void const* iData, Streamer* iStreamer) const;

    virtual Err Unstream_Uninit(void* oData, Unstreamer* iStreamer) const;
#ifdef EXL_LUA
    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const;

    luabind::object ConvertToLua(void const* iObj,lua_State* iState)const;
#endif
    Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const;

    virtual bool CanAssignFrom(Type const* iOtherType) const;

    virtual Err Assign_Uninit(Type const* inputType, void const* iData, void* oData) const;

    inline Rtti const& GetMinimalRtti() const {return m_Rtti;}

    Resource::UUID const& GetUUID(void const* iData) const;
    void SetUUID(void* iData, Resource::UUID const&) const;

  protected:
    ResourceHandleType();
    ResourceHandleType(Rtti const& iRtti);

    Rtti const& m_Rtti;
  };
}
