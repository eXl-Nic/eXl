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

  class EXL_CORE_API Unstreamer : public Stream_Base
  {
  public:
    template <class T>
    inline Err Read(T * iObj);

    virtual Err Begin();
    virtual Err End();

    virtual Err PushKey(String const& iKey) = 0;
    virtual Err PopKey() = 0;

    virtual Err BeginStruct() = 0;
    virtual Err EndStruct() = 0;

    /*Both of these fonction can return Err::Failure, when reaching the end of the array
    Do this : 
    if(BeginSequence())
    {
      do
      {

      }while(NextSequenceElement());
    }
    */
    virtual Err BeginSequence() = 0;
    virtual Err NextSequenceElement() = 0;

    virtual Err ReadInt(int * oInt) = 0;
    virtual Err ReadUInt(unsigned int * oUInt) = 0;
    virtual Err ReadUInt64(uint64_t * oUInt) = 0;
    virtual Err ReadFloat(float * oFloat) = 0;
    virtual Err ReadDouble(double * oDouble) = 0;
    virtual Err ReadString(String* oStr) = 0;
  };

  template <typename T>
  struct UnstreamerTemplateHandler;

  template <typename T>
  struct UnstreamerTemplateHandler
  {
    static Err Do(Unstreamer& iUnstreamer, T* iObj)
    {
      return TypeTraits::Unstream<T>(iObj, iUnstreamer);
    }
  };

  //template <>
  //struct UnstreamerTemplateHandler<Name>
  //{
  //  static Err Do(Unstreamer& iUnstreamer, Name* iObj)
  //  {
  //    String str;
  //    Err readErr = iUnstreamer.ReadString(&str);
  //    if (readErr)
  //    {
  //      *iObj = Name(str.c_str());
  //    }
  //
  //    return readErr;
  //  }
  //};

  template <typename T>
  struct UnstreamerTemplateHandler<Vector<T>>
  {
    static Err Do(Unstreamer& iUnstreamer, Vector<T>* iObj)
    {
      iObj->clear();
      Err seq = iUnstreamer.BeginSequence();
      if((seq))
      {
        do
        {
          T obj;
          iUnstreamer.Read(&obj);
          iObj->emplace_back(std::move(obj));

        }while((seq = iUnstreamer.NextSequenceElement()));
      }
      RETURN_SUCCESS;
    }
  };

  template <>
  inline Err Unstreamer::Read<int>(int * oObj)
  {
    return ReadInt(oObj);
  }

  template <>
  inline Err Unstreamer::Read<unsigned int>(unsigned int * oObj)
  {
    return ReadUInt(oObj);
  }

  template <>
  inline Err Unstreamer::Read<uint64_t>(uint64_t * oObj)
  {
    return ReadUInt64(oObj);
  }

  template <>
  inline Err Unstreamer::Read<uint16_t>(uint16_t * oObj)
  {
    uint32_t temp;
    Err res = ReadUInt(&temp);
    if (res)
    {
      *oObj = temp;
    }
    return res;
  }

  template <>
  inline Err Unstreamer::Read<float>(float * oObj)
  {
    return ReadFloat(oObj);
  }

  template <>
  inline Err Unstreamer::Read<double>(double * oObj)
  {
    return ReadDouble(oObj);
  }

  template <>
  inline Err Unstreamer::Read<AString>(AString * oObj)
  {
    return ReadString(oObj);
  }

  template <>
  inline Err Unstreamer::Read<KString>(KString * oObj)
  {
    eXl_ASSERT(false);
    return Err::Error;
  }

  template <>
  inline Err Unstreamer::Read<Name>(Name * oName)
  {
    String temp;
    Err readErr = ReadString(&temp);
    if (readErr)
    {
      *oName = temp;
    }

    return readErr;
  }

  template <>
  inline Err Unstreamer::Read<bool>(bool* oObj)
  {
    String tempStr;
    if(ReadString(&tempStr))
    {
      if(tempStr == "true")
        *oObj = true;
      else if(tempStr == "false")
        *oObj = false;
      else
        return Err::Failure;
    }
    return Err::Success;
  }

  template <>
  inline Err Unstreamer::Read<unsigned char>(unsigned char* oObj)
  {
    unsigned int temp;
    Err err = ReadUInt(&temp);
    if(err)
    {
      *oObj = temp;
    }
    return err;
  }

  template <class T>
  inline Err Unstreamer::Read(T* iObj)
  {
    return UnstreamerTemplateHandler<T>::Do(*this, iObj);
  }
}

#include <core/type/typetraits.hpp>

namespace eXl
{
  namespace TypeTraits
  {
    //template <>
    //EXL_CORE_API Err Unstream<IntrusivePtr<RttiObjectRefC> >(IntrusivePtr<RttiObjectRefC> * oObj, Unstreamer& iUnstreamer);

    template <>
    inline Err Unstream<void* >(void* * oObj, Unstreamer& iUnstreamer)
    {
      return Err::Undefined;
    }
  }
}
