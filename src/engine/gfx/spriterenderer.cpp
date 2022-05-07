#include <engine/gfx/spriterenderer.hpp>

#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/oglutils.hpp>

#define VALIDATE_FLOAT(floatValue) eXl_ASSERT(!std::isnan(floatValue) && std::isfinite(floatValue));

namespace eXl
{
  SpriteRenderer::SpriteRenderer(GfxSystem& iSys, GameDataView<GfxSpriteComponent::Desc> const& iSpriteDescView, DenseGameDataView<GfxSpriteData>& iSpriteData)
    : m_SpriteDescView(iSpriteDescView)
    , m_SpriteData(iSpriteData)
    , m_Transforms(iSys.GetTransforms())
  {
    m_SpriteProgram.reset(OGLSpriteAlgo::CreateSpriteProgram(iSys.GetSemanticManager()));
  }

  SpriteRenderer::~SpriteRenderer() = default;

  void SpriteRenderer::PrepareSprites()
  {
    for (ObjectHandle obj : m_DirtyComponents)
    {
      if (m_DefaultSpriteIdxBuffer == nullptr)
      {
        m_DefaultSpriteIdxBuffer = GfxSpriteData::MakeSpriteIdxBuffer();
      }

      GfxSpriteComponent::Desc const* descData = m_SpriteDescView.Get(obj);

      if (descData == nullptr)
      {
        m_SpriteData.Erase(obj);
        continue;
      }

      GfxSpriteData& data = m_SpriteData.GetOrCreate(obj);
      data.m_RemainingTime = -1.0;
      data.m_CurScale = One<Vec2>();
      data.m_CurOffset = Zero<Vec2>();
      data.m_Rotate = descData->m_RotateSprite;
      data.m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), data.m_Billboard ? &data.m_BillboardTransform : &data.m_Transform);
      data.m_Layer = descData->m_Layer;
      data.m_SpriteInfo.tint = descData->m_Tint;

      if (descData->m_Tileset.GetUUID().IsValid()
        && !descData->m_Tileset.IsLoaded())
      {
        descData->m_Tileset.Load();
      }
      Tileset const* tileset = descData->m_Tileset.Get();

      if (tileset == nullptr)
      {
        data.m_Texture = nullptr;

        continue;
      }

      Vec2i imageSize = One<Vec2i>();
      Vec2i tileSize = One<Vec2i>();
      Vec2i tileOffset = Zero<Vec2i>();

