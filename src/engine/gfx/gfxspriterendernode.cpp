/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include "gfxspriterendernode.hpp"

#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/oglutils.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>
#include <core/type/tagtype.hpp>

#define VALIDATE_FLOAT(floatValue) eXl_ASSERT(!std::isnan(floatValue) && std::isfinite(floatValue));

namespace eXl
{

  void GfxSpriteRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
    OGLSpriteAlgo::Init(iSys.GetSemanticManager());

    m_SpriteProgram.reset(OGLSpriteAlgo::CreateSpriteProgram(iSys.GetSemanticManager()));
  }

  SpriteDataTable::Handle GfxSpriteRenderNode::GetSpriteData(GfxSpriteComponent& iComp)
  {
    return iComp.m_SpriteData;
  }

  Vector2f const& GfxSpriteRenderNode::GetSpriteOffset(GfxSpriteComponent& iComp)
  {
    return iComp.m_Desc->m_Offset;
  }

  static void GetPositionOnly(Matrix4f const& iMat, Matrix4f& oMat)
  {
    MathTools::GetPosition(oMat) = MathTools::GetPosition(iMat);
  }


  void GfxSpriteRenderNode::PrepareSprites()
  {
    for (GfxSpriteComponent* comp : m_DirtyComponents)
    {
      if (m_DefaultSpriteIdxBuffer == nullptr)
      {
        m_DefaultSpriteIdxBuffer = GfxSpriteData::MakeSpriteIdxBuffer();
      }

      if (!comp->m_SpriteData.IsAssigned())
      {
        comp->m_SpriteData = m_SpriteData.Alloc();
      }
      GfxSpriteData& data = m_SpriteData.Get(comp->m_SpriteData);
      data.m_Component = comp;
      data.m_RemainingTime = -1.0;
      data.m_CurScale = Vector2f::ONE;
      data.m_CurOffset = Vector2f::ZERO;
      data.m_Rotate = comp->m_Desc->m_RotateSprite;
      data.m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), data.m_Billboard ? &data.m_BillboardTransform : &data.m_Transform);
      data.m_Layer = comp->m_Desc->m_Layer;
      data.m_SpriteInfo.tint = comp->m_Desc->m_Tint;

      if (comp->m_Desc->m_Tileset.GetUUID().IsValid()
        && !comp->m_Desc->m_Tileset.IsLoaded())
      {
        comp->m_Desc->m_Tileset.Load();
      }
      Tileset const* tileset = comp->m_Desc->m_Tileset.Get();

      if (tileset == nullptr)
      {
        data.m_Texture = nullptr;

        continue;
      }

      Vector2i imageSize = Vector2i::ONE;
      Vector2i tileSize = Vector2i::ONE;
      Vector2i tileOffset = Vector2i::ZERO;

      if (Tile const* tile = tileset->Find(comp->m_Desc->m_TileName))
      {
        data.m_Texture = tileset->GetTexture(tile->m_ImageName);
        data.m_TextureData.~OGLShaderData();
        new(&data.m_TextureData) OGLShaderData();
        data.m_TextureData.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), data.m_Texture);
        data.m_TextureData.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(), data.m_Texture);
        data.m_TextureData.AddData(OGLSpriteAlgo::GetSpriteColorUniform(), &data.m_SpriteInfo);

        data.m_CurScale = Vector2f(tile->m_Size.X(), tile->m_Size.Y());
        data.m_CurScale.X() *= tile->m_Scale.X();
        data.m_CurScale.Y() *= tile->m_Scale.Y();

        VALIDATE_FLOAT(tile->m_Scale.X());
        VALIDATE_FLOAT(tile->m_Scale.Y());

        data.m_CurOffset = tile->m_Offset;

        imageSize = tileset->GetImageSize(tile->m_ImageName);

        if (imageSize.X() == 0 || imageSize.Y() == 0)
        {
          LOG_ERROR << tile->m_ImageName << "has 0 size!!";
        }

        tileSize = tile->m_Size;

        if (!tile->m_Frames.empty())
        {
          tileOffset = tile->m_Frames[0];
          float frameTime = tile->m_FrameDuration / comp->m_Desc->m_AnimSpeed;
          if (tile->m_Frames.size() > 1 && frameTime > 0 && frameTime < Mathf::MAX_REAL)
          {
            data.m_CurrentFrame = 0;
            data.m_Forward = true;
            data.m_Loop = tile->m_AnimType != AnimationType::None;
            data.m_RemainingTime = frameTime;
          }
        }
        else
        {
          tileOffset = Vector2i::ZERO;
        }
      }
      else
      {
        LOG_WARNING << "Used unknown tile " << comp->m_Desc->m_TileName.get() << "\n";

        data.m_Texture = nullptr;
        continue;
      }

      data.m_Transform = Matrix4f::IDENTITY;

      Matrix4f const& worldTrans = m_Sys->GetTransforms().GetWorldTransform(comp->m_Object);
      if (data.m_Rotate)
      {
        Matrix4f localTrans;
        localTrans.MakeIdentity();
        localTrans.m_Data[0] = data.m_CurScale.X();
        localTrans.m_Data[5] = data.m_CurScale.Y();
        MathTools::GetPosition2D(localTrans) = comp->m_Desc->m_Offset + data.m_CurOffset;

        data.m_Transform = worldTrans * localTrans;
      }
      else
      {
        data.m_Transform.m_Data[0] = data.m_CurScale.X();
        data.m_Transform.m_Data[5] = data.m_CurScale.Y();
        GetPositionOnly(worldTrans, data.m_Transform);
        MathTools::GetPosition2D(data.m_Transform) += comp->m_Desc->m_Offset + data.m_CurOffset;
      }

      Vector3f cacheKey = MathTools::To3DVec(comp->m_Desc->m_Size, comp->m_Desc->m_Flat ? 0.0f : 1.0f);

      auto cacheIter = m_SpriteGeomCache.find(cacheKey);
      if (cacheIter == m_SpriteGeomCache.end())
      {
        IntrusivePtr<GeometryInfo> geom(eXl_NEW GeometryInfo);
        geom->m_Vertices = GfxSpriteData::MakeSpriteGeometry(comp->m_Desc->m_Size, comp->m_Desc->m_Flat);
        geom->m_Indices = m_DefaultSpriteIdxBuffer;
        geom->SetupAssembly(true);
        cacheIter = m_SpriteGeomCache.insert(std::make_pair(cacheKey, geom)).first;
      }

      IntrusivePtr<GeometryInfo> geom = cacheIter->second;

      Vector2f texStep(1.0f / imageSize.X(), 1.0f / imageSize.Y());

      data.m_Geometry = geom;
      data.m_SpriteInfo.alphaMult = 1.0;
      VALIDATE_FLOAT(texStep.X());
      VALIDATE_FLOAT(texStep.Y());
      data.m_SpriteInfo.tcOffset = Vector2f(tileOffset.X() * texStep.X(), tileOffset.Y() * texStep.Y());
      data.m_SpriteInfo.tcScaling = Vector2f(tileSize.X() * texStep.X(), tileSize.Y() * texStep.Y());
      data.m_SpriteInfo.imageSize = MathTools::ToFVec(imageSize);
    }
    m_DirtyComponents.clear();
  }

  void GfxSpriteRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {
    PrepareSprites();
    iList.SetDepth(true, true);
    iList.SetProgram(m_SpriteProgram.get());
    m_SpriteData.Iterate([this, &iList, iDelta](GfxSpriteData& data, SpriteDataTable::Handle)
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
            GfxSpriteComponent* comp = data.m_Component;
            Tileset const* tileset = comp->m_Desc->m_Tileset.Get();
            Tile const* tile = tileset->Find(comp->m_Desc->m_TileName);
            if (tile)
            {
              bool lastFrame = data.m_Forward ?
                tile->m_Frames.size() - 1 == data.m_CurrentFrame :
                0 == data.m_CurrentFrame;

              Vector2i size = tileset->GetImageSize(tile->m_ImageName);

              Vector2f texStep(1.0f / size.X(), 1.0f / size.Y());

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
                Vector2i offsetPix = tile->m_Frames[data.m_CurrentFrame];
                data.m_RemainingTime = tile->m_FrameDuration / comp->m_Desc->m_AnimSpeed;
                data.m_SpriteInfo.tcOffset = Vector2f(offsetPix.X() * texStep.X(), offsetPix.Y() * texStep.Y());
              }
            }
          }
        }

        data.Push(iList);
      });
  }

  GfxRenderNode::TransformUpdateCallback GfxSpriteRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Matrix4f const** iTransforms, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
      {
        SpriteComponents::Handle objHandle = m_ObjectToSpriteComp[iObjects->GetId()];
        if (auto* obj = m_SpriteComp.TryGet(objHandle))
        {
          SpriteDataTable::Handle dataHandle = GetSpriteData(*obj);
          if (dataHandle.IsAssigned())
          {
            GfxSpriteData& data = m_SpriteData.Get(dataHandle);
            if (data.m_Rotate)
            {
              Matrix4f localTrans;
              localTrans.MakeIdentity();
              localTrans.m_Data[0] = data.m_CurScale.X();
              localTrans.m_Data[5] = data.m_CurScale.Y();
              MathTools::GetPosition2D(localTrans) = GetSpriteOffset(*obj) + data.m_CurOffset;

              data.m_Transform = (**iTransforms) * localTrans;
            }
            else
            {
              GetPositionOnly(**iTransforms, data.m_Transform);
              MathTools::GetPosition2D(data.m_Transform) += GetSpriteOffset(*obj) + data.m_CurOffset;
            }
          }
        }
      }
    };
  }

  GfxRenderNode::UpdateCallback GfxSpriteRenderNode::GetDeleteCallback()
  {
    return [this](ObjectHandle const* iObjects, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects)
      {
        RemoveObject(*iObjects);
      }
    };
  }

  GfxSpriteComponent* GfxSpriteRenderNode::GetComponent(ObjectHandle iObject)
  {
    if(iObject.GetId() < m_ObjectToSpriteComp.size())
    {
      return &m_SpriteComp.Get(m_ObjectToSpriteComp[iObject.GetId()]);
    }
    return nullptr;
  }


  void GfxSpriteRenderNode::SetSpriteDirty(GfxSpriteComponent& iComp)
  {
    m_DirtyComponents.insert(&iComp);
  }


  void GfxSpriteRenderNode::AddObject(ObjectHandle iObject)
  {
    SpriteComponents::Handle compHandle = m_SpriteComp.Alloc();
    GfxSpriteComponent& newComp = m_SpriteComp.Get(compHandle);

    GfxSpriteComponent::Desc& desc = GetSpriteComponentView(m_Sys->GetWorld())->GetOrCreate(iObject);

    newComp.m_RenderNode = this;
    newComp.m_Object = iObject;
    newComp.m_Desc = &desc;
    m_DirtyComponents.insert(&newComp);

    while (m_ObjectToSpriteComp.size() <= iObject.GetId())
    {
      m_ObjectToSpriteComp.push_back(SpriteComponents::Handle());
    }
    m_ObjectToSpriteComp[iObject.GetId()] = compHandle;

    GfxRenderNode::AddObject(iObject);
  }

  void GfxSpriteRenderNode::RemoveObject(ObjectHandle iObject)
  {
    eXl_ASSERT_REPAIR_RET(iObject.GetId() < m_ObjectToSpriteComp.size() && m_SpriteComp.IsValid(m_ObjectToSpriteComp[iObject.GetId()]), void());
    auto compHandle = m_ObjectToSpriteComp[iObject.GetId()];
      
    GfxSpriteComponent* sprite = &m_SpriteComp.Get(compHandle);
    m_SpriteData.Release(sprite->m_SpriteData);
    m_DirtyComponents.erase(sprite);
    m_SpriteComp.Release(compHandle);
  }
}