/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/outputstream.hpp>

namespace eXl
{
  OutputStream::OutputStream() = default;

  OutputStream_ostream::OutputStream_streambuf::OutputStream_streambuf(OutputStream* iStream, size_t iBuffSize)
    :m_Stream(iStream)
    , m_Buffer(iBuffSize + 1)
    , m_WriteOffset(0)
  {
    char *base = &m_Buffer.front();
    setp(base, base + m_Buffer.size() - 1);
  }

  std::streambuf::int_type OutputStream_ostream::OutputStream_streambuf::overflow(int ch)
  {
    if (ch != traits_type::eof())
    {
      eXl_ASSERT(std::less_equal<char *>()(pptr(), epptr()));
      *pptr() = ch;
      pbump(1);
      if (sync() == 0)
        return ch;
    }

    return traits_type::eof();
  }

  int OutputStream_ostream::OutputStream_streambuf::sync()
  {
    std::ptrdiff_t n = pptr() - pbase();

    pbump(-n);

    size_t writtenSize = m_Stream->Write(n, pbase());
    m_WriteOffset += writtenSize;
    return writtenSize == n ? 0 : -1;
  }
}