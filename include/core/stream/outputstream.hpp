/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/corelibexp.hpp>
#include <core/heapobject.hpp>
#include <core/containers.hpp>

#include <ostream>
#include <streambuf>

namespace eXl
{
  class OutputStream : public HeapObject
  {
  public:
    virtual size_t Write(size_t iSize, void* oData) = 0;
    virtual size_t GetWrittenSize() = 0;
    virtual ~OutputStream() {}
    //inline StorageInfo const* GetInfo() const { return m_Info.get(); }
  protected:
    OutputStream();
    //OutputStream(StorageInfo const* iInfo) :m_Info(iInfo) {}
    //IntrusivePtr<StorageInfo const> m_Info;
  private:
    OutputStream& operator=(const OutputStream&);
    OutputStream(const OutputStream&);
  };

  class OutputStream_ostream : public std::ostream
  {
    class OutputStream_streambuf : public std::streambuf
    {
    public:
      EXL_CORE_API OutputStream_streambuf(OutputStream* iStream, size_t iBuffSize = 256);
    private:
      int_type overflow(int ch);
      int sync();
      OutputStream* m_Stream;
      size_t m_WriteOffset;
      Vector<char> m_Buffer;
    };
    unsigned char m_StreamBuff[sizeof(OutputStream_streambuf)];
  public:
    inline OutputStream_ostream(OutputStream* iStream)
      :std::ostream(new(m_StreamBuff) OutputStream_streambuf(iStream)) {}
    inline ~OutputStream_ostream()
    {
      flush();
    }
  };
}