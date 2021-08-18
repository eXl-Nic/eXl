/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/corelibexp.hpp>
#include <core/intrusiveptr.hpp>
#include <list>
#include <string>

namespace eXl
{
  class Image;

  namespace ImageStreamer
  {
    enum Codec
    {
      Png,
      Jpg,
      Bmp
    };

    class TransientLoader
    {
    public:
      virtual void Load(Image const* iImage) = 0;
    };

    EXL_CORE_API Image* Load(AString const& iPath);
    EXL_CORE_API Image* Load(uint8_t const* iBuffer, size_t iSize);

    EXL_CORE_API void TransientLoad(AString const& iPath, TransientLoader* iLoader);

    EXL_CORE_API void Save(Image const* iImage, AString const& iPath);

    EXL_CORE_API void Save(Image const* iImage, Codec iCodec, size_t& oSize, void*& oBuffer);

  };
}
