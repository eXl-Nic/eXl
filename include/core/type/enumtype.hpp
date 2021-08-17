/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/type.hpp>
#include <core/rtti.hpp>

namespace eXl
{

  class EnumType;

  namespace TypeManager
  {
    namespace detail
    {
      EnumType const* _MakeEnumType(TypeName iName, Vector<TypeEnumName>& iVal);
    }
  }

  class EXL_CORE_API EnumType : public Type
  {
    DECLARE_RTTI(EnumType,Type);
    friend EnumType const* TypeManager::detail::_MakeEnumType(TypeName iName, Vector<TypeEnumName>& iVal);
  public:

    static ClassType const* StaticClassType();

    void* Construct(void* iObj) const override;

    void Destruct(void* iObj) const override;

    virtual Err Copy_Uninit(void const* iData, void* oData) const override;
#ifdef EXL_LUA
    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    luabind::object ConvertToLua(void const* iObj,lua_State* iState) const override;
#endif

    inline const unsigned int GetNumEnum() const 
    {
      return m_Enums.size();
    }

    inline bool GetEnumName(unsigned int iVal, TypeEnumName& oName) const
    {
      if(iVal < m_Enums.size())
      {
        oName = m_Enums[iVal];
        return true;
      }
      return false;
    }

    Err GetEnumValue (TypeEnumName iName, unsigned int& oVal) const;

    Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes) const override;

    Err Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const override;

    Err Stream(void const* iData, Streamer* iStreamer) const override;

  protected:

    EnumType(TypeName iName,size_t iTypeId);

    Vector<TypeEnumName> m_Enums;
  };

}