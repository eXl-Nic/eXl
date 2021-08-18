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

#include <istream>
#include <streambuf>

namespace eXl
{
  class EXL_CORE_API InputStream : public HeapObject
  {
  public:
    virtual size_t Read(size_t iOffset, size_t iSize, void* oData) = 0;
    virtual size_t GetSize() = 0;
    virtual ~InputStream() {}
  protected:
    InputStream();
  private:
    InputStream& operator=(const InputStream&);
    InputStream(const InputStream&);
  };

  class InputStream_istream : public std::istream
  {
    class InputStream_streambuf : public std::streambuf
    {
    public:
      EXL_CORE_API InputStream_streambuf(InputStream* iStream, size_t iBuffSize = 256, size_t iPutBack = 8);
    private:
      int_type underflow();
      std::streampos seekpos(std::streampos sp, std::ios_base::openmode which);
      std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
      InputStream* m_Stream;
      size_t m_ReadOffset;
      size_t m_Size;
      const std::size_t m_PutBack;
      Vector<char> m_Buffer;
    };
    unsigned char m_StreamBuff[sizeof(InputStream_streambuf)];
  public:
    inline InputStream_istream(InputStream* iStream)
      :std::istream(new(m_StreamBuff) InputStream_streambuf(iStream)) {}
  };
}

#include <core/stream/textreader.hpp>

namespace eXl
{
  class EXL_CORE_API InputStreamTextReader : public TextReader
  {
  public:

    InputStreamTextReader(String const& iIdent, std::unique_ptr<InputStream> iStream);

    void reset() override;
    void clear() override;
    Char get() override;
    Char peek() override;
    size_t getPos() override;
    void setPos(size_t) override;

  protected:

    void EnsureBuffer(size_t iDesiredOffset);

    std::unique_ptr<InputStream> m_Stream;
    size_t m_CurOffset = 0;
    size_t m_BufferedOffset = -1;
    static constexpr size_t s_BufferSize = 256;
    Char m_Buffer[s_BufferSize];
  };
}