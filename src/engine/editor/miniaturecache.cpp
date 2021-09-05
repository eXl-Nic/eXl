#include "miniaturecache.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include <core/resource/resourcemanager.hpp>

#include <engine/map/tilinggroup.hpp>
#include <engine/game/archetype.hpp>
#include <engine/game/commondef.hpp>
#include <engine/gfx/gfxcomponent.hpp>

namespace eXl
{
  QPixmap MiniatureCache::GetMiniature(Resource::UUID const& iUUID, Name iSubobject)
  {
    Resource::Header const* header = ResourceManager::GetHeader(iUUID);
    if (header == nullptr)
    {
      return QPixmap();
    }
    if (header->m_LoaderName == Tileset::StaticLoaderName())
    {
      ResourceHandle<Tileset> handle;
      handle.SetUUID(iUUID);
      return GetMiniature(handle, TileName(iSubobject.get()));
    }

    if (header->m_LoaderName == TilingGroup::StaticLoaderName())
    {
      ResourceHandle<TilingGroup> handle;
      handle.SetUUID(iUUID);
      return GetMiniature(handle);
    }

    if (header->m_LoaderName == Archetype::StaticLoaderName())
    {
      ResourceHandle<Archetype> handle;
      handle.SetUUID(iUUID);
      return GetMiniature(handle);
    }

    return QPixmap();
  }

  QPixmap MiniatureCache::GetMiniature(ResourceHandle<Tileset> const& iHandle, TileName iName)
  {
    Tileset const* tileset = iHandle.GetOrLoad();
    eXl_ASSERT_REPAIR_RET(tileset != nullptr, QPixmap());
    Tile const* tile = tileset->Find(iName);
    eXl_ASSERT_REPAIR_RET(tile != nullptr, QPixmap());

    Vector2i size = tile->m_Size;

    ImageKey key = { iHandle.GetUUID(), tile->m_ImageName };

    auto iter = m_CacheTileset.find(key);
    if (iter == m_CacheTileset.end())
    {
      if (Image const* img = tileset->GetImage(tile->m_ImageName))
      {
        QImage temp;
        eXlImageToQImage(*img, temp);
        iter = m_CacheTileset.emplace(std::make_pair(key, std::move(temp))).first;
      }
      else
      {
        return QPixmap();
      }
    }
    Vector2i orig = tile->m_Frames.size() > 0 ? tile->m_Frames[0] : Vector2i::ZERO;
    QRect tileDim(QPoint(orig.X(), orig.Y()), QSize(size.X(), size.Y()));

    QPixmap miniature = QPixmap::fromImage(iter->second.copy(tileDim));

    ObjectKey objKey{ iHandle.GetUUID(), iName };
    m_CacheMiniature.insert(std::make_pair(objKey, miniature));

    return miniature;
  }

  QPixmap MiniatureCache::GetMiniature(ResourceHandle<Archetype> const& iArchetype)
  {
    Archetype const* archetype = iArchetype.GetOrLoad();
    eXl_ASSERT_REPAIR_RET(archetype != nullptr, QPixmap());

    auto const& components = archetype->GetComponents();
    auto iterGfx = components.find(EngineCommon::GfxSpriteComponentName());
    if (iterGfx == components.end())
    {
      return QPixmap();
    }
    auto const* gfxDesc = iterGfx->second.CastBuffer<GfxSpriteComponent::Desc>();

    QPixmap miniature = GetMiniature(gfxDesc->m_Tileset, gfxDesc->m_TileName);

    if (!miniature.isNull())
    {
      ObjectKey key{ iArchetype.GetUUID(), Name() };
      m_CacheMiniature.insert(std::make_pair(key, miniature));
    }

    return miniature;
  }

  QPixmap MiniatureCache::GetMiniature(ResourceHandle<TilingGroup> const& iGroup)
  {
    TilingGroup const* group = iGroup.GetOrLoad();
    eXl_ASSERT_REPAIR_RET(group != nullptr, QPixmap());

    QPixmap miniature = GetMiniature(group->GetTileset(), group->m_DefaultTile);

    if (!miniature.isNull())
    {
      ObjectKey key{ iGroup.GetUUID(), Name() };
      m_CacheMiniature.insert(std::make_pair(key, miniature));
    }

    return miniature;
  }
}