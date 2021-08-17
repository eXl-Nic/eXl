#pragma once

#include <core/rtti.hpp>

namespace eXl
{
  class EXL_CORE_API Reader : public RttiObject
  {
    DECLARE_RTTI(Reader, RttiObject)
  public:
    Reader(String const& iIdent)
      : m_Ident (iIdent)
    {}

    String const& m_Ident;
  };
}