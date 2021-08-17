#include "parser.enum.hpp"

#include "parser.util.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace
    {
      CXChildVisitResult VisitEnum(
        CXCursor cursor, CXCursor parent, CXClientData client_data)
      {
        if (clang_getCursorKind(cursor) == CXCursor_EnumConstantDecl)
        {
          String name = parser::Convert(clang_getCursorSpelling(cursor));
          int value = static_cast<int>(clang_getEnumConstantDeclValue(cursor));
          reinterpret_cast<Enum::ValueList*>(client_data)->emplace_back(
            name, value);
        }
        return CXChildVisit_Continue;
      }

      Enum::ValueList GetEnumValues(const CXCursor& cursor)
      {
        Enum::ValueList result;

        clang_visitChildren(cursor, VisitEnum, &result);

        return result;
      }
    }

    Enum parser::GetEnum(CXCursor cursor)
    {
      Enum e(GetFile(cursor), GetFullName(cursor));
      e.m_Values = GetEnumValues(cursor);
      return e;
    }

  }
}
