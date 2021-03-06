/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "corelibexp.hpp"
#include <string>
#include <string_view>
#include "allocator.hpp"

#ifndef EXL_CHAR_TYPE
#define EXL_CHAR_TYPE char
#define EXL_CHAR_TYPE_IS_CHAR
//#define EXL_CHAR_TYPE wchar_t
//#define EXL_CHAR_TYPE_IS_WCHAR
#endif

//#if defined(EXL_CHAR_TYPE_IS_CHAR)
//#define _EXL_TEXT(t) t
//#endif
//
//#if defined(EXL_CHAR_TYPE_IS_WCHAR)
//#define _EXL_TEXT(t) L##t
//#endif

//#define EXL_TEXT(t) _EXL_TEXT(t)

#include <core/corenew.hpp>

namespace std
{
  typedef basic_string<EXL_CHAR_TYPE, std::char_traits<EXL_CHAR_TYPE>, eXl::Allocator<EXL_CHAR_TYPE> > eXl_String;

  typedef basic_string<char, std::char_traits<char>, eXl::Allocator<char> > eXl_AString;

  typedef basic_string<wchar_t, std::char_traits<wchar_t>, eXl::Allocator<wchar_t> > eXl_WString;
}

template class EXL_CORE_API std::basic_string<char, std::char_traits<char>, eXl::Allocator<char> >;
template class EXL_CORE_API std::basic_string<wchar_t, std::char_traits<wchar_t>, eXl::Allocator<wchar_t> >;

namespace eXl
{
  typedef EXL_CHAR_TYPE Char;

  typedef std::basic_string<char, std::char_traits<char>, Allocator<char> > AString;
  typedef AString String;
  typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, Allocator<wchar_t> > WString;

  typedef std::basic_string_view<char> KString;

  class EXL_CORE_API StringUtil
  {
  public:

    inline static String FromInt(int);
    inline static String FromInt(unsigned int);
    inline static String FromSizeT(size_t);
    inline static String FromFloat(float);
    inline static String FromFloat(double);
    inline static String FromPtr(const void*);

    static AString AFromInt(int);
    static AString AFromInt(unsigned int);
    static AString AFromSizeT(size_t);
    static AString AFromFloat(float);
    static AString AFromFloat(double);
    static AString AFromPtr(const void*);

    static WString WFromInt(int);
    static WString WFromInt(unsigned int);
    static WString WFromSizeT(size_t);
    static WString WFromFloat(float);
    static WString WFromFloat(double);
    static WString WFromPtr(const void*);

    inline static bool IsSpace(Char iChar);

    inline static String FromASCII(char const*);
    inline static AString ToASCII(AString const& iStr){return iStr;}
    inline static String FromASCII(AString const& iStr){ return FromASCII(iStr.c_str()); }

    static WString WFromASCII(char const*);
    inline static WString FromASCII(WString const& iStr){return iStr;}
    static AString ToASCII(wchar_t const*);
    inline static AString ToASCII(WString const& iStr){return ToASCII(iStr.c_str());}

    static int          ToInt(KString const& iStr);
    static unsigned int ToUInt(KString const& iStr);
    static size_t       ToSizeT(KString const& iStr);
    static float        ToFloat(KString const& iStr);
    static double       ToDouble(KString const& iStr);
    static void const*  ToPtr(KString const& iStr);

    static int          ToInt(WString const& iStr);
    static unsigned int ToUInt(WString const& iStr);
    static size_t       ToSizeT(WString const& iStr);
    static float        ToFloat(WString const& iStr);
    static double       ToDouble(WString const& iStr);
    static void const*  ToPtr(WString const& iStr);

    // Limited to 4kB length
    static String Format(const char* iFormatStr, ...);

  };
#ifdef EXL_CHAR_TYPE_IS_CHAR

  inline bool StringUtil::IsSpace(Char iChar)
  {
    return isspace(iChar) != 0;
  }
  inline String StringUtil::FromASCII(char const* iStr)
  {
    return String(iStr);
  }
  inline String StringUtil::FromInt(int iVal)
  {
    return StringUtil::AFromInt(iVal);
  }
  inline String StringUtil::FromInt(unsigned int iVal)
  {
    return StringUtil::AFromInt(iVal);
  }
  inline String StringUtil::FromSizeT(size_t iVal)
  {
    return StringUtil::AFromSizeT(iVal);
  }
  inline String StringUtil::FromFloat(float iVal)
  {
    return StringUtil::AFromFloat(iVal);
  }
  inline String StringUtil::FromFloat(double iVal)
  {
    return StringUtil::AFromFloat(iVal);
  }
  inline String StringUtil::FromPtr(const void* iVal)
  {
    return StringUtil::AFromPtr(iVal);
  }
#endif
#ifdef EXL_CHAR_TYPE_IS_WCHAR

  inline bool StringUtil::IsSpace(wchar_t iChar)
  {
    return iswspace(iChar) != 0;
  }

  inline String StringUtil::FromASCII(char const* iStr)
  {
    return WFromASCII(iStr);
  }

  inline String StringUtil::FromInt(int iVal)
  {
    return StringUtil::WFromInt(iVal);
  }
  inline String StringUtil::FromInt(unsigned int iVal)
  {
    return StringUtil::WFromInt(iVal);
  }
  inline String StringUtil::FromSizeT(size_t iVal)
  {
    return StringUtil::WFromSizeT(iVal);
  }
  inline String StringUtil::FromFloat(float iVal)
  {
    return StringUtil::WFromFloat(iVal);
  }
  inline String StringUtil::FromFloat(double iVal)
  {
    return StringUtil::WFromFloat(iVal);
  }
  inline String StringUtil::FromPtr(const void* iVal)
  {
    return StringUtil::WFromPtr(iVal);
  }
#endif

  template <typename TChar,
    typename TTraits,
    typename TAlloc1, typename TAlloc2>
    bool operator==(std::basic_string<TChar, TTraits, TAlloc1> const & s1,
      std::basic_string<TChar, TTraits, TAlloc2> const & s2)
  {
    return s1.length() == s2.length() &&
      std::equal(s1.begin(), s1.end(), s2.begin());
  }
}