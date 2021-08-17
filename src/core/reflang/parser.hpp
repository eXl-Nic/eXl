#pragma once

#include <regex>

#include "types.hpp"
namespace eXl
{
  namespace reflang
  {
    namespace parser
    {
      struct Options
      {
        std::regex include;
        std::regex exclude;
      };

      Vector<String> GetSupportedTypeNames(
        const Vector<Path>& files,
        int argc, char* argv[],
        const Options& options = Options());

      Vector<std::unique_ptr<TypeBase>> GetTypes(
        const Vector<Path>& files,
        int argc, char* argv[],
        const Options& options = Options());
    };
  }
}