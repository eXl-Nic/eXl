#include "types.hpp"

#include <core/type/typetraits.hpp>

namespace eXl
{
  namespace reflang
  {
    TypeBase::TypeBase(Path file, String full_name)
      : m_FullName(std::move(full_name))
      , m_File(std::move(file))
    {
    }

    TypeBase::~TypeBase() = default;

    const String& TypeBase::GetFullName() const
    {
      return m_FullName;
    }

    String TypeBase::GetShortName() const
    {
      size_t pos = m_FullName.rfind("::");
      if (pos != String::npos)
      {
        return m_FullName.substr(pos + 2);
      }
      return m_FullName;
    }

    const Path& TypeBase::GetFile() const
    {
      return m_File;
    }

    Enum::Enum(Path file, String full_name)
      : TypeBase(std::move(file), move(full_name))
    {
    }

    Enum::Type Enum::GetType() const
    {
      return Type::Enum;
    }

    Function::Function(Path file, String full_name)
      : TypeBase(std::move(file), move(full_name))
    {
    }

    Function::Type Function::GetType() const
    {
      return Type::Function;
    }

    Class::Class(Path file, String full_name)
      : TypeBase(std::move(file), move(full_name))
    {
    }

    Class::Type Class::GetType() const
    {
      return Type::Class;
    }

    bool Class::HasReflectionMarker() const
    {
      return std::find_if(m_StaticMethods.begin(), m_StaticMethods.end(), [](Function const& iFun)
      {
        return iFun.m_Name == EXL_REFLECTION_MARKER_STR; 
      }) != m_StaticMethods.end();
    }
  }
}