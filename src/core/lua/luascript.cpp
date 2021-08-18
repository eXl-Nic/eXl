/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/lua/luascript.hpp>
#include <core/stream/serializer.hpp>
#include <xxhash.h>

#include <boost/uuid/uuid_io.hpp>
#include <sstream>

#include <core/stream/jsonstreamer.hpp>
#include <core/stream/jsonunstreamer.hpp>

#define CLEAR_WHITESPACES(iReader, statement)  do {\
if (!iReader.ClearWhiteSpaces()) \
{                                \
  statement;                     \
} \
}while(false)

#define EXPECT_STRING(iReader, s, statement) do{\
for (char c : s)           \
{                          \
  if (iReader.get() != c)  \
  {                        \
    iReader.reset();       \
    statement;             \
  }                        \
} \
}while(false)

namespace eXl
{
  IMPLEMENT_RTTI(LuaScript);

  LuaScript::LuaScript(ResourceMetaData& iData)
    : Resource(iData)
  {
    //String uuidStr(boost::uuids::to_string(GetHeader().m_ResourceId.uuid).c_str());
    //m_ModuleName = "LuaScript_" + uuidStr;
  }

  uint32_t LuaScript::ComputeHash()
  {
    return XXH32(m_Script.data(), m_Script.length(), 0);
  }

  Err LuaScript::Stream_Data(Streamer& iStreamer) const
  {
    Serializer serializer(iStreamer);
    return const_cast<LuaScript*>(this)->Serialize(serializer);
  }

  Err LuaScript::Unstream_Data(Unstreamer& iStreamer)
  {
    Serializer serializer(iStreamer);
    return Serialize(serializer);
  }

  Err LuaScript::Serialize(Serializer& iSerialize)
  {
    return Err::Success;
  }

  char const* const LuaScriptLoader::s_HeaderSection = "--eXlHeader : ";
  char const* const LuaScriptLoader::s_DataSection = "--scriptData : ";

  Resource* LuaScriptLoader::Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Reader& iReader) const
  {
    if (auto textReader = TextReader::DynamicCast(&iReader))
    {
      CLEAR_WHITESPACES((*textReader), return nullptr);
      EXPECT_STRING((*textReader), KString(s_HeaderSection), return nullptr);
      while(textReader->good() && !textReader->eof() && textReader->get() != '\n')
      { }
      CLEAR_WHITESPACES((*textReader), return nullptr);
      EXPECT_STRING((*textReader), KString(s_DataSection), return nullptr);
      String dataSection;
      while (textReader->good() && !textReader->eof() && textReader->peek() != '\n')
      {
        dataSection.push_back(textReader->get());
      }
      CLEAR_WHITESPACES((*textReader), return nullptr);

      Resource* newRsc = Create_Impl(iMetaData);
      LuaScript* newScript = LuaScript::DynamicCast(newRsc);

      if (newScript == nullptr)
      {
        eXl_DELETE newRsc;
        return nullptr;
      }

      StringViewReader dataReader(iHeader.m_ResourceName, dataSection.c_str(), dataSection.c_str() + dataSection.size());
      JSONUnstreamer unstreamer(&dataReader);
      unstreamer.Begin();
      newScript->Unstream_Data(unstreamer);
      unstreamer.End();

      while (textReader->good() && !textReader->eof())
      {
        newScript->m_Script.push_back(textReader->get());
      }

      return newScript;
    }

    return nullptr;
  }

  Err LuaScriptLoader::Save(Resource* iRsc, Writer& iWriter) const
  {
    if (auto stdOut = StdOutWriter::DynamicCast(&iWriter))
    {
      LuaScript* script = LuaScript::DynamicCast(iRsc);
      if (script == nullptr)
      {
        return Err::Failure;
      }

      Resource::Header const& header = script->GetHeader();
      String headerRep(s_HeaderSection);
      {
        std::stringstream headerStr;
        JSONStreamer headerStreamer(&headerStr);
        headerStreamer.Begin();
        header.Stream(headerStreamer);
        headerStreamer.End();
        headerRep.append(headerStr.str());
      }
      for(auto& c : headerRep)
      {
        if (c == '\n' || c == '\r' || c == '\t')
        {
          c = ' ';
        }
      }
      headerRep.append("\n");

      headerRep.append(s_DataSection);
      {
        std::stringstream scriptDataStr;
        JSONStreamer dataStreamer(&scriptDataStr);
        dataStreamer.Begin();
        script->Stream_Data(dataStreamer);
        dataStreamer.End();
        for (auto c : scriptDataStr.str())
        {
          if (c == '\n' || c == '\r' || c == '\t')
          {
            headerRep.push_back(' ');
          }
          else
          {
            headerRep.push_back(c);
          }
        }
      }
      headerRep.append("\n\n");

      stdOut->m_Stream << headerRep;
      stdOut->m_Stream << script->m_Script;

      return Err::Success;
    }

    return Err::Failure;
  }
}