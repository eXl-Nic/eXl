#include <core/type/fundesc.hpp>

namespace eXl
{
  size_t ComputeSize(Vector<Type const*> const& iArgs)
  {
    size_t size = 0;
    for (auto type : iArgs)
    {
      size += type->GetSize();
    }
    return size;
  }

  ArgsBuffer::ArgsBuffer(Vector<Type const*> const& iArgs)
    : TupleType("", 0, ComputeSize(iArgs), Type_Is_POD)
    , m_Args(iArgs)
  {
    size_t offset = 0;
    for (auto type : iArgs)
    {
      m_Offsets.push_back(offset);
      offset += type->GetSize();
    }
  }

  size_t ArgsBuffer::GetNumField()const
  {
    return m_Args.size();
  }

  const Type* ArgsBuffer::GetFieldDetails(unsigned int iNum)const
  {
    if (iNum >= GetNumField())
    {
      return nullptr;
    }
    return m_Args[iNum];
  }

  namespace
  {
    bool isNumericChar(char x)
    {
      return (x >= '0' && x <= '9') ? true : false;
    }
    
    KString GetEndNumber(TypeFieldName const& iFieldName)
    {
      const char* beginStr = &(*iFieldName.begin());
      const char* endStr = &(*iFieldName.end());
      const char* beginNum = endStr;

      while (beginNum > beginStr && isNumericChar(*(beginNum - 1)))
      {
        --beginNum;
      }

      return KString(beginNum, endStr - beginNum);
    }
  }

  const Type* ArgsBuffer::GetFieldDetails(TypeFieldName iFieldName)const
  {
    KString numStr = GetEndNumber(iFieldName);
    if (numStr.empty())
    {
      return nullptr;
    }
    return GetFieldDetails(StringUtil::ToUInt(numStr));
  }

  const Type* ArgsBuffer::GetFieldDetails(unsigned int iNum, TypeFieldName& oFieldName)const
  {
    if (iNum >= GetNumField())
    {
      return nullptr;
    }
    oFieldName = "Arg" + StringUtil::FromInt(iNum);
    return m_Args[iNum];
  }

  const Type* ArgsBuffer::GetFieldDetails(TypeFieldName iFieldName, unsigned int& oNumField)const
  {
    KString numStr = GetEndNumber(iFieldName);
    if (numStr.empty())
    {
      return nullptr;
    }
    oNumField = StringUtil::ToUInt(numStr);
    return GetFieldDetails(oNumField);
  }

  Err ArgsBuffer::ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType)const
  {
    return Err::Undefined;
  }

  void* ArgsBuffer::GetField(void* iObj, unsigned int iNum, Type const*& oType)const
  {
    if (iNum >= GetNumField())
    {
      return nullptr;
    }

    oType = m_Args[iNum];
    return (uint8_t*)iObj + m_Offsets[iNum];
  }

  void* ArgsBuffer::GetField(void* iObj, TypeFieldName iName, Type const*& oType)const
  {
    KString numStr = GetEndNumber(iName);
    if (numStr.empty())
    {
      return nullptr;
    }
    return GetField(iObj, StringUtil::ToUInt(numStr), oType);
  }

  void const* ArgsBuffer::GetField(void const* iObj, unsigned int iNum, Type const*& oType)const
  {
    if (iNum >= GetNumField())
    {
      return nullptr;
    }
    oType = m_Args[iNum];
    return (uint8_t const*)iObj + m_Offsets[iNum];
  }

  void const* ArgsBuffer::GetField(void const* iObj, TypeFieldName iName, Type const*& oType)const
  {
    KString numStr = GetEndNumber(iName);
    if (numStr.empty())
    {
      return nullptr;
    }
    return GetField(iObj, StringUtil::ToUInt(numStr), oType);
  }

#ifdef EXL_LUA

  Err ArgsBuffer::ConvertFromLuaRaw_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj)const
  {
    return Err::Undefined;
  }

  luabind::object ArgsBuffer::ConvertToLua(void const* iObj, lua_State* iState) const
  {
    return luabind::object();
  }

  Err ArgsBuffer::ConvertFromLua_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj) const
  {
    return Err::Undefined;
  }

  void ArgsBuffer::RegisterLua(lua_State* iState) const
  {

  }
#endif
}