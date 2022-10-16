#include "parser.util.hpp"

namespace eXl
{
  namespace reflang
  {
    String parser::Convert(const CXString& s)
    {
      String result = clang_getCString(s);
      clang_disposeString(s);
      return result;
    }

    Path parser::ToPath(const CXString& s)
    {
      Path result = clang_getCString(s);
      clang_disposeString(s);
      return result;
    }
    String parser::GetName(const CXType& type);

    String parser::GetFullName(CXCursor cursor)
    {
      CXType underlying = clang_getTypedefDeclUnderlyingType(cursor);
      if (underlying.kind != CXType_Invalid)
      {
        return GetName(underlying);
      }
      else
      {
        String name;
        while (clang_isDeclaration(clang_getCursorKind(cursor)) != 0)
        {
          String cur = Convert(clang_getCursorSpelling(cursor));
          if (name.empty())
          {
            name = cur;
          }
          else
          {
            name = cur + "::" + name;
          }
          cursor = clang_getCursorSemanticParent(cursor);
        }

        return name;
      }
    }

    String parser::GetName(const CXType& type)
    {
      int numTArgs = clang_Type_getNumTemplateArguments(type);
      String fullTemplateName = parser::Convert(clang_getTypeSpelling(type));
      size_t templateNameEnd = fullTemplateName.find("<");
      if (templateNameEnd == String::npos)
      {
        numTArgs = -1;
      }
      if (numTArgs != -1)
      {
        // How to get the template type's fully qualified name?
        // What about typedef?
        String fullTemplateName = parser::Convert(clang_getTypeSpelling(type));
        {
          String fullyQualName = fullTemplateName.substr(0, templateNameEnd + 1);

          for (uint32_t i = 0; i < numTArgs; ++i)
          {
            CXType templateArg = clang_Type_getTemplateArgumentAsType(type, i);
            String templateArgStr = GetName(templateArg);
            fullyQualName.append(templateArgStr);
            if (i < numTArgs - 1)
            {
              fullyQualName.append(", ");
            }
          }
          fullyQualName.append(">");
          return fullyQualName;
        }
      }

      CXCursor decl = clang_getTypeDeclaration(type);
      if (decl.kind != CXCursor_NoDeclFound)
      {
        return GetFullName(decl);
      }
      
      return Convert(clang_getTypeSpelling(type));
    }

    Path parser::GetFile(const CXCursor& cursor)
    {
      auto location = clang_getCursorLocation(cursor);
      CXFile file;
      clang_getSpellingLocation(location, &file, nullptr, nullptr, nullptr);
      return ToPath(clang_getFileName(file));
    }

    bool parser::IsRecursivelyPublic(CXCursor cursor)
    {
      while (clang_isDeclaration(clang_getCursorKind(cursor)) != 0)
      {
        auto access = clang_getCXXAccessSpecifier(cursor);
        if (access == CX_CXXPrivate || access == CX_CXXProtected)
        {
          return false;
        }

        if (clang_getCursorLinkage(cursor) == CXLinkage_Internal)
        {
          return false;
        }

        if (clang_getCursorKind(cursor) == CXCursor_Namespace
          && Convert(clang_getCursorSpelling(cursor)).empty())
        {
          // Anonymous namespace.
          return false;
        }

        cursor = clang_getCursorSemanticParent(cursor);
      }

      return true;
    }
  }
}