#pragma once

#include <core/lua/luascript.hpp>
#include <core/path.hpp>
#include <engine/enginelib.hpp>

namespace eXl
{
  class EXL_ENGINE_API LuaScriptBehaviour : public LuaScript
  {
    DECLARE_RTTI(LuaScriptBehaviour, LuaScript);
  public:

    static void Init();
    static ResourceLoaderName StaticLoaderName();

#ifdef EXL_RSC_HAS_FILESYSTEM
    static LuaScriptBehaviour* Create(Path const& iPath, String const& iName);
#endif

    Name m_BehaviourName;

    LuaScriptBehaviour(ResourceMetaData&);
  protected:
    Err Serialize(Serializer iStreamer);
    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
  };
}