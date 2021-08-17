/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/textreader.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(TextReader)

  bool isNumericChar(char x)
  {
    return (x >= '0' && x <= '9')? true: false;
  }

  Err TextReader::ClearWhiteSpaces()
  {
    if (!eof())
    {
      while (good() && StringUtil::IsSpace(peek()))
      {
        get();
      }
      if (eof() && good())
      {
        clear();
      }
    }
    RETURN_SUCCESS;
  }

  Err TextReader::ExtractInt(int& oInt)
  {
    unsigned int res = 0;
    int sign = 1;
    if(good())
    {
      if (peek() == '-')
      {
        sign = -1;
        get();
      }
      if(ExtractUInt(res))
      {
        if(good() && ((sign == 1 && res <= INT32_MAX) || (-((int)res) >= INT32_MIN)))
        {
          oInt = sign * res;
          return Err::Success;
        }
      }
    }
    return Err::Failure;
  }

  template <typename T>
  Err TextReader::ExtractUInt_T(T& oUInt)
  {
    T res = 0;
    if(good())
    {
      unsigned int numRead = 0;
      while (good() && !eof())
      {
        if (!isNumericChar(peek()))
        {
          break;
        }
          
        T oldRes = res;
        res = res*10 + get() - '0';
        ++numRead;
        if(res < oldRes)
        {
          //Overflow
          return Err::Failure;
        }
      }
      if(good() && numRead > 0)
      {
        oUInt = res;
        return Err::Success;
      }
    }
    return Err::Failure;
  }

  Err TextReader::ExtractUInt(uint32_t& oUInt)
  {
    return ExtractUInt_T(oUInt);
  }

  Err TextReader::ExtractUInt64(uint64_t& oUInt)
  {
    return ExtractUInt_T(oUInt);
  }

  Err TextReader::ExtractFloat(float& oFloat)
  {
    double temp;
    if(ExtractDouble(temp))
    {
      oFloat = (float)temp;
      return Err::Success;
    }
    return Err::Failure;
  }

  Err TextReader::ExtractDouble(double& oDouble)
  {
    double integerPart = 0;
    double fractionPart = 0;
    int divisorForFraction = 1;
    int sign = 1;
    bool inFraction = false;
    double exponentVal = 1.0;
    /*Take care of +/- sign*/
    if (peek() == '-')
    {
      get();
      sign = -1;
    }
    else if (peek() == '+')
    {
      get();
    }

    unsigned int numRead = 0;

    while (good() && !eof())
    {
      if(peek() == '.')
      {
        get();
        if(inFraction)
          break;
        else
          inFraction = true;
      }
      else if (peek() == 'e')
      {
        get();
        int exponent;
        if(ExtractInt(exponent))
        {
          exponentVal = pow(10.0, exponent);
          break;
        }
        else 
        {
          return Err::Failure;
        }
      }
      else if (!isNumericChar(peek()))
      {
        break;
      }

      if (inFraction)
      {
        ++numRead;
        fractionPart = fractionPart*10 + (get() - '0');
        divisorForFraction *= 10;
      }
      else
      {
        ++numRead;
        integerPart = integerPart*10 + (get() - '0');
      }
    }
    if(good() && numRead > 0)
    {
      oDouble = sign * exponentVal * (integerPart + fractionPart/divisorForFraction);
      return Err::Success;
    }
    return Err::Failure;
  }

  StringViewReader::StringViewReader()
    : TextReader("")
    , m_FileBegin(nullptr)
    , m_FileEnd(nullptr)
    , m_CurCharPtr(nullptr)
    , m_NextCharPtr(nullptr)
    , m_Size(0)
  {
    m_IsEof = true;
    m_isGood = false;
  }

  StringViewReader::StringViewReader(String const& iIdent, const char* iFileBegin, const char* iFileEnd)
    : TextReader(iIdent)
    , m_FileBegin(iFileBegin)
    , m_FileEnd(iFileEnd)
    , m_Size(iFileEnd - iFileBegin)
  {
    m_IsEof = true;
    m_isGood = false;
    reset();
  }

  void StringViewReader::clear()
  {
    if(m_Size > 0)
    {
      m_isGood = true;
    }
  }

  Char StringViewReader::advance()
  {
    if(m_NextCharPtr == m_FileEnd)
    {
      m_NextCharPtr = nullptr;
    }
    return *(m_NextCharPtr++);
  }

  void StringViewReader::reset()
  {
    if(m_Size > 0)
    {
      m_CurCharPtr = m_FileBegin;
      m_NextCharPtr = m_CurCharPtr;
      m_CurChar = advance();
      m_isGood = m_NextCharPtr != nullptr;
      m_IsEof = false;
    }
  }

  Char StringViewReader::get()
  {
    if(m_isGood && !m_IsEof)
    {
      Char curChar = m_CurChar;
      m_CurCharPtr = m_NextCharPtr;
      m_CurChar = advance();
      m_isGood = m_NextCharPtr != nullptr;
      m_IsEof = m_NextCharPtr == m_FileEnd;
      return curChar;
    }
    else
    {
      m_isGood = false;
    }
    return 0;
  }

  Char StringViewReader::peek()
  {
    return m_CurChar;
  }

  size_t StringViewReader::getPos()
  {
    if(m_isGood)
    {
      return m_CurCharPtr - m_FileBegin;
    }
    return 0;
  }

  void StringViewReader::setPos(size_t iOffset)
  {
    if(iOffset < m_Size)
    {
      m_CurCharPtr = m_FileBegin + iOffset;
      m_NextCharPtr = m_CurCharPtr;
      m_CurChar = advance();
      m_isGood = m_NextCharPtr != nullptr;
      m_IsEof = m_NextCharPtr == m_FileEnd;
    }
    else
    {
      m_isGood = false;
    }
  }
}