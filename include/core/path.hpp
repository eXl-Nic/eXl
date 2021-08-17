#pragma once

#include <core/coredef.hpp>

#if !defined(__ANDROID__)
#define EXL_RSC_HAS_FILESYSTEM
#include <boost/filesystem.hpp>

namespace eXl
{
  using Path = boost::filesystem::path;
  namespace Filesystem = boost::filesystem;
}
#endif