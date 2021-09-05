#pragma once

#include <core/resource/resource.hpp>
#include <engine/gfx/tileset.hpp>

#include <QPixmap>
#include <QImage>

namespace eXl
{
  class TilingGroup;
  class Archetype;

  struct MiniatureCache
  {
    using ObjectKey = std::pair<Resource::UUID, Name>;
    using ImageKey = std::pair<Resource::UUID, ImageName>;
    UnorderedMap<ImageKey, QImage> m_CacheTileset;
    UnorderedMap<ObjectKey, QPixmap> m_CacheMiniature;

    QPixmap GetMiniature(Resource::UUID const& iUUID, Name iSubobject);
    QPixmap GetMiniature(ResourceHandle<Tileset> const& iHandle, TileName iName);
    QPixmap GetMiniature(ResourceHandle<Archetype> const& iArchetype);
    QPixmap GetMiniature(ResourceHandle<TilingGroup> const& iGroup);
  };
}