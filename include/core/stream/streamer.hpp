/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/type/typetraits.hpp>
#include <core/name.hpp>
#include <core/stream/stream_base.hpp>

namespace eXl
{
  class Rtti;
  class RttiObject;

  class EXL_CORE_API Streamer : public Stream_Base
  {
  public:

    Streamer();

    template <class T>
    inline Err Write(T const* iObj);

    template <typename Iterator>
    inline Err WriteSequence(Iterator begin, Iterator end);

    virtual Err Begin();
    virtual Err End();

    virtual Err BeginSequence(/*unsigned int iSize*/) = 0;
    virtual Err EndSequence() = 0;

    virtual Err BeginStruct() = 0;
    virtual Err EndStruct() = 0;

    virtual Err PushKey(String const& iKey) = 0;
    virtual Err PopKey() = 0;

    virtual Err WriteInt(int const* iInt) = 0;
    virtual Err WriteUInt(unsigned int const* iUInt) = 0;
    virtual Err WriteUInt64(uint64_t const* iUInt) = 0;
    virtual Err WriteFloat(float const* iFloat) = 0;
    virtual Err WriteDouble(double const* iDouble) = 0;
    virtual Err WriteString(Char const* iStr) = 0;
    virtual Err WriteString(KString const& iStr) = 0;
    inline  Err WriteString(String const& iStr)
    {return WriteString(iStr.c_str());}
  };

  template <typename T>
  struct StreamerTemplateHandler;

  template <class T>
  inline Err Streamer::Write(T const* iObj)
  {
    return StreamerTemplateHandler<T>::Do(*this, iObj);
  }

  template <typename T>
  struct StreamerTemplateHandler
  {
    static Err Do(Streamer& iStreamer, T const* iObj)
    {
      return TypeTraits::Stream<T>(iObj, iStreamer);
    }
  };

  template <>
  struct StreamerTemplateHandler<Name>
  {
    static Err Do(Streamer& iStreamer, Name const* iObj)
    {
      return iStreamer.WriteString(iObj->get());
    }
  };

  template <typename Iterator>
  inline Err Streamer::WriteSequence(Iterator begin, Iterator end)
  {
    BeginSequence();
    for(; begin != end; ++begin)
    {
      Write(&(*begin));
    }
    EndSequence();
    RETURN_SUCCESS;
  }

  template <typename T>
  struct StreamerTemplateHandler<Vector<T>>
  {
    static Err Do(Streamer& iStreamer, Vector<T> const* iObj)
    {
      return iStreamer.WriteSequence(iObj->begin(), iObj->end());
    }
  };

  template <>
  inline Err Streamer::Write<int32_t>(int32_t const* iObj)
  {
    return WriteInt(iObj);
  }

  template <>
  inline Err Streamer::Write<uint16_t>(uint16_t const* iObj)
  {
    uint32_t temp = *iObj;
    return WriteUInt(&temp);
  }

  template <>
  inline Err Streamer::Write<uint32_t>(uint32_t const* iObj)
  {
    return WriteUInt(iObj);
  }

  template <>
  inline Err Streamer::Write<uint64_t>(uint64_t const* iObj)
  {
    return WriteUInt64(iObj);
  }

  template <>
  inline Err Streamer::Write<float>(float const* iObj)
  {
    return WriteFloat(iObj);
  }

  template <>
  inline Err Streamer::Write<double>(double const* iObj)
  {
    return WriteDouble(iObj);
  }

  template <>
  inline Err Streamer::Write<Char const*>(Char const* const* iObj)
  {
    return WriteString(*iObj);
  }

  //template <>
  //inline Err Streamer::Write<WString>(WString const* iObj)
  //{
  //  return WriteString(iObj->c_str());
  //}

  template <>
  inline Err Streamer::Write<AString>(AString const* iObj)
  {
    return WriteString(*iObj);
  }

  template <>
  inline Err Streamer::Write<KString>(KString const* iObj)
  {
    return WriteString(*iObj);
  }

  template <>
  inline Err Streamer::Write<bool>(bool const* iObj)
  {
    if(*iObj)
      return WriteString("true");
    else
      return WriteString("false");
  }

  template <>
  inline Err Streamer::Write(Name const* iName)
  {
    return WriteString(iName->get());
  }

  template <>
  inline Err Streamer::Write<unsigned char>(unsigned char const* iObj)
  {
    unsigned int tempUint = *iObj;
    return WriteUInt(&tempUint);
  }
}

#include <core/type/typetraits.hpp>

namespace eXl
{
  namespace TypeTraits
  {
    template <>
    inline Err Stream<void* >(void* const* iObj, Streamer& iStreamer)
    {
      return Err::Undefined;
    }
  }
}
