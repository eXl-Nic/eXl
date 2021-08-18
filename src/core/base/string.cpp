/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/string.hpp>

#include <cstdio>
#include <cstdarg>

namespace eXl
{


#ifdef WIN32
#define SPrintfWrapper(iBuffer, iFormat, ...) sprintf_s(iBuffer, iFormat, __VA_ARGS__);
#else
#define SPrintfWrapper(iBuffer, iFormat, ...) sprintf(iBuffer, iFormat, __VA_ARGS__);
#endif

#ifdef WIN32
#define SWPrintfWrapper(iBuffer, iFormat, ...) swprintf_s(iBuffer, iFormat, __VA_ARGS__);
#else

  template <unsigned int N>
  void swprintf_N(wchar_t (&iBuffer)[N], const wchar_t* iFormat, ...)
  {
    va_list args;
    va_start(args, iFormat);
    vswprintf(iBuffer, N, iFormat, args);
    va_end(args);
  }

#define SWPrintfWrapper(iBuffer, iFormat, ...) swprintf_N(iBuffer, iFormat, __VA_ARGS__)
  
#endif

  AString StringUtil::AFromInt(int iVal)
  {
    char buffer[64];
    SPrintfWrapper(buffer, "%i", iVal);
    return AString(buffer);
  }
  AString StringUtil::AFromInt(unsigned int iVal)
  {
    char buffer[64];
    SPrintfWrapper(buffer, "%u", iVal);
    return AString(buffer);
  }
  AString StringUtil::AFromSizeT(size_t iVal)
  {
    char buffer[64];
#ifdef WIN32
    SPrintfWrapper(buffer,"%Iu",iVal);
#else
    SPrintfWrapper(buffer,"%zu",iVal);
#endif
    return AString(buffer);
  }
  AString StringUtil::AFromFloat(float iVal)
  {
    char buffer[64];
    SPrintfWrapper(buffer,"%f",iVal);
    return AString(buffer);
  }
  AString StringUtil::AFromFloat(double iVal)
  {
    char buffer[64];
    SPrintfWrapper(buffer,"%lf",iVal);
    return AString(buffer);
  }
  AString StringUtil::AFromPtr(const void* iVal)
  {
    char buffer[64];
    SPrintfWrapper(buffer,"%p",iVal);
    return AString(buffer);
  }

  WString StringUtil::WFromInt(int iVal)
  {
    wchar_t buffer[64];
    SWPrintfWrapper(buffer,L"%i",iVal);
    return WString(buffer);
  }
  WString StringUtil::WFromInt(unsigned int iVal)
  {
    wchar_t buffer[64];
    SWPrintfWrapper(buffer,L"%u",iVal);
    return WString(buffer);
  }
  WString StringUtil::WFromSizeT(size_t iVal)
  {
    wchar_t buffer[64];
#ifdef WIN32
    SWPrintfWrapper(buffer,L"%Iu",iVal);
#else
    SWPrintfWrapper(buffer,L"%zu",iVal);
#endif
    return WString(buffer);
  }
  WString StringUtil::WFromFloat(float iVal)
  {
    wchar_t buffer[64];
    SWPrintfWrapper(buffer,L"%f",iVal);
    return WString(buffer);
  }
  WString StringUtil::WFromFloat(double iVal)
  {
    wchar_t buffer[64];
    SWPrintfWrapper(buffer,L"%lf",iVal);
    return WString(buffer);
  }
  WString StringUtil::WFromPtr(const void* iVal)
  {
    wchar_t buffer[64];
    SWPrintfWrapper(buffer,L"%p",iVal);
    return WString(buffer);
  }


#if WIN32
#define SScanF sscanf_s
#define WSScanF wscanf_s
#else
#define SScanF sscanf
#define WSScanF wscanf
#endif


  int StringUtil::ToInt(AString const& iStr)
  {
    int res;
    SScanF(iStr.c_str(), "%i", &res);
    return res;
  }

  unsigned int StringUtil::ToUInt(AString const& iStr)
  {
    unsigned int res;
    SScanF(iStr.c_str(),"%u",&res);
    return res;
  }

  size_t StringUtil::ToSizeT(AString const& iStr)
  {
    size_t res;
#ifdef WIN32
    SScanF(iStr.c_str(),"%Iu",&res);
#else
    SScanF(iStr.c_str(),"%zu",&res);
#endif
    return res;
  }

  float StringUtil::ToFloat(AString const& iStr)
  {
    float res;
    SScanF(iStr.c_str(),"%f",&res);
    return res;
  }

  double StringUtil::ToDouble(AString const& iStr)
  {
    double res;
    SScanF(iStr.c_str(),"%lf",&res);
    return res;
  }

  void const* StringUtil::ToPtr(AString const& iStr)
  {
    void const* res;
    SScanF(iStr.c_str(),"%p",&res);
    return res;
  }

  int StringUtil::ToInt(WString const& iStr)
  {
    int res;
    WSScanF(iStr.c_str(),"%i",&res);
    return res;
  }

  unsigned int StringUtil::ToUInt(WString const& iStr)
  {
    unsigned int res;
    WSScanF(iStr.c_str(),"%u",&res);
    return res;
  }

  size_t StringUtil::ToSizeT(WString const& iStr)
  {
    size_t res;
#ifdef WIN32
    WSScanF(iStr.c_str(),"%Iu",&res);
#else
    WSScanF(iStr.c_str(),"%zu",&res);
#endif
    return res;
  }

  float StringUtil::ToFloat(WString const& iStr)
  {
    float res;
    WSScanF(iStr.c_str(),"%f",&res);
    return res;
  }

  double StringUtil::ToDouble(WString const& iStr)
  {
    double res;
    WSScanF(iStr.c_str(),"%lf",&res);
    return res;
  }

  void const* StringUtil::ToPtr(WString const& iStr)
  {
    void const* res;
    WSScanF(iStr.c_str(),"%p",&res);
    return res;
  }

  WString StringUtil::WFromASCII(char const* iStr)
  {
    size_t buffLen = 4* strlen(iStr) + 1;
    wchar_t* strBuff = (wchar_t*)malloc(buffLen);
    size_t converted;
#if WIN32
    mbstowcs_s(&converted, strBuff, buffLen / sizeof(wchar_t), iStr, buffLen);
#else
    mbstowcs(strBuff, iStr, buffLen);
#endif
    WString ret(strBuff);
    free(strBuff);
    return ret;
  }

  AString StringUtil::ToASCII(wchar_t const* iStr)
  {
    size_t buffLen = wcslen(iStr) + 1; // +1 for nullptr terminator
    char* strBuff = (char*)malloc(buffLen);
    size_t charsConverted = 0;
#if WIN32
    wcstombs_s(&charsConverted, strBuff, buffLen, iStr, buffLen - 1);
#else
    wcstombs(strBuff, iStr, buffLen - 1);
#endif
    AString ret(strBuff);
    return ret;
  }
}