      if (Tile const* tile = tileset->Find(descData->m_TileName))
      {
        data.m_Texture = tileset->GetTexture(tile->m_ImageName);
        if (data.m_Texture == nullptr)
        {
          LOG_WARNING << "Used unknown tile " << tile->m_ImageName;
          continue;
        }
        data.m_TextureData.~OGLShaderData();
        new(&data.m_TextureData) OGLShaderData();
        data.m_TextureData.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), data.m_Texture);
        data.m_TextureData.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(), data.m_Texture);
        data.m_TextureData.AddData(OGLSpriteAlgo::GetSpriteColorUniform(), &data.m_SpriteInfo);

        data.m_CurScale = Vec2(tile->m_Size.x, tile->m_Size.y);
        data.m_CurScale.x *= tile->m_Scale.x;
        data.m_CurScale.y *= tile->m_Scale.y;

        VALIDATE_FLOAT(tile->m_Scale.x);
        VALIDATE_FLOAT(tile->m_Scale.y);

        data.m_CurOffset = tile->m_Offset;

        imageSize = tileset->GetImageSize(tile->m_ImageName);

        if (imageSize.x == 0 || imageSize.y == 0)
        {
          LOG_ERROR << tile->m_ImageName << "has 0 size!!";
        }

        tileSize = tile->m_Size;

        if (!tile->m_Frames.empty())
        {
          tileOffset = tile->m_Frames[0];
          float frameTime = tile->m_FrameDuration / descData->m_AnimSpeed;
          if (tile->m_Frames.size() > 1 && frameTime > 0 && frameTime < Mathf::MaxReal())
          {
            data.m_CurrentFrame = 0;
            data.m_Forward = true;
            data.m_Loop = tile->m_AnimType != AnimationType::None;
            data.m_RemainingTime = frameTime;
          }
        }
        else
        {
          tileOffset = Zero<Vec2i>();
        }
      }
      else
      {
        LOG_WARNING << "Used unknown tile " << descData->m_TileName.get() << "\n";

        data.m_Texture = nullptr;
        continue;
      }

      data.m_Transform = Identity<Mat4>();

      Mat4 const& worldTrans = m_Transforms.GetWorldTransform(obj);
      if (data.m_Rotate)
      {
        data.m_Transform = translate(worldTrans, Vec3(descData->m_Offset + data.m_CurOffset, 0));
      }
      else
      {
        data.m_Transform = translate(Identity<Mat4>(), Vec3(worldTrans[3]) + Vec3(descData->m_Offset + data.m_CurOffset, 0));
      }
      data.m_Transform = scale(data.m_Transform, Vec3(data.m_CurScale, 1));

      Vec3 cacheKey = Vec3(descData->m_Size, descData->m_Flat ? 0.0f : 1.0f);

      auto cacheIter = m_SpriteGeomCache.find(cacheKey);
      if (cacheIter == m_SpriteGeomCache.end())
      {
        IntrusivePtr<GeometryInfo> geom(eXl_NEW GeometryInfo);
        geom->m_Vertices = GfxSpriteData::MakeSpriteGeometry(descData->m_Size, descData->m_Flat);
        geom->m_Indices = m_DefaultSpriteIdxBuffer;
        geom->SetupAssembly(true);
        cacheIter = m_SpriteGeomCache.insert(std::make_pair(cacheKey, geom)).first;
      }

      IntrusivePtr<GeometryInfo> geom = cacheIter->second;

      Vec2 texStep(1.0f / imageSize.x, 1.0f / imageSize.y);

      data.m_Geometry = geom;
      data.m_SpriteInfo.alphaMult = 1.0;
      VALIDATE_FLOAT(texStep.x);
      VALIDATE_FLOAT(texStep.y);
      data.m_SpriteInfo.tcOffset = Vec2(tileOffset.x * texStep.x, tileOffset.y * texStep.y);
      data.m_SpriteInfo.tcScaling = Vec2(tileSize.x * texStep.x, tileSize.y * texStep.y);
      data.m_SpriteInfo.imageSize = Vec2(imageSize);
    }
    m_DirtyComponents.clear();
  }

  void SpriteRenderer::TickAnimation(ObjectHandle object, GfxSpriteData& data, float iDelta)
  {
    if (!data.m_Texture)
    {
      return;
    }

    if (data.m_RemainingTime > 0.0)
    {
      data.m_RemainingTime -= iDelta;
      if (data.m_RemainingTime <= 0)
      {
        GfxSpriteComponent::Desc const* descData = m_SpriteDescView.GetConst(object);
        if (descData == nullptr)
        {
          return;
        }

        Tileset const* tileset = descData->m_Tileset.Get();
        Tile const* tile = tileset->Find(descData->m_TileName);
        if (tile)
        {
          bool lastFrame = data.m_Forward ?
            tile->m_Frames.size() - 1 == data.m_CurrentFrame :
            0 == data.m_CurrentFrame;

          Vec2i size = tileset->GetImageSize(tile->m_ImageName);

          Vec2 texStep(1.0f / size.x, 1.0f / size.y);

          bool updateAnim = !lastFrame;
          if (lastFrame)
          {
            switch (tile->m_AnimType)
            {
            case AnimationType::None:
              break;
            case AnimationType::Loop:
              data.m_CurrentFrame = 0;
              updateAnim = true;
              break;
            case AnimationType::Pingpong:
              data.m_Forward = !data.m_Forward;
              data.m_CurrentFrame += data.m_Forward ? 1 : -1;
              updateAnim = true;
              break;
            }
          }
          else
          {
            data.m_CurrentFrame += data.m_Forward ? 1 : -1;
          }

          if (updateAnim)
          {
            Vec2i offsetPix = tile->m_Frames[data.m_CurrentFrame];
            data.m_RemainingTime = tile->m_FrameDuration / descData->m_AnimSpeed;
            data.m_SpriteInfo.tcOffset = Vec2(offsetPix.x * texStep.x, offsetPix.y * texStep.y);
          }
        }
      }
    }
  }

  void SpriteRenderer::UpdateTransforms(ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum)
  {
    for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
    {
      if (auto* descData = m_SpriteDescView.Get(*iObjects))
      {
        if (GfxSpriteData* data = m_SpriteData.Get(*iObjects))
        {
          if (data->m_Rotate)
          {
            data->m_Transform = translate((**iTransforms), Vec3(descData->m_Offset + data->m_CurOffset, 0));
          }
          else
          {
            data->m_Transform = translate(Identity<Mat4>(), Vec3((**iTransforms)[3]) + Vec3(descData->m_Offset + data->m_CurOffset, 0));
          }
          data->m_Transform = scale(data->m_Transform, Vec3(data->m_CurScale, 1));
        }
      }
    }
  }
}