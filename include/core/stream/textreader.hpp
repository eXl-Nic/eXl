/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/stream/reader.hpp>

namespace eXl
{
  class EXL_CORE_API TextReader : public Reader
  {
    DECLARE_RTTI(TextReader, Reader)
  public:
    TextReader(String const& iIdent) : Reader(iIdent) {}
    virtual ~TextReader() {}

    Err ClearWhiteSpaces();

    Err ExtractInt(int& oInt);
    Err ExtractUInt(unsigned int& oUInt);
    Err ExtractUInt64(uint64_t& oUInt);
    Err ExtractFloat(float& oFloat);
    Err ExtractDouble(double& oDouble);

    virtual void reset() = 0;
    virtual void clear() = 0;
    virtual Char get() = 0;
    virtual Char peek() = 0;
    virtual size_t getPos() = 0;
    virtual void setPos(size_t) = 0;
    bool eof() const { return m_IsEof; };
    bool good() const { return m_isGood; }
  protected:

    template <typename T>
    Err ExtractUInt_T(T& oUInt);

    bool m_isGood;
    bool m_IsEof;
  };

  class EXL_CORE_API StringViewReader : public TextReader
  {
  public:
    StringViewReader();
    StringViewReader(String const& iIdent, const char*  iFileBegin, const char*  iFileEnd);

    void reset() override;
    void clear() override;
    Char get() override;
    Char peek() override;
    size_t getPos() override;
    void setPos(size_t) override;

  protected:

    Char advance();

    const char*  m_FileBegin;
    const char*  m_FileEnd;
    const char*  m_CurCharPtr;
    const char*  m_NextCharPtr;
    Char   m_CurChar;
    size_t m_Size;
  };
}