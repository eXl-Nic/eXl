#pragma once

#include <core/stream/jsonunstreamer.hpp>

namespace eXl
{
  class EXL_CORE_API FileTextReader : public StringViewReader
  {
  public:

    static FileTextReader* Create(const char* path);
    ~FileTextReader();

  protected:

    FileTextReader(String const& iIdent, void* iFile, void* iMapping, char const* iFileBegin, char const* iFileEnd)
      : StringViewReader(iIdent, iFileBegin, iFileEnd)
      , m_File(iFile)
      , m_Mapping(iMapping)
    {
    }

    void* m_File;
    void* m_Mapping;
  };
}