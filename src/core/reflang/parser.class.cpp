#include "parser.class.hpp"

#include "parser.util.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace
    {
      Function GetMethodFromCursor(CXCursor cursor)
      {
        auto type = clang_getCursorType(cursor);

        Function f(parser::GetFile(cursor), parser::GetFullName(cursor));
        f.m_Name = parser::Convert(clang_getCursorSpelling(cursor));
        int num_args = clang_Cursor_getNumArguments(cursor);
        for (int i = 0; i < num_args; ++i)
        {
          auto arg_cursor = clang_Cursor_getArgument(cursor, i);
          NamedObject arg;
          arg.name = parser::Convert(clang_getCursorSpelling(arg_cursor));
          if (arg.name.empty())
          {
            arg.name = "nameless";
          }
          auto arg_type = clang_getArgType(type, i);
          arg.type = parser::GetName(arg_type);
          f.m_Arguments.push_back(arg);
        }

        f.m_ReturnType = parser::GetName(clang_getResultType(type));
        return f;
      }

      NamedObject GetFieldFromCursor(CXCursor cursor)
      {
        NamedObject field;
        field.name = parser::Convert(clang_getCursorSpelling(cursor));
        field.type = parser::GetName(clang_getCursorType(cursor));
        return field;
      }

      CXChildVisitResult VisitClass(
        CXCursor cursor, CXCursor parent, CXClientData client_data)
      {
        auto* c = reinterpret_cast<Class*>(client_data);
        if (clang_getCXXAccessSpecifier(cursor) == CX_CXXPublic)
        {
          switch (clang_getCursorKind(cursor))
          {
          case CXCursor_CXXMethod:
            if (clang_CXXMethod_isStatic(cursor) != 0)
            {
              c->m_StaticMethods.push_back(GetMethodFromCursor(cursor));
            }
            else
            {
              c->m_Methods.push_back(GetMethodFromCursor(cursor));
            }
            break;
          case CXCursor_FieldDecl:
            c->m_Fields.push_back(GetFieldFromCursor(cursor));
            break;
          case CXCursor_VarDecl:
            c->m_StaticFields.push_back(GetFieldFromCursor(cursor));
            break;
          default:
            break;
          }
        }
        return CXChildVisit_Continue;
      }
    }

    Class parser::GetClass(CXCursor cursor)
    {
      Class c(GetFile(cursor), GetFullName(cursor));
      clang_visitChildren(cursor, VisitClass, &c);
      return c;
    }
  }
}

