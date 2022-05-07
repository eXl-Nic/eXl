/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxcomponent.hpp>
#include "gfxspriterendernode.hpp"
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/oglspritealgo.hpp>
#include <core/type/tagtype.hpp>

namespace eXl
{
  IMPLEMENT_RefC(GfxResource)

  void GeometryInfo::SetupAssembly(bool hasTexCoord)
  {
    m_Assembly.m_Attribs.clear();

    const unsigned int stride = (hasTexCoord ? 5 : 3) * sizeof(float);

    m_Assembly.AddAttrib(m_Vertices.get(), OGLBaseAlgo::GetPosAttrib(), 3, stride, 0);
    if(hasTexCoord)
    {
      m_Assembly.AddAttrib(m_Vertices.get(), OGLBaseAlgo::GetTexCoordAttrib(), 2, stride, 3*sizeof(float));
    }
    m_Assembly.m_IBuffer = m_Indices.get();
    m_Assembly.m_IOffset = 0;
  }

  void SpriteMaterialInfo::SetupData()
  {
    m_TextureData.~OGLShaderData();
    new (&m_TextureData) OGLShaderData;
    if (m_Texture)
    {
      m_TextureData.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), m_Texture.get());
      m_TextureData.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(), m_Texture.get());
    }
    m_TextureData.AddData(OGLSpriteAlgo::GetSpriteColorUniform(), &m_SpriteInfo);
  }

  uint32_t SpriteMaterialInfo::Push(OGLDisplayList& iList)
  {
    iList.PushData(&m_TextureData);
    return 1;
  }

  GfxComponent::GfxComponent()
  {
    //m_Program = OGLSpriteAlgo::GetSpriteProgram(false);
    m_Transform = Identity<Mat4>();
    m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_Transform);
  }

  void GfxComponent::SetTransform(Mat4 const& iTransform)
  {
    //m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &iTransform);
    m_Transform = iTransform;
  }

  void GfxComponent::SetGeometry(GeometryInfo* iGeometry)
  {
    m_Geometry = iGeometry;
  }

  void GfxComponent::Push(OGLDisplayList& iList)
  {
    if( m_Program != nullptr
      && m_Geometry != nullptr 
      && !m_Draws.empty())
    {
      iList.SetProgram(m_Program);
      iList.SetVAssembly(&m_Geometry->m_Assembly);
      for(auto& draw : m_Draws)
      {
        uint32_t const toPop = 1 + (draw.m_Material != nullptr 
          ? draw.m_Material->Push(iList)
          : 0);

        iList.PushData(&m_PositionData);
        if (draw.m_NumInstances == 0)
        {
          iList.PushDraw(0x0100 | draw.m_Layer, m_Geometry->m_Command, draw.m_NumElements, draw.m_Offset, 0);
        }
        else
        {
#ifndef __ANDROID__
          iList.PushDrawInstanced(0x0100 | draw.m_Layer, m_Geometry->m_Command, draw.m_NumElements, draw.m_Offset, 0, draw.m_NumInstances, draw.m_BaseInstance);
#else
          iList.PushDrawInstanced(0x0100 | draw.m_Layer, m_Geometry->m_Command, draw.m_NumElements, draw.m_Offset, 0, draw.m_NumInstances);
#endif
        }

        for (uint32_t i = 0; i < toPop; ++i)
        {
          iList.PopData();
        }
      }
    }
  }

	IntrusivePtr<OGLBuffer> GfxSpriteData::MakeSpriteGeometry(Vec2 iSize, bool iFlat)
	{
		float vtxLocData[20];
		memset(vtxLocData, 0, sizeof(vtxLocData));
		vtxLocData[0] = -0.5 * iSize.x;
		vtxLocData[1] = -0.5 * iSize.y;
		vtxLocData[2] = 0.0;

		vtxLocData[5] = 0.5 * iSize.x;
		vtxLocData[6] = -0.5 * iSize.y;
		vtxLocData[7] = 0.0;

		vtxLocData[10] = -0.5 * iSize.x;
		vtxLocData[11] = 0.5 * iSize.y;
		vtxLocData[12] = iFlat ? 0.0 : 1.0;

		vtxLocData[15] = 0.5 * iSize.x;
		vtxLocData[16] = 0.5 * iSize.y;
		vtxLocData[17] = iFlat ? 0.0 : 1.0;

		//vtxLocData[3] = vtxLocData[13] = iTileOffset.x() * texStep.x() + texStep.x() * 0.25 * 0.0;
		//vtxLocData[8] = vtxLocData[18] = (iTileOffset.x() + iTileSize.x()) * texStep.x() - texStep.x() * 0.25 * 0.0;
		//vtxLocData[4] = vtxLocData[9] = (iTileOffset.y() + iTileSize.y()) * texStep.y() - texStep.y() * 0.25 * 0.0;
		//vtxLocData[14] = vtxLocData[19] = (iTileOffset.y()) * texStep.y() + texStep.y() * 0.25 * 0.0;

		vtxLocData[3] = vtxLocData[13] = 0.0;
		vtxLocData[8] = vtxLocData[18] = 1.0;
		vtxLocData[4] = vtxLocData[9] =  1.0;
		vtxLocData[14] = vtxLocData[19] = 0.0;

		return IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, sizeof(vtxLocData), vtxLocData));
		
		//m_SpriteInfo.color = One<Vec4>();
		//m_SpriteInfo.alphaMult = 1.0;
	};

  IntrusivePtr<OGLBuffer> GfxSpriteData::MakeSpriteIdxBuffer()
  {
    unsigned int indexData[] = { 0, 1, 2, 2, 1, 3 };
    return IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData));
  }

	void GfxSpriteData::Push(OGLDisplayList& iList, uint16_t iKey)
	{
    iList.SetVAssembly(&m_Geometry->m_Assembly);
    iList.PushData(&m_TextureData);
		iList.PushData(&m_PositionData);
		iList.PushDraw(iKey + m_Layer, OGLDraw::TriangleList, 6, 0, 0);
		iList.PopData();
		iList.PopData();
	}

  GfxSpriteComponent::Desc* GetDataToMutate(World& iWorld, ObjectHandle iObj)
  {
    GameDataView<GfxSpriteComponent::Desc>* view = GetSpriteComponentView(iWorld);
    if (!view->HasEntry(iObj))
    {
      return nullptr;
    }
    return view->Get(iObj);
  }

  void MakeSpriteDirty(World& iWorld, ObjectHandle iObj)
  {
    GfxSystem* gfx = iWorld.GetSystem<GfxSystem>();
    if (gfx == nullptr)
    {
      return;
    }
    GfxRenderNodeHandle handle = gfx->GetSpriteHandle();
    GfxSpriteRenderNode::DynamicCast(gfx->GetRenderNode(handle))->SetSpriteDirty(iObj);
  }

  void GfxSpriteComponent::SetDesc(World& iWorld, ObjectHandle iObj, Desc const& iDesc)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    *descData = iDesc;
    descData->m_Tileset.GetOrLoad()->Find("AA");
    MakeSpriteDirty(iWorld, iObj);
  }

	void GfxSpriteComponent::SetSize(World& iWorld, ObjectHandle iObj, Vec2 const& iSize)
	{
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Size = iSize;

    MakeSpriteDirty(iWorld, iObj);
	}

  void GfxSpriteComponent::SetOffset(World& iWorld, ObjectHandle iObj, Vec2 const& iOffset)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Offset = iOffset;

    MakeSpriteDirty(iWorld, iObj);
  }

	void GfxSpriteComponent::SetTileset(World& iWorld, ObjectHandle iObj, Tileset const* iTileset)
	{
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Tileset.Set(iTileset);

    MakeSpriteDirty(iWorld, iObj);
	}

	void GfxSpriteComponent::SetTileName(World& iWorld, ObjectHandle iObj, TileName iName)
	{
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_TileName = iName;

    MakeSpriteDirty(iWorld, iObj);
	}

	void GfxSpriteComponent::SetAnimationSpeed(World& iWorld, ObjectHandle iObj, float iSpeed)
	{
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_AnimSpeed = iSpeed;

    MakeSpriteDirty(iWorld, iObj);
	}

  void GfxSpriteComponent::SetRotateSprite(World& iWorld, ObjectHandle iObj, bool iRotate)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_RotateSprite = iRotate;

    MakeSpriteDirty(iWorld, iObj);
  }

  void GfxSpriteComponent::SetLayer(World& iWorld, ObjectHandle iObj, uint8_t iLayer)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Layer = iLayer;

    MakeSpriteDirty(iWorld, iObj);
  }


  void GfxSpriteComponent::SetTint(World& iWorld, ObjectHandle iObj, Vec4 const& iTint)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Tint = iTint;

    MakeSpriteDirty(iWorld, iObj);
  }

  void GfxSpriteComponent::SetFlat(World& iWorld, ObjectHandle iObj, bool iValue)
  {
    GfxSpriteComponent::Desc* descData = GetDataToMutate(iWorld, iObj);
    if (descData == nullptr)
    {
      return;
    }
    descData->m_Flat = iValue;

    MakeSpriteDirty(iWorld, iObj);
  }

  GameDataView<GfxSpriteComponent::Desc>* GetSpriteComponentView(World& iWorld)
  {
    GameDatabase* database = iWorld.GetSystem<GameDatabase>();
    if (database)
    {
      return database->GetView<GfxSpriteComponent::Desc>(EngineCommon::GfxSpriteDescName());
    }
    return nullptr;
  }
}