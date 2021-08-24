#include <iostream>
#include <string>
#include <fstream>

#include "parser.hpp"
#include "serializer.hpp"

#define CXXOPTS_NO_RTTI
#include <cxxopts.hpp>

namespace eXl
{
  namespace
  {
#if 0
    void PrintHelp(boost::program_options::options_description const& desc)
    {
      std::cout << "Reflang tool to generate reflection metadata.\n";
      std::cout << "\n";
      std::cout << "Usage: reflang [reflang_flags] -- [clang_flags]\n";
      std::cout << "Where [reflang_flags] are any of the below, and [clang_flags] are any flags supported by the libclang version installed\n";
      std::cout << "\n";
      std::cout << "Supported flags:\n";
      desc.print(std::cout);
      exit(-1);
    }
#endif

    Vector<String> GetFilesToProcess(cxxopts::ParseResult const& iResult)
    {
      Vector<String> files;
      bool printHelp = iResult.count("help") > 0;
      
      if(!printHelp)
      {
        if (iResult.count("input-file") == 0)
        {
          printHelp = true;
        }
        else
        {
          std::vector<std::string> fileInput = iResult["input-file"].as<std::vector<std::string>>();
          for (auto const& file : fileInput)
          {
            if (!file.empty())
            {
              files.push_back(file.c_str());
            }
          }
        }
        if (files.empty())
        {
          std::cerr << "No input files specified." << std::endl;
          printHelp = true;
        }
      }
      

      if (printHelp)
      {
        
      }

      return files;
    }
  }
}

using namespace eXl;
using namespace eXl::reflang;

int main(int argc, char** argv)
{
  cxxopts::Options options(argv[0]);
  options.positional_help("[optional args]").show_positional_help();

  options.allow_unrecognised_options();
  options.add_options()
    ("input-file", "Files to parse", cxxopts::value<std::vector<std::string>>())
    ("include", "regex for which types to include in reflection generation", cxxopts::value<std::string>()->default_value(".*"))
    ("exclude", "regex for which types to exclude from reflection generation", cxxopts::value<std::string>()->default_value("std::.*"))
    ("out-hpp", "Output file path to write declarations (header) to.", cxxopts::value<std::string>())
    ("out-cpp", "Output file path to write definitions to.", cxxopts::value<std::string>()->default_value(""))
    ("list-only", "Only list type names, don't generate", cxxopts::value<bool>()->default_value("false"))
    ("internal-name", "Internal lib name, to be used with macros, etc...", cxxopts::value<std::string>());
  
  options.parse_positional({ "input-file" });

  int32_t optionsBeforeClang = 0;

  for (int32_t i = 0; i < argc; ++i)
  {
    if (strcmp(argv[i], "--") == 0)
    {
      break;
    }
    ++optionsBeforeClang;
  }

  int32_t const clangArgc = argc - (optionsBeforeClang + 1);
  char** clangArgv = argv + (optionsBeforeClang + 1);

  cxxopts::ParseResult result = options.parse(optionsBeforeClang, argv);

  Vector<String> filesStr = GetFilesToProcess(result);
  Vector<Path> files;
  for (auto const& str : filesStr)
  {
    files.push_back(Path(str.c_str()));
  }

  parser::Options parserOptions;
  parserOptions.include = "^(" + result["include"].as<std::string>() + ")$";
  parserOptions.exclude = "^(" + result["exclude"].as<std::string>() + ")$";

  // Write a dummy file for parsing;
  if(result.count("out-hpp") > 0)
  {
    std::ofstream dummyOut(result["out-hpp"].as<std::string>());
    dummyOut << "#pragma once\n";
    dummyOut << "//Placeholder file for parsing\n";
  }

  if (result["list-only"].as<bool>())
  {
    auto names = parser::GetSupportedTypeNames(files, clangArgc, clangArgv, parserOptions);
    for (const auto& it : names)
    {
      std::cout << it << std::endl;
    }
  }
  else
  {
    auto types = parser::GetTypes(files, clangArgc, clangArgv, parserOptions);
    serializer::Options options;
    options.out_hpp_path = result["out-hpp"].as<std::string>().c_str();
    options.out_cpp_path = result["out-cpp"].as<std::string>().c_str();
    options.internalLibName = result["internal-name"].as<std::string>().c_str();
    serializer::Serialize(types, options);
  }
}