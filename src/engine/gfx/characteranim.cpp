/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/characteranim.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/log.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(CharacterAnimation)

  using CharAnimLoader = TResourceLoader <CharacterAnimation, ResourceLoader>;

  void CharacterAnimation::Init()
  {
    ResourceManager::AddLoader(&CharAnimLoader::Get(), CharacterAnimation::StaticRtti());
  }

  ResourceLoaderName CharacterAnimation::StaticLoaderName()
  {
    return ResourceLoaderName("CharacterAnimation");
  }

  CharacterAnimation::CharacterAnimation(ResourceMetaData& iMetaData)
    : Resource(iMetaData)
  {

  }

  Err CharacterAnimation::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<CharacterAnimation*>(this)->Serialize(Serializer(iStreamer));
  }

  Err CharacterAnimation::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  Err CharacterAnimation::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Animations");
    iStreamer.HandleMap(m_Anims);
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  uint32_t CharacterAnimation::ComputeHash()
  {
    return 0;
  }
}