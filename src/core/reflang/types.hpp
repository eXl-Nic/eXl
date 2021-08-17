#pragma once

#include <core/string.hpp>
#include <core/containers.hpp>
#include <core/path.hpp>

namespace eXl
{
  namespace reflang
  {
    class TypeBase
    {
    public:
      enum class Type
      {
        Enum,
        Function,
        Class,
      };

    public:
      TypeBase(Path file, String full_name);
      virtual ~TypeBase();

      virtual Type GetType() const = 0;
      const String& GetFullName() const;
      String GetShortName() const;

      const Path& GetFile() const;

    private:
      Path m_File;
      String m_FullName;
    };

    class Enum : public TypeBase
    {
    public:
      using ValueList = Vector<std::pair<String, int>>;

    public:
      Enum(Path file, String full_name);
      Type GetType() const override;

      ValueList m_Values;
    };

    struct NamedObject
    {
      String name;
      String type;
    };

    class Function : public TypeBase
    {
    public:
      Function(Path file, String full_name);
      Type GetType() const override;

      String m_Name;
      Vector<NamedObject> m_Arguments;
      String m_ReturnType;
    };

    class Class : public TypeBase
    {
    public:
      using MethodList = Vector<Function>;
      using FieldList = Vector<NamedObject>;

    public:
      Class(Path file, String full_name);
      Type GetType() const override;

      bool HasReflectionMarker() const;

      MethodList m_Methods;
      MethodList m_StaticMethods;

      FieldList m_Fields;
      FieldList m_StaticFields;
      UnorderedSet<String> m_EnumFields;
    };
  }
}
