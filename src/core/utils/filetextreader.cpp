/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/utils/filetextreader.hpp>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace eXl
{
  FileTextReader::~FileTextReader()
  {
    UnmapViewOfFile(m_FileBegin);
    CloseHandle(m_Mapping);
    CloseHandle(m_File);
  }

  void ReportLastError()
  {
    LPSTR msg = nullptr;
    DWORD err = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
      nullptr, 
      err, 0, (LPSTR)&msg, 0, nullptr);

    if(msg)
    {
      eXl_ASSERT_MSG(false, msg);
      LocalFree(msg);
    }
  }

  FileTextReader* FileTextReader::Create(char const* iPath)
  {
    HANDLE file = CreateFileA(iPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

    if(file != INVALID_HANDLE_VALUE)
    {
      DWORD highSize;
      DWORD size = GetFileSize(file, &highSize);
      if(size > 0)
      {
        HANDLE mapping =  CreateFileMapping(file, nullptr, FILE_READ_ONLY, highSize, size, nullptr);
        if(mapping != 0)
        {
          const char* fileBegin = (char*)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, size);
          if(fileBegin)
          {
            return eXl_NEW FileTextReader(String(iPath), file, mapping, fileBegin, fileBegin + size);
          }
          else
          {
            ReportLastError();
          }
        }
        else
        {
          ReportLastError();
        }
      }
      else
      {
        ReportLastError();
      }
    } 
    else
    {
      ReportLastError();
    }

    return nullptr;
  }
}
#endif