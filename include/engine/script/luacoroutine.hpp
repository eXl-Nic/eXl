/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/lua/luascript.hpp>
#include <core/path.hpp>
#include <engine/enginelib.hpp>

namespace eXl
{
  class LuaFunctionLibrary;

  class EXL_ENGINE_API LuaCoroutine : public LuaScript
  {
    DECLARE_RTTI(LuaCoroutine, LuaScript);
  public:

    static void Init();
    static ResourceLoaderName StaticLoaderName();

#ifdef EXL_RSC_HAS_FILESYSTEM
    static LuaCoroutine* Create(Path const& iPath, String const& iName);
#endif

    LuaCoroutine(ResourceMetaData&);
    ~LuaCoroutine();

    bool m_DefaultStartPaused = false;
    float m_DefaultTickRate = 1.0f / 1000.f;

    Vector<ResourceHandle<LuaFunctionLibrary>> m_Dependencies;
  protected:
    Err Serialize(Serializer iStreamer);
    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
  };
}