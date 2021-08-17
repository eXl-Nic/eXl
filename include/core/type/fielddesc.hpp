#pragma once

#include <core/type/typedefs.hpp>

namespace eXl
{
  class Type;
  class FieldDesc
  {
  public:
    template <class Storage, class T, unsigned int I>
    static FieldDesc MakeField();

    template <class T, class U>
    static FieldDesc MakeField(TypeFieldName iName, U T::* iOffset);

    inline const size_t GetOffset()const { return m_Offset; }
    inline TypeFieldName GetName()const { return m_Name; }
    inline const Type* GetType()const { return m_Type; }

    inline FieldDesc() :m_Name(""), m_Offset(0), m_Type(0) {}
    inline FieldDesc(TypeFieldName iName, size_t iOffset, const Type* iType)
      : m_Name(iName), m_Offset(iOffset), m_Type(iType) {}
  private:
    TypeFieldName m_Name;
    size_t m_Offset;
    const Type* m_Type;
  };
}