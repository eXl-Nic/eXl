/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/stream/streamer.hpp>

#include <vector>
#include <ostream>

namespace eXl
{
  class EXL_CORE_API JSONStreamer : public Streamer
  {
  public:

    typedef std::basic_ostream<Char, String::traits_type> OStream;

    JSONStreamer(OStream* iOutStream);

    Err Begin();
    Err End();

    Err PushKey(String const& iKey);
    Err PopKey();

    Err BeginSequence(/*unsigned int iSize*/);
    Err EndSequence();

    Err BeginStruct();
    Err EndStruct();

    Err WriteInt(int const* iInt);
    Err WriteUInt(unsigned int const* iUInt);
    Err WriteUInt64(uint64_t const* iUInt);
    Err WriteFloat(float const* iFloat);
    Err WriteDouble(double const* oDouble);
    Err WriteString(Char const* iStr);
    Err WriteString(KString const& iStr);

  protected:

    void WriteChar(Char iChar);

    Err ElementPrologue();
    Err ElementEpilogue();

    void WriteIndent();
    bool CurLevelInSequence();
    bool ConsumeKey();

    std::vector<unsigned int> m_SequenceLevel;
    std::vector<bool>         m_PendingSeqElem;

    bool m_PendingPushKey;
    bool m_PendingPopKey;

    unsigned int m_CurrentStructLevel;
    unsigned int m_CurrentKeyLevel;

    bool m_StreamFailed = false;

    OStream* m_OutStream;
  };
}