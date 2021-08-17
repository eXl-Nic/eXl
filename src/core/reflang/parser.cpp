#include "parser.hpp"

#include <iostream>

#include <clang-c/Index.h>

#include "parser.class.hpp"
#include "parser.enum.hpp"
#include "parser.function.hpp"
#include "parser.util.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace
    {
      std::ostream& operator<<(std::ostream& s, const CXString& str)
      {
        s << parser::Convert(str);
        return s;
      }

      CXTranslationUnit Parse(
        CXIndex& index, const Path& file, int argc, char* argv[])
      {
        CXTranslationUnit unit = clang_parseTranslationUnit(
          index,
          file.string().c_str(), argv, argc,
          nullptr, 0,
          CXTranslationUnit_None);
        if (unit == nullptr)
        {
          std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
          exit(-1);
        }

        auto diagnostics = clang_getNumDiagnostics(unit);
        if (diagnostics != 0)
        {
          std::cerr << "> Diagnostics:" << std::endl;
          for (int i = 0; i != diagnostics; ++i)
          {
            auto diag = clang_getDiagnostic(unit, i);
            std::cerr << ">>> "
              << clang_formatDiagnostic(
                diag, clang_defaultDiagnosticDisplayOptions());
          }
        }

        return unit;
      }

      struct GetTypesStruct
      {
        Path fileBeingParsed;
        Vector<std::unique_ptr<TypeBase>>* types;
        const parser::Options* options;

        Vector<String> enumNames;
        Vector<Enum> potentialEnums;

      };

      CXChildVisitResult GetTypesVisitor(
        CXCursor cursor, CXCursor parent, CXClientData client_data)
      {
        auto* data = reinterpret_cast<GetTypesStruct*>(client_data);
        std::unique_ptr<TypeBase> type;
        switch (clang_getCursorKind(cursor))
        {
        case CXCursor_EnumDecl:
        {
          Enum newEnum = parser::GetEnum(cursor);
          if (newEnum.GetFile() == data->fileBeingParsed)
          {
            //auto file = newEnum.GetFile();
            //std::replace(file.begin(), file.end(), '\\', '/');
            //if (file.find("D:/eXlProject/include/dunatk") != String::npos)
            //{
            //  printf("oo");
            //}
            data->potentialEnums.push_back(std::move(newEnum));
          }
        }
          break;
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        {
          Class newClass = parser::GetClass(cursor);
          if (newClass.HasReflectionMarker())
          {
            if (newClass.GetFile() == data->fileBeingParsed)
            {
              type = std::make_unique<Class>(std::move(newClass));
            }
          }
        }
          break;
        case CXCursor_FunctionDecl:
        {
          Function newFunction = parser::GetFunction(cursor);
          if (newFunction.GetFile() == data->fileBeingParsed)
          {
            //auto file = newFunction.GetFile();
            //std::replace(file.begin(), file.end(), '\\', '/');
            //if (file.find("D:/eXlProject/include/dunatk") != String::npos)
            //{
            //  printf("oo");
            //}
            if (newFunction.GetShortName() == "DeclareEnumReflection")
            {
              data->enumNames.push_back(newFunction.m_Arguments[0].type);
            }
          }
        }
          break;
        default:
          break;
        }

        String const& name = type->GetFullName();
        if (type
          && !name.empty()
          && parser::IsRecursivelyPublic(cursor)
          && !(name.back() == ':')
          && regex_match(name, data->options->include)
          && !regex_match(name, data->options->exclude))
        {
          data->types->push_back(std::move(type));
        }

        return CXChildVisit_Recurse;
      }
    }  // namespace

    Vector<String> parser::GetSupportedTypeNames(
      const Vector<Path>& files,
      int argc, char* argv[],
      const Options& options)
    {
      auto types = GetTypes(files, argc, argv, options);

      Vector<String> names;
      names.reserve(types.size());
      for (const auto& type : types)
      {
        names.push_back(type->GetFullName());
      }
      return names;
    }

    Vector<std::unique_ptr<TypeBase>> parser::GetTypes(
      const Vector<Path>& files,
      int argc, char* argv[],
      const Options& options)
    {
      Vector<std::unique_ptr<TypeBase>> results;
      for (const auto& file : files)
      {
        CXIndex index = clang_createIndex(0, 0);
        CXTranslationUnit unit = Parse(index, file, argc, argv);

        auto cursor = clang_getTranslationUnitCursor(unit);

        GetTypesStruct data = {file, &results, &options };
        clang_visitChildren(cursor, GetTypesVisitor, &data);

        for (auto const& name : data.enumNames)
        {
          for (auto& potEnum : data.potentialEnums)
          {
            size_t foundName = potEnum.GetFullName().find(name);
            if (foundName != std::string::npos)
            {
              //TODO : retrieve the type's full name.
              size_t foundNameEnd = foundName + name.size();
              if (foundNameEnd == potEnum.GetFullName().size()
                && (foundName == 0 || potEnum.GetFullName()[foundName - 1] == ':'))
              {
                data.types->push_back(std::make_unique<Enum>(std::move(potEnum)));
              }
            }
          }
        }

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);
      }
      return results;
    }
  }
}