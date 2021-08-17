#include <iostream>
#include <string>
#include <fstream>

#include "parser.hpp"
#include "serializer.hpp"

#include <boost/program_options.hpp>

namespace eXl
{
  namespace
  {
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

    Vector<String> GetFilesToProcess(boost::program_options::options_description const& desc, boost::program_options::variables_map& vm)
    {
      Vector<String> files;
      bool printHelp = vm.count("help") > 0;
      
      if(!printHelp)
      {
        if (vm.count("input-file") == 0)
        {
          printHelp = true;
        }
        else
        {
          std::vector<std::string> fileInput = vm["input-file"].as<std::vector<std::string>>();
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
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("input-file", boost::program_options::value<std::vector<std::string>>(), "Files to parse")
    ("include", boost::program_options::value<std::string>()->default_value(".*"), "regex for which types to include in reflection generation")
    ("exclude", boost::program_options::value<std::string>()->default_value("std::.*"), "regex for which types to exclude from reflection generation")
    ("out-hpp", boost::program_options::value<std::string>(), "Output file path to write declarations (header) to.")
    ("out-cpp", boost::program_options::value<std::string>()->default_value(""), "Output file path to write definitions to.")
    ("list-only", boost::program_options::bool_switch()->default_value(false), "Only list type names, don't generate")
    ("internal-name", boost::program_options::value<std::string>(), "Internal lib name, to be used with macros, etc...")
    ;

  boost::program_options::positional_options_description p;
  p.add("input-file", -1);

  int32_t optionsBeforeClang = 0;

  for (int32_t i = 0; i < argc; ++i)
  {
    if (strcmp(argv[i], "--") == 0)
    {
      break;
    }
    ++optionsBeforeClang;
  }

  boost::program_options::variables_map vm;
  try
  {
    boost::program_options::store(boost::program_options::command_line_parser(optionsBeforeClang, argv).options(desc).positional(p).run(), vm);
    boost::program_options::notify(vm);
  }
  catch (std::exception& ex)
  {
    std::cerr << ex.what();
    PrintHelp(desc);
    exit(-1);
  }

  int32_t const clangArgc = argc - (optionsBeforeClang + 1);
  char** clangArgv = argv + (optionsBeforeClang + 1);

  Vector<String> filesStr = GetFilesToProcess(desc, vm);
  Vector<Path> files;
  for (auto const& str : filesStr)
  {
    files.push_back(Path(str.c_str()));
  }

  parser::Options options;
  options.include = "^(" + vm["include"].as<std::string>() + ")$";
  options.exclude = "^(" + vm["exclude"].as<std::string>() + ")$";

  // Write a dummy file for parsing;
  if(vm.count("out-hpp") > 0)
  {
    std::ofstream dummyOut(vm["out-hpp"].as<std::string>());
    dummyOut << "#pragma once\n";
    dummyOut << "//Placeholder file for parsing\n";
  }

  if (vm["list-only"].as<bool>())
  {
    auto names = parser::GetSupportedTypeNames(files, clangArgc, clangArgv, options);
    for (const auto& it : names)
    {
      std::cout << it << std::endl;
    }
  }
  else
  {
    auto types = parser::GetTypes(files, clangArgc, clangArgv, options);
    serializer::Options options;
    options.out_hpp_path = vm["out-hpp"].as<std::string>().c_str();
    options.out_cpp_path = vm["out-cpp"].as<std::string>().c_str();
    options.internalLibName = vm["internal-name"].as<std::string>().c_str();
    serializer::Serialize(types, options);
  }
}