/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/resource/resource.hpp>
#include <boost/uuid/random_generator.hpp>
#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

namespace eXl
{
  IMPLEMENT_RefCCustom(Resource);
  IMPLEMENT_RTTI(Resource);

  Resource::~Resource()
  {}

  String Resource::UUID::ToString() const
  {
    String uuidStr("(");
    uuidStr.append(StringUtil::FromInt(uuid_dwords[0]));
    uuidStr.append(StringUtil::FromInt(uuid_dwords[1]));
    uuidStr.append(StringUtil::FromInt(uuid_dwords[2]));
    uuidStr.append(StringUtil::FromInt(uuid_dwords[3]));
    uuidStr.append(")");

    return uuidStr;
  }

  Err Resource::UUID::Stream(Streamer& iStreamer) const
  {
    iStreamer.BeginSequence();
    iStreamer.Write(uuid_dwords + 0);
    iStreamer.Write(uuid_dwords + 1);
    iStreamer.Write(uuid_dwords + 2);
    iStreamer.Write(uuid_dwords + 3);
    iStreamer.EndSequence();

    return Err::Success;
  }

  Err Resource::UUID::Unstream(Unstreamer& iStreamer)
  {
    if (iStreamer.BeginSequence())
    {
      uint32_t idx = 0;
      do
      {
        eXl_ASSERT_REPAIR_RET(idx < 4, Err::Failure);
        iStreamer.Read(uuid_dwords + idx);
        ++idx;
      } while (iStreamer.NextSequenceElement());
    }
    return Err::Success;
  }

  Err Resource::Header::Stream(Streamer& iStreamer) const
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("UUID");
    m_ResourceId.Stream(iStreamer);
    iStreamer.PopKey();
    iStreamer.PushKey("Name");
    iStreamer.Write(&m_ResourceName);
    iStreamer.PopKey();
    iStreamer.PushKey("LoaderName");
    iStreamer.Write(&m_LoaderName.get());
    iStreamer.PopKey();
    iStreamer.PushKey("LoaderVersion");
    iStreamer.Write(&m_LoaderVersion);
    iStreamer.PopKey();
    iStreamer.PushKey("Hash");
    iStreamer.Write(&m_ResourceHash);
    iStreamer.PopKey();
    iStreamer.PushKey("Flags");
    iStreamer.Write(&m_Flags);
    iStreamer.PopKey();
    iStreamer.EndStruct();
    return Err::Success;
  }

  Err Resource::Header::Unstream(Unstreamer& iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("UUID");
    m_ResourceId.Unstream(iStreamer);
    iStreamer.PopKey();
    iStreamer.PushKey("Name");
    iStreamer.Read(&m_ResourceName);
    iStreamer.PopKey();

    iStreamer.PushKey("LoaderName");

    String loaderName;
    iStreamer.Read(&loaderName);

    m_LoaderName = ResourceLoaderName(loaderName);

    iStreamer.PopKey();
    iStreamer.PushKey("LoaderVersion");
    iStreamer.Read(&m_LoaderVersion);
    iStreamer.PopKey();
    iStreamer.PushKey("Hash");
    iStreamer.Read(&m_ResourceHash);
    iStreamer.PopKey();
    iStreamer.PushKey("Flags");
    iStreamer.Read(&m_Flags);
    iStreamer.PopKey();
    iStreamer.EndStruct();
    return Err::Success;
  }

  Err Resource::Stream(Streamer& iStreamer) const
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Header");
    GetHeader().Stream(iStreamer);
    iStreamer.PopKey();

    iStreamer.PushKey("Data");
    Stream_Data(iStreamer);
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err Resource::Unstream(Unstreamer& iStreamer)
  {
    iStreamer.BeginStruct();
    //iStreamer.PushKey("Header");
    //m_Header.Unstream(iStreamer);
    //iStreamer.PopKey();

    iStreamer.PushKey("Data");
    Unstream_Data(iStreamer);
    iStreamer.PopKey();
    iStreamer.EndStruct();

    PostLoad();

    return Err::Success;
  }

  void Resource::PostLoad()
  {

  }

  Err UnstreamResourceHandle(Resource::UUID& oUUID, Rtti const& iRtti, Unstreamer& iUnstreamer)
  { 
    Err err = Err::Failure;
    if (err = iUnstreamer.BeginStruct())
    {
      if (err = iUnstreamer.PushKey("ResourceType"))
      {
        String tempVal;
        err = iUnstreamer.ReadString(&tempVal);
        if (StringUtil::ToASCII(tempVal) != iRtti.GetName())
        {
          LOG_ERROR << "Wrong resource type : " << tempVal << " expected " << iRtti.GetName() << "\n";
          err = Err::Error;
        }
        iUnstreamer.PopKey();
      }
      if (err && (err = iUnstreamer.PushKey("ResourceID")))
      {
        err = oUUID.Unstream(iUnstreamer);  
        iUnstreamer.PopKey();
      }

      iUnstreamer.EndStruct();
    }

    return err;
  }

  Err StreamResourceHandle(Resource::UUID const& oUUID, Rtti const& iRtti, Streamer& iStreamer)
  {
    Err err = iStreamer.BeginStruct();
    if (err)
      err = iStreamer.PushKey("ResourceType");
    if (err)
      err = iStreamer.WriteString(iRtti.GetName());
    if (err)
      err = iStreamer.PopKey();
    if (err)
      err = iStreamer.PushKey("ResourceID");
    if (err)
      err = oUUID.Stream(iStreamer);
    if (err)
      err = iStreamer.PopKey();
    if (err)
      err = iStreamer.EndStruct();

    return err;
  }
}