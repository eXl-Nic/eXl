#pragma once

#include <core/coredef.hpp>

#if WIN32

namespace eXl
{
  class EXL_CORE_API Process
  {
  public:

    Process(const char* iExecutablePath);
    ~Process();

    void AddArgument(const char* iArg);

    Err Start();

    Err WaitForEnd(uint32_t timeInMs = UINT32_MAX);

    Err Succeeded();

    Vector<char> const& GetStdOut();
    Vector<char> const& GetStdErr();

    void Clear();

  protected:

    String m_ExecutablePath;
    Vector<String> m_Arguments;

    class Impl;

    Impl* m_Impl = nullptr;
  };
}

#endif