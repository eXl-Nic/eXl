/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/process.hpp>

#if WIN32

#include <Windows.h>

#include <thread>

namespace eXl
{
  class Process::Impl
  {
  public:
    Impl()
    {
      // set the size of the structures
      ZeroMemory( &si, sizeof(si) );
      si.cb = sizeof(si);
      ZeroMemory( &pi, sizeof(pi) );

      // Set the bInheritHandle flag so pipe handles are inherited. 

      saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
      saAttr.bInheritHandle = TRUE; 
      saAttr.lpSecurityDescriptor = NULL; 

      // Create a pipe for the child process's STDOUT. 

      CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0);
      // Ensure the read handle to the pipe for STDOUT is not inherited.
      SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

      CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0);
      // Ensure the read handle to the pipe for STDOUT is not inherited.
      SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0);

      si.hStdOutput = hChildStd_OUT_Wr;
      //si.hStdError = hChildStd_ERR_Wr;
      si.dwFlags |= STARTF_USESTDHANDLES;
    }

    void CollectOutput()
    {
      const uint32_t bufSize = 4096;
      DWORD dwReadOut;
      //DWORD dwReadErr;
      CHAR chBuf[bufSize]; 
      BOOL bSuccess = FALSE;

      for (;;) 
      { 
        bool hasOutput = false;
        if(ReadFile( hChildStd_OUT_Rd, chBuf, bufSize, &dwReadOut, NULL) && dwReadOut > 0)
        {
          hasOutput = true;
          stdOut.insert(stdOut.end(), chBuf, chBuf + dwReadOut);
        }

        //if(ReadFile( hChildStd_ERR_Rd, chBuf, bufSize, &dwReadErr, NULL) && dwReadErr > 0)
        //{
        //  hasOutput = true;
        //  stdErr.insert(stdErr.end(), chBuf, chBuf + dwReadErr);
        //}
        

        if (! hasOutput ) break; 
      } 
    }

    void Clear()
    {
      CloseHandle(hChildStd_OUT_Rd);
      CloseHandle(hChildStd_ERR_Rd);
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;

    std::thread m_readerThread;
    SECURITY_ATTRIBUTES saAttr; 
    STARTUPINFO si;     
    PROCESS_INFORMATION pi;
    Vector<char> stdOut;
    Vector<char> stdErr;
    DWORD exitCode;
  };

  Process::Process(const char* iExecutablePath)
    : m_ExecutablePath(iExecutablePath)
  {

  }

  Process::~Process()
  {
    Clear();
  }

  void Process::AddArgument(const char* iArg)
  {
    m_Arguments.push_back(iArg);
  }

  Err Process::Start()
  {
    if(m_Impl)
    {
      return Err::Failure;
    }

    m_Impl = new Impl;

    Vector<char> cmdLine(m_ExecutablePath.begin(), m_ExecutablePath.end());
    
    for(auto const& arg : m_Arguments)
    {
      cmdLine.push_back(' ');
      cmdLine.insert(cmdLine.end(), arg.begin(), arg.end());
    }

    cmdLine.push_back(0);

    // start the program up
    bool started = CreateProcess( m_ExecutablePath.c_str(),   // CmdLine
      cmdLine.data(),        // Args
      NULL,           // Process handle not inheritable
      NULL,           // Thread handle not inheritable
      TRUE,          // Set handle inheritance
      0,              // No creation flags
      NULL,           // Use parent's environment block
      NULL,           // Use parent's starting directory 
      &m_Impl->si,    // Pointer to STARTUPINFO structure
      &m_Impl->pi     // Pointer to PROCESS_INFORMATION structure
    );

    CloseHandle(m_Impl->hChildStd_OUT_Wr);
    CloseHandle(m_Impl->hChildStd_ERR_Wr);
    if(!started)
    {
      Clear();
      return Err::Failure;
    }
    else
    {
      m_Impl->m_readerThread = std::thread([this]{m_Impl->CollectOutput();});
      return Err::Success;
    }
  }

  void Process::Clear()
  {
    if(m_Impl)
    {
      m_Impl->Clear();

      delete m_Impl;
      m_Impl = nullptr;
    }
    m_Arguments.clear();
  }
  
  Err Process::WaitForEnd(uint32_t iTimeOut)
  {
    if(m_Impl)
    {
      DWORD waitResult = WaitForSingleObject(m_Impl->pi.hProcess, iTimeOut);
      if(waitResult == WAIT_TIMEOUT)
      {
        return Err::Failure;
      }

      eXl_ASSERT(waitResult == WAIT_OBJECT_0);

      m_Impl->m_readerThread.join();
      GetExitCodeProcess(m_Impl->pi.hProcess, &m_Impl->exitCode);

      return Err::Success;
    }
    return Err::Failure;
  }

  Err Process::Succeeded()
  {
    if(m_Impl)
    {
      if(m_Impl->exitCode == 0)
      {
        return Err::Success;
      }
    }
    return Err::Failure;
  }

  static Vector<char> s_dummy;

  Vector<char> const& Process::GetStdOut()
  {
    if(m_Impl)
    {
      return m_Impl->stdOut;
    }
    return s_dummy;
  }

  Vector<char> const& Process::GetStdErr()
  {
    if(m_Impl)
    {
      return m_Impl->stdErr;
    }
    return s_dummy;
  }

  bool Process::IsRunning()
  {
    return m_Impl != nullptr;
  }
}

#endif