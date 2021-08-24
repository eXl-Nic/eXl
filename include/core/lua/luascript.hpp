/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/resource/resource.hpp>
#include <core/resource/resourceloader.hpp>

namespace eXl
{
  class Serializer;
  class EXL_CORE_API LuaScript : public Resource
  {
    DECLARE_RTTI(LuaScript, Resource);
  public:
    AString m_Script;

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

  protected:
    uint32_t ComputeHash() override;
    Err Serialize(Serializer& iSerialize);
    LuaScript(ResourceMetaData&);

  };

  class EXL_CORE_API LuaScriptLoader : public ResourceLoader
  {
  public:

    static char const* const s_HeaderSection;
    static char const* const s_DataSection;

    Err Save(Resource* iRsc, Writer& iWriter) const override;
    Resource* Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Reader& iStreamer) const override;
  protected:
    LuaScriptLoader(ResourceLoaderName iName, uint32_t iVersion)
      : ResourceLoader(iName, iVersion)
    {}
  };

  template <typename T>
  using LuaScriptLoader_T = TResourceLoader<T, LuaScriptLoader>;
}