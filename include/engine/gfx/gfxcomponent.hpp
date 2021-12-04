/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>

#include <core/heapobject.hpp>
#include <core/refcobject.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/oglspritealgo.hpp>
#include <ogl/renderer/oglrendercommand.hpp>
#include <math/matrix4.hpp>

#include <engine/common/transforms.hpp>
#include <engine/gfx/tileset.hpp>
#include <engine/game/commondef.hpp>

namespace eXl
{
  class GfxSystem;
  class OGLDisplayList;


	class EXL_ENGINE_API GfxResource : public HeapObject
	{
		DECLARE_RefC;
	public:
		virtual ~GfxResource() {}
	};

  class EXL_ENGINE_API GeometryInfo : public GfxResource
  {
  public:

    void SetupAssembly(bool hasTexCoord);

    IntrusivePtr<OGLBuffer> m_Vertices;
    IntrusivePtr<OGLBuffer> m_Indices;

    OGLVAssembly m_Assembly;
    OGLDraw::Command m_Command;
  };

  class EXL_ENGINE_API MaterialInfo : public GfxResource
  {
  public:

    virtual uint32_t Push(OGLDisplayList& iList) = 0;
  };

  class EXL_ENGINE_API SpriteMaterialInfo : public MaterialInfo
  {
  public:

    void SetupData();

    uint32_t Push(OGLDisplayList& iList) override;

    IntrusivePtr<OGLTexture const> m_Texture;
    SpriteColor m_SpriteInfo;

    OGLShaderData m_TextureData;
  };

  class EXL_ENGINE_API GfxComponent
  {
  public:
    
    GfxComponent();

    void SetTransform(Matrix4f const& iTransform);

    void SetGeometry(GeometryInfo* iGeom);
    inline void SetGeometry(IntrusivePtr<GeometryInfo> const& iGeom) { SetGeometry(iGeom.get()); }

    void AddDraw(MaterialInfo* iMat, uint32_t iNumElems, uint32_t iOffset, uint8_t iLayer = 0);

    void ClearDraws()
    {
      m_Draws.clear();
    }

    void Push(OGLDisplayList& iList);

  protected:
    friend class GfxSystem;
    
    ObjectHandle m_Object;
    Matrix4f m_Transform;

    OGLShaderData m_PositionData;

    struct Draw
    {
      IntrusivePtr<MaterialInfo> m_Material;

      uint32_t m_NumElements;
      uint32_t m_Offset;
      uint8_t m_Layer;
    };

    Vector<Draw> m_Draws;
    IntrusivePtr<GeometryInfo> m_Geometry;
  };

  class GfxSpriteComponent;
	struct EXL_ENGINE_API GfxSpriteData
	{
		IntrusivePtr<GeometryInfo> m_Geometry;

		IntrusivePtr<OGLTexture const> m_Texture;
		SpriteColor m_SpriteInfo;

    // TODO : Put that into the Tileset
    // TODO : Make a cache somewhere.
		OGLShaderData m_TextureData;
		Matrix4f m_Transform;
    Matrix4f m_BillboardTransform;
    Vector2f m_CurScale;
    Vector2f m_CurOffset;
		OGLShaderData m_PositionData;
		GfxSpriteComponent* m_Component;

		float m_RemainingTime;
		uint32_t m_CurrentFrame;
    uint8_t m_Layer = 0;
		bool m_Forward;
		bool m_Loop;
    bool m_Rotate;
    bool m_Billboard = false;

		static IntrusivePtr<OGLBuffer> MakeSpriteGeometry(Vector2f iSize, bool iFlat);
    static IntrusivePtr<OGLBuffer> MakeSpriteIdxBuffer();

		void Push(OGLDisplayList& iList);
	};

	class GfxSpriteComponent;

	typedef ObjectTable<GfxSpriteData> SpriteDataTable;

	class EXL_ENGINE_API GfxSpriteComponent
	{
		friend GfxSystem;
	public:

    struct EXL_ENGINE_API Desc
    {
      EXL_REFLECT;

      ResourceHandle<Tileset> m_Tileset;
      TileName m_TileName;
      Vector4f m_Tint = Vector4f(1.0, 1.0, 1.0, 1.0);
      Vector2f m_Size = Vector2f(1.0 / EngineCommon::s_WorldToPixel, 1.0 / EngineCommon::s_WorldToPixel);
      Vector2f m_Offset = Vector2f(0.0, 0.0);
      float m_AnimSpeed = 1.0;
      uint8_t m_Layer = 0;
      bool m_RotateSprite = false;
      bool m_Flat = false;
    };
		
    GfxSpriteComponent();

    void SetDesc(Desc const& iDesc);

    void SetOffset(Vector2f const& iOffset);
		void SetSize(Vector2f const& iSize);
		void SetTileset(Tileset const* iTileset);
		void SetTileName(TileName iName);
		void SetAnimationSpeed(float iSpeed);
    void SetRotateSprite(bool iRotate);
    void SetLayer(uint8_t iLayer);
    void SetTint(Vector4f const& iTint);
    void SetFlat(bool iValue);
		
	protected:

    Desc* m_Desc;

		GfxSystem* m_System = nullptr;
		ObjectHandle m_Object;
		SpriteDataTable::Handle m_SpriteData;
	};
  DEFINE_ENGINE_TYPE(GfxSpriteComponent);

  EXL_ENGINE_API GameDataView<GfxSpriteComponent::Desc>* GetSpriteComponentView(World& iWorld);

	typedef ObjectTable<GfxSpriteComponent> SpriteComponents;
}