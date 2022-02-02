/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <exception>

namespace std
{
  class exception;
}

namespace eXl
{
  class EXL_CORE_API TestUnit
  {
  public:
    TestUnit(const String& iName);

    void AddTest();

    inline const String& GetName()const {return m_Name;}

    String Report()const;

    void Failed(const String& msg);

  private:
    const String m_Name;
    String m_Msg;
    unsigned int status;
    unsigned int numTest;
  };

  const unsigned int CRASH_UNEX = 0x00000002;
  const unsigned int CRASH_UNIMP = 0x00000004;
  const unsigned int CRASH_ASSERT = 0x00000008;
  const unsigned int SIGNAL_UNIMP = 0x00000010;
  const unsigned int ALLOW_ASSERT_REPAIR = 0x00000020;
  const unsigned int THROW_UNEX = 0x00000040;
  const unsigned int THROW_ASSERT = 0x00000080;
  const unsigned int NOASK_UNEX = 0x00000100;
  const unsigned int NOASK_ASSERT = 0x00000200;
  const unsigned int SILENT_UNEX = 0x00000500;
  const unsigned int SILENT_ASSERT = 0x00000A00;
  
  const unsigned int TESTING_STAGE = THROW_UNEX | THROW_ASSERT | SILENT_UNEX | SILENT_ASSERT;
  const unsigned int DEBUG_STAGE = THROW_UNEX | THROW_ASSERT;
  const unsigned int RELEASE_STAGE = CRASH_UNEX | /*CRASH_ASSERT |*/ NOASK_ASSERT | NOASK_UNEX | ALLOW_ASSERT_REPAIR;
  const unsigned int BETA_STAGE = ALLOW_ASSERT_REPAIR;
  
  EXL_CORE_API void SetErrorHandling(unsigned int);
  EXL_CORE_API void ClearException();

}

