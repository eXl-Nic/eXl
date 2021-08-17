#include "parser.function.hpp"

#include "parser.util.hpp"

namespace eXl
{
  namespace reflang
  {
    Function parser::GetFunction(CXCursor cursor)
    {
      Function f(GetFile(cursor), GetFullName(cursor));
      auto type = clang_getCursorType(cursor);

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

  }
}
