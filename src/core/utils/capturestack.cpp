#include <core/utils/capturestack.hpp>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#endif

namespace eXl
{
  Stack CaptureStack()
  {
#ifdef WIN32

    void* stack[64];
    unsigned long stackHash;

    uint32_t numCaptured = CaptureStackBackTrace(1, 64, stack, &stackHash);

    Stack capturedStack;
    capturedStack.m_Hash = stackHash;
    capturedStack.m_Stack.reserve(numCaptured);
    capturedStack.m_Stack.insert(capturedStack.m_Stack.begin(), stack, stack + numCaptured);

    return capturedStack;
#endif
  }

  static constexpr size_t TRACE_MAX_FUNCTION_NAME_LENGTH = 1024;

  EXL_CORE_API DebugString DumpStackInformation(Stack const& iStack)
  {
    DebugString result;
#ifdef WIN32
    static const bool s_Initialized = []
    {
      HANDLE process = GetCurrentProcess();
      return SymInitialize(process, NULL, TRUE);
    }();
    eXl_ASSERT_REPAIR_RET(s_Initialized, result);

    HANDLE process = GetCurrentProcess();
    char buf[sizeof(SYMBOL_INFO) + (TRACE_MAX_FUNCTION_NAME_LENGTH - 1) * sizeof(TCHAR)];
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)buf;
    symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    DWORD displacement;
    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    for (uint32_t i = 0; i < iStack.m_Stack.size(); i++)
    {
      DWORD64 address = (DWORD64)(iStack.m_Stack[i]);
      SymFromAddr(process, address, NULL, symbol);
      if (SymGetLineFromAddr64(process, address, &displacement, &line))
      {
        result += StringUtil::FormatDebug("\tat %s in %s: line: %lu: address: 0x%0X\n", symbol->Name, line.FileName, line.LineNumber, symbol->Address);
      }
      //else
      //{
      //  result += StringUtil::Format("\tSymGetLineFromAddr64 returned error code %lu.\n", GetLastError());
      //  result += StringUtil::Format("\tat %s, address 0x%0X.\n", symbol->Name, symbol->Address);
      //}
    }

#endif
    return result;
  }
}