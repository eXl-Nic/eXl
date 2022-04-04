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

  class GfxComponentRenderNode;

  class EXL_ENGINE_API GfxComponent
  {
    friend GfxComponentRenderNode;
  protected:
    struct Draw
    {
      IntrusivePtr<MaterialInfo> m_Material;

      uint32_t m_NumElements = 0;
      uint32_t m_Offset = 0;
      uint32_t m_NumInstances = 0;
#ifndef __ANDROID__
      uint32_t m_BaseInstance = 0;
#endif
      uint8_t m_Layer = 0;
    };

  public:
    
    GfxComponent();

    void SetProgram(OGLCompiledProgram const* iProgram)
    {
      m_Program = iProgram;
    }
    void SetTransform(Mat4 const& iTransform);

    void SetGeometry(GeometryInfo* iGeom);
    inline void SetGeometry(IntrusivePtr<GeometryInfo> const& iGeom) { SetGeometry(iGeom.get()); }

    struct [[nodiscard]] DrawBuilder
    {
      [[nodiscard]] DrawBuilder& NumElements(uint32_t iElems) { m_Draw.m_NumElements = iElems; return *this; }
      [[nodiscard]] DrawBuilder& Offset(uint32_t iOffset) { m_Draw.m_Offset = iOffset; return *this; }
      [[nodiscard]] DrawBuilder& NumInstances(uint32_t iNum) { m_Draw.m_NumInstances = iNum; return *this; }
#ifndef __ANDROID__
      [[nodiscard]] DrawBuilder& BaseInstance(uint32_t iBase) { m_Draw.m_BaseInstance = iBase; return *this; }
#endif
      [[nodiscard]] DrawBuilder& Layer(uint32_t iLayer) { m_Draw.m_Layer = iLayer; return *this; }
      void End() { m_Component.AddDraw(std::move(m_Draw)); }
    protected:
      friend GfxComponent;
      DrawBuilder(GfxComponent& iComp, MaterialInfo* iMat)
        : m_Component(iComp)
      {
        m_Draw.m_Material = iMat;
      }
      GfxComponent& m_Component;

      Draw m_Draw;
    };

    //void AddDraw(MaterialInfo* iMat, uint32_t iNumElems, uint32_t iOffset, uint8_t iLayer = 0);
    //void AddDrawIstanced(MaterialInfo* iMat, uint32_t iNumInstances, uint32_t iNumElems, uint32_t iOffset, uint8_t iLayer = 0);
    DrawBuilder AddDraw(MaterialInfo* iMat) { return DrawBuilder(*this, iMat); }
    DrawBuilder AddDraw(IntrusivePtr<MaterialInfo> const& iMat) { return AddDraw(iMat.get()); }

    void ClearDraws()
    {
      m_Draws.clear();
    }

    void Push(OGLDisplayList& iList);

  protected:

    void AddDraw(Draw&& iDraw)
    {
      m_Draws.emplace_back(std::move(iDraw));
    }

    friend class GfxSystem;
    
    ObjectHandle m_Object;
    Mat4 m_Transform;

    Vector<Draw> m_Draws;
    IntrusivePtr<GeometryInfo> m_Geometry;

    OGLShaderData m_PositionData;
    OGLCompiledProgram const* m_Program = nullptr;
  };

  class GfxSpriteComponent;
	struct EXL_ENGINE_API GfxSpriteData
	{
		IntrusivePtr<GeometryInfo> m_Geometry;

		SpriteColor m_SpriteInfo;

    // TODO : Put that into the Tileset
    // TODO : Make a cache somewhere.
		
		Mat4 m_Transform;
    Mat4 m_BillboardTransform;
    Vec2 m_CurScale = One<Vec2>();
    Vec2 m_CurOffset = Zero<Vec2>();


    IntrusivePtr<OGLTexture const> m_Texture;
    OGLShaderData m_TextureData;
		OGLShaderData m_PositionData;

		GfxSpriteComponent* m_Component;

		float m_RemainingTime;
		uint32_t m_CurrentFrame;
    uint8_t m_Layer = 0;
		bool m_Forward;
		bool m_Loop;
    bool m_Rotate;
    bool m_Billboard = false;

		static IntrusivePtr<OGLBuffer> MakeSpriteGeometry(Vec2 iSize, bool iFlat);
    static IntrusivePtr<OGLBuffer> MakeSpriteIdxBuffer();

		void Push(OGLDisplayList& iList);
	};

	class GfxSpriteComponent;

	typedef ObjectTable<GfxSpriteData> SpriteDataTable;

  class GfxSpriteRenderNode;
	class EXL_ENGINE_API GfxSpriteComponent
	{
		friend GfxSpriteRenderNode;
	public:

    struct EXL_ENGINE_API Desc
    {
      EXL_REFLECT;

      ResourceHandle<Tileset> m_Tileset;
      TileName m_TileName;
      Vec4 m_Tint = Vec4(1.0, 1.0, 1.0, 1.0);
      Vec2 m_Size = Vec2(1.0 / EngineCommon::s_WorldToPixel, 1.0 / EngineCommon::s_WorldToPixel);
      Vec2 m_Offset = Vec2(0.0, 0.0);
      float m_AnimSpeed = 1.0;
      uint8_t m_Layer = 0;
      bool m_RotateSprite = false;
      bool m_Flat = false;
    };
		
    GfxSpriteComponent();

    void SetDesc(Desc const& iDesc);

    void SetOffset(Vec2 const& iOffset);
		void SetSize(Vec2 const& iSize);
		void SetTileset(Tileset const* iTileset);
		void SetTileName(TileName iName);
		void SetAnimationSpeed(float iSpeed);
    void SetRotateSprite(bool iRotate);
    void SetLayer(uint8_t iLayer);
    void SetTint(Vec4 const& iTint);
    void SetFlat(bool iValue);
		
	protected:

    bool Mutate();

    Desc* m_Desc = nullptr;

    GfxSpriteRenderNode* m_RenderNode = nullptr;
		ObjectHandle m_Object;
		SpriteDataTable::Handle m_SpriteData;
	};
  DEFINE_ENGINE_TYPE(GfxSpriteComponent);

  EXL_ENGINE_API GameDataView<GfxSpriteComponent::Desc>* GetSpriteComponentView(World& iWorld);

	//typedef ObjectTable<GfxSpriteComponent> SpriteComponents;
}