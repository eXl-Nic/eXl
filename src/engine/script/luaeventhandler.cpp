/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/script/luaeventhandler.hpp>
#include <engine/script/luafunctionlibrary.hpp>

#include <core/resource/resourceloader.hpp>
#include <core/stream/serializer.hpp>
#include <boost/optional.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(LuaEventHandler);

  using LuaEventHandlerLoader = TResourceLoader<LuaEventHandler, LuaScriptLoader>;

  void LuaEventHandler::Init()
  {
    ResourceManager::AddLoader(&LuaEventHandlerLoader::Get(), LuaEventHandler::StaticRtti(), GetType());
  }

  ResourceLoaderName LuaEventHandler::StaticLoaderName()
  {
    return ResourceLoaderName("LuaEventHandler");
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  LuaEventHandler* LuaEventHandler::Create(Path const& iPath, String const& iName)
  {
    return LuaEventHandlerLoader::Get().Create(iPath, iName);
  }
#endif

  Err LuaEventHandler::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Interface");
    iStreamer &= m_InterfaceName;
    iStreamer.PopKey();
    iStreamer.PushKey("Dependencies");
    iStreamer &= m_Dependencies;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err LuaEventHandler::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<LuaEventHandler*>(this)->Serialize(iStreamer);
  }

  Err LuaEventHandler::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(iStreamer);
  }

  LuaEventHandler::LuaEventHandler(ResourceMetaData& iMeta)
    : LuaScript(iMeta)
  {

  }
}