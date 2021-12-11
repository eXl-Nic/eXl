/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <engine/common/debugtool.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>
#include <core/type/tagtype.hpp>

#define VALIDATE_FLOAT(floatValue) eXl_ASSERT(!std::isnan(floatValue) && std::isfinite(floatValue));

namespace eXl
{
  IMPLEMENT_RTTI(GfxSystem);

  class GfxDebugDrawer : public DebugTool::Drawer
  {
    friend GfxSystem;

  public:

    void DrawLine(const Vector3f& iFrom, const Vector3f& iTo, const Vector4f& iColor, bool iScreenSpace = false)
    {
      auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
      while (m_Lines.size() <= matEntry->second)
      {
        m_Lines.push_back(Vector<Vector3f>());
      }

      auto& lines = m_Lines[matEntry->second];

      if (!iScreenSpace)
      {
        lines.push_back(iFrom);
        lines.push_back(iTo);
      }
      else
      {
        lines.push_back(iFrom * m_CurrentScreenSize);
        lines.push_back(iTo * m_CurrentScreenSize);
      }
    }

    void DrawBox(AABB2Df const& iBox, const Vector4f& iColor, bool iScreenSpace = false)
    {
      auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
      while (m_Boxes.size() <= matEntry->second)
      {
        m_Boxes.push_back(Vector<AABB2Df>());
      }

      auto& boxes = m_Boxes[matEntry->second];
      boxes.push_back(iBox);
    }

    virtual void DrawConvex(Vector<Vector2f> const& iConvex, const Vector4f& iColor, bool iScreenSpace)
    {
      auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
      while (m_Convex.size() <= matEntry->second)
      {
        m_Convex.push_back(Vector<Vector<Vector2f>>());
      }

      auto& boxes = m_Convex[matEntry->second];
      boxes.push_back(iConvex);
    }

  protected:

    Vector<Vector<Vector3f>> m_Lines;
    Vector<Vector<AABB2Df>> m_Boxes;
    Vector<Vector<Vector<Vector2f>>> m_Convex;
    Map<Vector4f, uint32_t> m_Colors;
    float m_CurrentScreenSize;
  };

  void GetPositionOnly(Matrix4f const& iMat, Matrix4f& oMat)
  {
    MathTools::GetPosition(oMat) = MathTools::GetPosition(iMat);
  }

  class GfxSystem::Impl
  {
  public:
    Impl(Transforms& iTrans)
      : m_Transforms(iTrans)
    {
      
    }

    static SpriteDataTable::Handle GetSpriteData(GfxSpriteComponent& iComp)
    {
      return iComp.m_SpriteData;
    }

    static Vector2f const& GetSpriteOffset(GfxSpriteComponent& iComp)
    {
      return iComp.m_Desc->m_Offset;
    }

		void PrepareSprites()
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
          
					if(!tile->m_Frames.empty())
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
        
        Matrix4f const& worldTrans = m_Transforms.GetWorldTransform(comp->m_Object);
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

      /*
      m_SpriteData.Iterate([this](GfxSpriteData& data, SpriteDataTable::Handle)
      {
        if (data.m_Billboard)
        {
          Matrix4f alignMatrix = m_Camera.m_ViewInverseMat;
          MathTools::GetPosition(alignMatrix) = Vector3f::ZERO;

          Matrix4f spriteRotation = data.m_Transform;
          MathTools::GetPosition(spriteRotation) = Vector3f::ZERO;

          data.m_BillboardTransform = alignMatrix * spriteRotation;
          MathTools::GetPosition(data.m_BillboardTransform) = MathTools::GetPosition(data.m_Transform);
        }
      });
      */
		}

    Vector2i m_ViewportSize;
    float m_NearP;
    float m_FarP;
    Vector4f m_ClearColor;
    float m_ClearDepth;
    CameraMatrix m_Camera;
    OGLTextureLoader m_TexLoader;
    GfxDebugDrawer m_DebugDrawer;

    IntrusivePtr<OGLBuffer> m_CameraBuffer;

    Transforms& m_Transforms;

    typedef ObjectTable<GfxComponent> Components;

    Components m_ToRender;
		SpriteDataTable m_SpriteData;
		
