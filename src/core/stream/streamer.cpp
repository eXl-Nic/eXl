/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/streamer.hpp>

#include <streambuf>
#include <istream>

namespace eXl
{

  Streamer::Streamer()
  {

  }

  Err Streamer::Begin()
  {
    return Err::Success;
  }

  Err Streamer::End()
  {
    return Err::Success;
  }

  struct BufferAdapter : public std::streambuf
  {
    BufferAdapter(uint8_t const* s, std::size_t n)
    {
      setg((char*)s, (char*)s, (char*)s + n);
    }
  };

  Err Streamer::WriteBinary(uint8_t const* iData, size_t iSize)
  {
    BufferAdapter buffer(iData, iSize);
    std::istream stream(&buffer);
    return WriteBinary(&stream, iSize);
  }
}