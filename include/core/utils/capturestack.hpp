#pragma once

#include <core/containers.hpp>

namespace eXl
{
  struct Stack
  {
    bool operator==(Stack const& iStack) const
    {
      return m_Hash == iStack.m_Hash && m_Stack == iStack.m_Stack;
    }
    boost::container::small_vector<void*, 24, boost::container::eXlRawAllocator<void*>> m_Stack;
    unsigned long m_Hash;
  };

  EXL_CORE_API Stack CaptureStack();
  EXL_CORE_API DebugString DumpStackInformation(Stack const& iStack);
}