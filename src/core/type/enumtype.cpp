/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <core/type/enumtype.hpp>
#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#include <core/type/arraytype.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>

#ifdef EXL_LUA

#include <core/lua/luaconverter.hpp>

#endif

namespace eXl
{

  IMPLEMENT_RTTI(EnumType)

  EnumType::EnumType(TypeName iName,size_t iTypeId)
     : Type(iName,iTypeId,sizeof(unsigned int),Type_Is_Enum)
  {

  }

  void* EnumType::Construct(void* iObj)const
  {
    return TypeTraits::DefaultCTor<unsigned int>(iObj);
  }

  void EnumType::Destruct(void* iObj)const
  {
    TypeTraits::DTor<unsigned int>(iObj);
  }

  Err EnumType::Copy_Uninit(void const* iData, void* oData) const
  {
    eXl_ASSERT_MSG(oData != nullptr && iData != nullptr,"Wrong parameters");
    void* res = TypeTraits::Copy<unsigned int>(oData,iData);
    if(res != nullptr)
      RETURN_SUCCESS;
    LOG_ERROR<<GetName()<<" cannot be built in place"<<"\n";
    RETURN_FAILURE;
  }
#ifdef EXL_LUA
  luabind::object EnumType::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    if(iObj == nullptr || iState == nullptr)
      return luabind::object();
    unsigned int val = *reinterpret_cast<unsigned int const*>(iObj);
    if(val < m_Enums.size())
    {
      return luabind::object(iState,val);
    }
    else
    {
      LOG_ERROR<<"Invalid enum value in "<<GetName()<<"\n";
      return luabind::object();
    }
  }

  Err EnumType::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    Err err = Err::Failure;
    if(iState!=nullptr && oObj != nullptr)
    {
     
      err = LuaConverter<unsigned int>::ConvertFromLua(this,oObj,iState,ioIndex);
      if(err)
      {
        unsigned int val = *reinterpret_cast<unsigned int*>(oObj);
        if(!(val < m_Enums.size()))
        {
          LOG_ERROR<<"Invalid enum value"<<"\n";
          err = Err::Failure;
        }
      }
    }
    return err;
  }
#endif
  Err EnumType::GetEnumValue(TypeEnumName iName,unsigned int& oVal)const
  {
    for(unsigned int i = 0;i<m_Enums.size();++i)
    {
      if(m_Enums[i] == iName)
      {
        oVal = i;
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err EnumType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    oRes = TypeTraits::Compare<unsigned int>(iVal1,iVal2);

    if(oRes != CompEqual)
      oRes = CompDifferent;

    RETURN_SUCCESS;
  }

  Err EnumType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const
  {
    String str;
    Err err = iUnstreamer->ReadString(&str);
    if(err)
    {
      TypeEnumName enumName(StringUtil::ToASCII(str));
      err = Err::Failure;
      for(unsigned int i = 0; i<m_Enums.size(); ++i)
      {
        if(m_Enums[i] == enumName)
        {
          *reinterpret_cast<unsigned int*>(oData) = i;
          err = Err::Success;
          break;
        }
      }
      if(!err)
      {
        LOG_ERROR<<"Could not find the enum value from its name"<<"\n";
      }
    }
    if(!err)
    {
      LOG_ERROR<<"Err while unstreaming "<<GetName() << "\n";
    }
    return err;
  }

  Err EnumType::Stream(void const* iData, Streamer* iStreamer) const
  {
    Err err = Err::Failure;
    if(iData && iStreamer)
    {
      unsigned int val = *reinterpret_cast<unsigned int const*>(iData);
      if(val < m_Enums.size())
      {
        err = iStreamer->WriteString(StringUtil::FromASCII(m_Enums[val]));
      }
      else
      {
        LOG_ERROR<<"Invalid enum value"<<"\n";
      }
    }
    if(!err)
    {
      LOG_ERROR<<"Err while streaming "<<GetName() << "\n";
    }
    return err;
  }

}