		SpriteComponents m_SpriteComp;
    Vector<Components::Handle> m_ObjectToComp;
		Vector<SpriteComponents::Handle> m_ObjectToSpriteComp;

		UnorderedSet<GfxSpriteComponent*> m_DirtyComponents;

		UnorderedMap<Vector3f, IntrusivePtr<GeometryInfo>> m_SpriteGeomCache;
		IntrusivePtr<OGLBuffer> m_DefaultSpriteIdxBuffer;

  };

  void GfxSystem::ScreenToWorld(Vector2i const& iScreenPos, Vector3f& oWorldPos, Vector3f& oViewDir)
  {
    Vector2f screenSpacePos(( 2.0 * float(iScreenPos.X()) / m_Impl->m_ViewportSize.X() - 1.0), 
      ((1.0 - 2.0 * float(iScreenPos.Y()) / m_Impl->m_ViewportSize.Y())));
    Vector4f screenPos(screenSpacePos.X(), screenSpacePos.Y(), m_Impl->m_NearP, 1.0);
    Vector4f screenPosF(screenSpacePos.X(), screenSpacePos.Y(), m_Impl->m_NearP + (m_Impl->m_FarP - m_Impl->m_NearP) * 0.1, 1.0);

    Matrix4f invMat = m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix;
    invMat = invMat.Inverse();

    Vector4f worldPt = invMat * screenPos;
    Vector4f worldPtF = invMat * screenPosF;

    oWorldPos = reinterpret_cast<Vector3f&>(worldPt);
    oViewDir = reinterpret_cast<Vector3f&>(worldPtF) - oWorldPos;
    oViewDir.Normalize();
  }

  Vector2i GfxSystem::WorldToScreen(Vector3f const& iWorldPos)
  {
    Vector4f worldPos(iWorldPos.X(), iWorldPos.Y(), iWorldPos.Z(), 1.0);

    Vector4f projectedPt = m_Impl->m_Camera.projMatrix * m_Impl->m_Camera.viewMatrix * worldPos;

    return Vector2i(((projectedPt.X() * 0.5) + 0.5) * m_Impl->m_ViewportSize.X()
      , (0.5 - ((projectedPt.Y() * 0.5))) * m_Impl->m_ViewportSize.Y());
  }

  namespace
  {
    Image BitmapToImage(uint8_t const* iBitmap, Image::Size iImageSize)
    {
      uint32_t numPixels = iImageSize.X() * iImageSize.Y();
      uint8_t* imageData = (uint8_t*)eXl_ALLOC(numPixels * 4 * sizeof(uint8_t));

      uint8_t* curPixel = imageData;
      for (uint32_t i = 0; i < numPixels; ++i)
      {
        uint8_t intensity = (*iBitmap & 127) << 1;
        uint8_t alphaValue = *iBitmap & 128 ? 255 : 0;

        curPixel[0] = curPixel[1] = curPixel[2] = intensity;
        curPixel[3] = alphaValue;

        curPixel += 4;
        iBitmap++;
      }

      return Image(imageData, iImageSize, Image::RGBA, Image::Char, Image::Adopt);
    }
  }

  static IntrusivePtr<OGLTexture> s_DebugTexture;

  void GfxSystem::StaticInit()
  {
    static bool s_StaticInitDone = false;
    if (!s_StaticInitDone)
    {
      OGLProgramInterface::InitStaticData();

      OGLBaseAlgo::Init();
      OGLSpriteAlgo::Init();
      OGLLineAlgo::Init();

      const uint8_t dummyWhite[] =
      {
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255
      };

      Image whiteImage = BitmapToImage(dummyWhite, Image::Size(8, 8));
      s_DebugTexture = OGLTextureLoader::CreateFromImage(&whiteImage, true);

			s_StaticInitDone = true;
    }
  }

  GfxSystem::GfxSystem(Transforms& iTransforms)
    : m_Impl(new Impl(iTransforms))
  {
  }

  DebugTool::Drawer* GfxSystem::GetDebugDrawer()
  {
    return &m_Impl->m_DebugDrawer;
  }

  void GfxSystem::EnableDebugDraw()
  {
    DebugTool::SetDrawer(&m_Impl->m_DebugDrawer);
  }

  void GfxSystem::DisableDebugDraw()
  {
    DebugTool::SetDrawer(nullptr);
  }

  GfxComponent& GfxSystem::CreateComponent(ObjectHandle iObject)
  {
    eXl_ASSERT(GetWorld().IsObjectValid(iObject));

    Impl::Components::Handle compHandle = m_Impl->m_ToRender.Alloc();
    GfxComponent& newComp = m_Impl->m_ToRender.Get(compHandle);
    newComp.m_Object = iObject;

    if(m_Impl->m_Transforms.HasTransform(iObject))
    {
      newComp.m_Transform = m_Impl->m_Transforms.GetWorldTransform(iObject);
    }
    else
    {
      newComp.m_Transform = Matrix4f::IDENTITY;
    }

    while(m_Impl->m_ObjectToComp.size() <= iObject.GetId())
    {
      m_Impl->m_ObjectToComp.push_back(Impl::Components::Handle());
    }
    m_Impl->m_ObjectToComp[iObject.GetId()] = compHandle;
    ComponentManager::CreateComponent(iObject);
    return newComp;
  }

  GfxComponent* GfxSystem::GetComponent(ObjectHandle iObject)
  {
    if (GetWorld().IsObjectValid(iObject)
      && iObject.GetId() < m_Impl->m_ObjectToComp.size())
    {
      return &m_Impl->m_ToRender.Get(m_Impl->m_ObjectToComp[iObject.GetId()]);
    }
    return nullptr;
  }

	GfxSpriteComponent& GfxSystem::CreateSpriteComponent(ObjectHandle iObject)
	{
		eXl_ASSERT(GetWorld().IsObjectValid(iObject));

		SpriteComponents::Handle compHandle = m_Impl->m_SpriteComp.Alloc();
		GfxSpriteComponent& newComp = m_Impl->m_SpriteComp.Get(compHandle);

    GfxSpriteComponent::Desc& desc = GetSpriteComponentView(GetWorld())->GetOrCreate(iObject);

		newComp.m_System = this;
		newComp.m_Object = iObject;
    newComp.m_Desc = &desc;
		m_Impl->m_DirtyComponents.insert(&newComp);

		while (m_Impl->m_ObjectToSpriteComp.size() <= iObject.GetId())
		{
			m_Impl->m_ObjectToSpriteComp.push_back(SpriteComponents::Handle());
		}
		m_Impl->m_ObjectToSpriteComp[iObject.GetId()] = compHandle;
		ComponentManager::CreateComponent(iObject);
		return newComp;
	}

	GfxSpriteComponent* GfxSystem::GetSpriteComponent(ObjectHandle iObject)
	{
    if (GetWorld().IsObjectValid(iObject)
      && iObject.GetId() < m_Impl->m_ObjectToSpriteComp.size())
    {
      return &m_Impl->m_SpriteComp.Get(m_Impl->m_ObjectToSpriteComp[iObject.GetId()]);
    }

    return nullptr;
	}

  void GfxSystem::DeleteComponent(ObjectHandle iObject)
  {
    if (iObject.GetId() < m_Impl->m_ObjectToComp.size()
      && m_Impl->m_ToRender.IsValid(m_Impl->m_ObjectToComp[iObject.GetId()]))
    {
      m_Impl->m_ToRender.Release(m_Impl->m_ObjectToComp[iObject.GetId()]);
    }

		if (iObject.GetId() < m_Impl->m_ObjectToSpriteComp.size())
		{
      auto compHandle = m_Impl->m_ObjectToSpriteComp[iObject.GetId()];
      if (m_Impl->m_SpriteComp.IsValid(compHandle))
      {
        GfxSpriteComponent* sprite = &m_Impl->m_SpriteComp.Get(compHandle);
        m_Impl->m_SpriteData.Release(sprite->m_SpriteData);
        m_Impl->m_DirtyComponents.erase(sprite);

        m_Impl->m_SpriteComp.Release(compHandle);
      }
		}

		ComponentManager::DeleteComponent(iObject);
  }

	void GfxSystem::SetSpriteDirty(GfxSpriteComponent& iComp)
	{
		m_Impl->m_DirtyComponents.insert(&iComp);
	}

  Vector2i GfxSystem::GetViewportSize() const
  {
    return m_Impl->m_ViewportSize;
  }

  void GfxSystem::SynchronizeTransforms()
  {
    m_Impl->m_Transforms.IterateOverDirtyTransforms([this](Matrix4f const& iMat, ObjectHandle iObj)
    {
      if(iObj.GetId() < m_Impl->m_ObjectToComp.size())
      {
        Impl::Components::Handle objHandle = m_Impl->m_ObjectToComp[iObj.GetId()];
        if(auto* obj = m_Impl->m_ToRender.TryGet(objHandle))
        {
          obj->SetTransform(iMat);
        }
      }

      if (iObj.GetId() < m_Impl->m_ObjectToSpriteComp.size())
      {
        SpriteComponents::Handle objHandle = m_Impl->m_ObjectToSpriteComp[iObj.GetId()];
        if (auto* obj = m_Impl->m_SpriteComp.TryGet(objHandle))
        {
          SpriteDataTable::Handle dataHandle = Impl::GetSpriteData(*obj);
          if (dataHandle.IsAssigned())
          {
            GfxSpriteData& data = m_Impl->m_SpriteData.Get(dataHandle);
            if (data.m_Rotate)
            {
              Matrix4f localTrans;
              localTrans.MakeIdentity();
              localTrans.m_Data[0] = data.m_CurScale.X();
              localTrans.m_Data[5] = data.m_CurScale.Y();
              MathTools::GetPosition2D(localTrans) = Impl::GetSpriteOffset(*obj) + data.m_CurOffset;

              data.m_Transform = iMat * localTrans;
            }
            else
            {
              GetPositionOnly(iMat, data.m_Transform);
              MathTools::GetPosition2D(data.m_Transform) += Impl::GetSpriteOffset(*obj) + data.m_CurOffset;
            }
          }
        }
      }
    });
  }

  CameraMatrix const& GfxSystem::GetCurrentCamera()
  {
    return m_Impl->m_Camera;
  }

  void GfxSystem::SetView(ViewInfo const& iInfo)
  {
    m_Impl->m_ViewportSize = iInfo.viewportSize;
    m_Impl->m_Camera.projMatrix.MakeZero();
    m_Impl->m_Camera.viewInverseMatrix.MakeIdentity();

    float screenRatio = float(m_Impl->m_ViewportSize.X()) / float(m_Impl->m_ViewportSize.Y());
    bool ortho = iInfo.projection == Orthographic;

    if (ortho)
    {
      m_Impl->m_NearP = 0.0001;
      m_Impl->m_FarP = iInfo.displayedSize * 100.0;

      m_Impl->m_Camera.projMatrix.m_Data[0] = 2.0 / (iInfo.displayedSize * screenRatio);
      m_Impl->m_Camera.projMatrix.m_Data[5] = 2.0 / iInfo.displayedSize;
      m_Impl->m_Camera.projMatrix.m_Data[10] = -2.0 / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[14] = -(m_Impl->m_FarP + m_Impl->m_NearP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[15] = 1.0;
    }
    else
    {
      m_Impl->m_NearP = iInfo.displayedSize * 0.5 / tan(iInfo.fov * 0.5);
      m_Impl->m_FarP = iInfo.displayedSize * 1000;

      m_Impl->m_Camera.projMatrix.m_Data[0] = 2.0 * m_Impl->m_NearP / screenRatio;
      m_Impl->m_Camera.projMatrix.m_Data[5] = 2.0 * m_Impl->m_NearP;
      m_Impl->m_Camera.projMatrix.m_Data[10] = -1.0* (m_Impl->m_NearP + m_Impl->m_FarP) / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[14] = -2.0 * m_Impl->m_NearP * m_Impl->m_FarP / (m_Impl->m_FarP - m_Impl->m_NearP);
      m_Impl->m_Camera.projMatrix.m_Data[11] = -1.0;
    }

    Vector3f basisX = iInfo.basis[0];
    Vector3f basisY = iInfo.basis[1];
    Vector3f basisZ = iInfo.basis[2];

    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 0, &basisX, sizeof(Vector3f));
    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 4, &basisY, sizeof(Vector3f));
    memcpy(m_Impl->m_Camera.viewInverseMatrix.m_Data + 8, &basisZ, sizeof(Vector3f));

    //Vector3f transPos = basisX * (-iInfo.pos.X()) + basisY * (-iInfo.pos.Y()) + basisZ * (-iInfo.pos.Z());
    m_Impl->m_Camera.viewInverseMatrix.m_Data[12] = iInfo.pos.X();
    m_Impl->m_Camera.viewInverseMatrix.m_Data[13] = iInfo.pos.Y();
    m_Impl->m_Camera.viewInverseMatrix.m_Data[14] = iInfo.pos.Z();

    m_Impl->m_Camera.viewMatrix = m_Impl->m_Camera.viewInverseMatrix.Inverse();

    // Will work for ortho, but not persp.
    m_Impl->m_DebugDrawer.m_CurrentScreenSize = iInfo.displayedSize;

    m_Impl->m_ClearColor = iInfo.backgroundColor;
    //m_Impl->m_ClearDepth = 1.0;
  }

  void GfxSystem::RenderFrame(float iDelta)
  {
    if (!m_Impl->m_CameraBuffer)
    {
      m_Impl->m_CameraBuffer = OGLBuffer::CreateBuffer(OGLBufferUsage::UNIFORM_BUFFER, CameraMatrix::GetType()->GetSize(), nullptr);
    }

    OGLDisplayList list;

    list.SetDefaultViewport(Vector2i::ZERO, m_Impl->m_ViewportSize);
    list.SetDefaultDepth(true, true);
    list.SetDefaultScissor(Vector2i(0,0),Vector2i(-1,-1));
    list.SetDefaultBlend(true, OGLBlend::SRC_ALPHA, OGLBlend::ONE_MINUS_SRC_ALPHA);

    list.InitForPush();

    list.Clear(0,true,true, m_Impl->m_ClearColor);

    OGLShaderData camData;
    //camData.AddData(OGLBaseAlgo::GetCameraUniform(), &m_Impl->m_Camera);
    m_Impl->m_CameraBuffer->SetData(0, sizeof(m_Impl->m_Camera), &m_Impl->m_Camera);
    camData.SetDataBuffer(OGLBaseAlgo::GetCameraUniform(), m_Impl->m_CameraBuffer);

    list.PushData(&camData);

    List<GfxComponent*> toDelete;

    m_Impl->m_ToRender.Iterate([&](GfxComponent& comp, Impl::Components::Handle )
    {
      comp.Push(list);
    });

    list.SetProgram(OGLSpriteAlgo::GetSpriteProgram(false));

		m_Impl->PrepareSprites();
		m_Impl->m_SpriteData.Iterate([this, &list, iDelta](GfxSpriteData& data, SpriteDataTable::Handle)
		{
			if (!data.m_Texture)
			{
				return ;
			}

			//Matrix4f const& transform = m_Impl->m_Transforms.GetTransform(data.m_Object);
			//data.m_PositionData.~OGLShaderData();
			//data.m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &transform);

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
							tile->m_Frames.size() - 1 == data.m_CurrentFrame:
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

						if(updateAnim)
						{
							Vector2i offsetPix = tile->m_Frames[data.m_CurrentFrame];
							data.m_RemainingTime = tile->m_FrameDuration / comp->m_Desc->m_AnimSpeed;
							data.m_SpriteInfo.tcOffset = Vector2f(offsetPix.X() * texStep.X(), offsetPix.Y() * texStep.Y());
						}
					}
				}
			}

			data.Push(list);
		});

    list.SetDepth(false, false);

    Matrix4f identMatrix;
    identMatrix.MakeIdentity();
    OGLBuffer* debugGeomLines = nullptr;
    OGLBuffer* debugGeomBoxes = nullptr;
    OGLShaderData worldTransform;
    OGLVAssembly debugGeomAssemblyL;
    OGLVAssembly debugGeomAssemblyB;
    
    GfxDebugDrawer& debugDrawer = m_Impl->m_DebugDrawer;


    Vector<SpriteColor> spriteInfo(debugDrawer.m_Colors.size());
    Vector<OGLShaderData> colors(debugDrawer.m_Colors.size());
    Vector<uint32_t> lineNums(debugDrawer.m_Colors.size(), 0);
    Vector<uint32_t> boxNums(debugDrawer.m_Colors.size(), 0);
    Vector<uint32_t> convexNumPts(debugDrawer.m_Colors.size(), 0);
    for(auto const& matEntry : debugDrawer.m_Colors)
    {
      spriteInfo[matEntry.second].alphaMult = 1.0f;
      spriteInfo[matEntry.second].tint = matEntry.first;
      colors[matEntry.second].AddData(OGLLineAlgo::GetColor(), &matEntry.first);
      colors[matEntry.second].AddData(OGLSpriteAlgo::GetSpriteColorUniform(), spriteInfo.data() + matEntry.second);
      colors[matEntry.second].AddData(OGLBaseAlgo::GetWorldMatUniform(), &identMatrix);
      colors[matEntry.second].AddTexture(OGLBaseAlgo::GetDiffuseTexture(), s_DebugTexture.get());

      if(debugDrawer.m_Lines.size() > matEntry.second)
      {
        lineNums[matEntry.second] = debugDrawer.m_Lines[matEntry.second].size();
      }
      if(debugDrawer.m_Boxes.size() > matEntry.second)
      {
        boxNums[matEntry.second] = debugDrawer.m_Boxes[matEntry.second].size();
      }
      if (debugDrawer.m_Convex.size() > matEntry.second)
      {
        for (auto const& cvx : debugDrawer.m_Convex[matEntry.second])
        {
          convexNumPts[matEntry.second] += 3 * cvx.size() ;
        }
      }
    }

    uint32_t totNumLines = 0;
    for(auto& offset : lineNums)
    {
      uint32_t numLines = offset;
      offset = totNumLines;
      totNumLines += numLines;
    }
    uint32_t totNumBoxes = 0;
    for(auto& offset : boxNums)
    {
      size_t numBoxes = offset;
      offset = totNumBoxes;
      totNumBoxes += numBoxes;
    }
    Vector<uint32_t> convexOffset;
    uint32_t totNumConvexPt = 0;
    uint32_t startOffsetConvex = totNumBoxes * 6;
    for (auto numPts : convexNumPts)
    {
      convexOffset.push_back(totNumConvexPt + startOffsetConvex);
      totNumConvexPt += numPts;
    }

    if(!debugDrawer.m_Lines.empty())
    {
      list.SetProgram(OGLLineAlgo::GetProgram());

      debugGeomLines = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, totNumLines * sizeof(Vector3f), nullptr);

      for(uint32_t i = 0; i<debugDrawer.m_Lines.size(); ++i)
      {
        auto const& linesDL = debugDrawer.m_Lines[i];

        size_t dataSize = linesDL.size() * sizeof(Vector3f);
        debugGeomLines->SetData(lineNums[i] * sizeof(Vector3f), dataSize, (void*)linesDL.data());
      }

      debugGeomAssemblyL.AddAttrib(debugGeomLines, OGLBaseAlgo::GetPosAttrib(), 3, sizeof(Vector3f), 0);
      debugGeomAssemblyL.m_IBuffer = nullptr;

      list.SetVAssembly(&debugGeomAssemblyL);
      list.SetDepth(false, false);

      for(auto const& matEntry : debugDrawer.m_Colors)
      {
        if(debugDrawer.m_Lines.size() > matEntry.second)
        {
          list.PushData(colors.data() + matEntry.second);

          auto const& linesDL = debugDrawer.m_Lines[matEntry.second];

          list.PushDraw(0x0100, OGLDraw::LineList, linesDL.size(), lineNums[matEntry.second], 0);

          list.PopData();
        }
      }
    }

    if (!debugDrawer.m_Boxes.empty()
      || !debugDrawer.m_Convex.empty())
    {
      list.SetProgram(OGLSpriteAlgo::GetSpriteProgram());

      Vector<Vector3f> triangles;
      triangles.resize(totNumBoxes * 6 + totNumConvexPt);
      
      for(uint32_t i = 0; i<debugDrawer.m_Boxes.size(); ++i)
      {
        auto const& boxDL = debugDrawer.m_Boxes[i];

        Vector3f* ptData = triangles.data() + 6 * boxNums[i];

        for (uint32_t j = 0; j<boxDL.size(); ++j)
        {
          auto const& box = boxDL[j];

          *(ptData++) = (Vector3f(box.m_Data[0].X(), box.m_Data[0].Y(), 0.0));
          *(ptData++) = (Vector3f(box.m_Data[1].X(), box.m_Data[0].Y(), 0.0));
          *(ptData++) = (Vector3f(box.m_Data[0].X(), box.m_Data[1].Y(), 0.0));
          *(ptData++) = (Vector3f(box.m_Data[1].X(), box.m_Data[0].Y(), 0.0));
          *(ptData++) = (Vector3f(box.m_Data[1].X(), box.m_Data[1].Y(), 0.0));
          *(ptData++) = (Vector3f(box.m_Data[0].X(), box.m_Data[1].Y(), 0.0));
        }
      }

      for (uint32_t i = 0; i < debugDrawer.m_Convex.size(); ++i)
      {
        auto const& cvxDL = debugDrawer.m_Convex[i];
        Vector3f* ptData = triangles.data() + convexOffset[i];
        for (auto const& cvx : cvxDL)
        {
          if (cvx.empty())
          {
            continue;
          }
          Vector2f midPt;
          for (auto const& pt : cvx)
          {
            midPt += pt;
          }
          midPt /= cvx.size();
          for (uint32_t j = 0; j < cvx.size(); ++j)
          {
            uint32_t nextPt = (j + 1) % cvx.size();
            *(ptData++) = Vector3f(midPt.X(), midPt.Y(), 0.0);
            *(ptData++) = Vector3f(cvx[j].X(), cvx[j].Y(), 0.0);
            *(ptData++) = Vector3f(cvx[nextPt].X(), cvx[nextPt].Y(), 0.0);
          }
        }
      }

      debugGeomBoxes = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, (totNumBoxes * 6 + totNumConvexPt) * sizeof(Vector3f), (void*)triangles.data());
      //debugGeomBoxes->SetData(0, triangles.size() * sizeof(Vector3f), (void*)triangles.data());

      debugGeomAssemblyB.AddAttrib(debugGeomBoxes, OGLBaseAlgo::GetPosAttrib(), 3, sizeof(Vector3f), 0);
      debugGeomAssemblyB.m_IBuffer = nullptr;

      list.SetVAssembly(&debugGeomAssemblyB);
      list.SetDepth(false, false);

      for (auto const& matEntry : debugDrawer.m_Colors)
      {
        if(debugDrawer.m_Boxes.size() > matEntry.second
          || debugDrawer.m_Convex.size() > matEntry.second)
        {
          list.PushData(colors.data() + matEntry.second);
          if (debugDrawer.m_Boxes.size() > matEntry.second)
          {
            auto const& boxDL = debugDrawer.m_Boxes[matEntry.second];
            list.PushDraw(0x0100, OGLDraw::TriangleList, boxDL.size() * 6, boxNums[matEntry.second] * 6, 0);
          }
          if (debugDrawer.m_Convex.size() > matEntry.second)
          {
            list.PushDraw(0x0100, OGLDraw::TriangleList, convexNumPts[matEntry.second], convexOffset[matEntry.second], 0);
          }
          list.PopData();
        }
      }
    }

    OGLRenderContext renderContext;
    list.Render(&renderContext);

    //for(auto comp : toDelete)
    //{
    //  //if(comp->m_TransformId)
    //  {
    //    m_ObjectToComp[comp->m_Object.GetId()] = nullptr;
    //  }
    //  eXl_DELETE comp;
    //}

    debugDrawer.m_Lines.clear();
    debugDrawer.m_Colors.clear();
    debugDrawer.m_Boxes.clear();
    debugDrawer.m_Convex.clear();

    if(debugGeomLines)
    {
      eXl_DELETE debugGeomLines;
      debugGeomLines = nullptr;
    }

    if(debugGeomBoxes)
    {
      eXl_DELETE debugGeomBoxes;
      debugGeomBoxes = nullptr;
    }

  }
}