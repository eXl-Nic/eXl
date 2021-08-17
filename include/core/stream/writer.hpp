#pragma once

#include <core/rtti.hpp>
#include <iostream>

namespace eXl
{
  class EXL_CORE_API Writer : public RttiObject
  {
    DECLARE_RTTI(Writer, RttiObject)
  };

  class EXL_CORE_API StdOutWriter : public Writer
  {
    DECLARE_RTTI(StdOutWriter, RttiObject);
  public:
    StdOutWriter(std::ostream& stream)
      : m_Stream(stream)
    {}

    std::ostream& m_Stream;
  };
}