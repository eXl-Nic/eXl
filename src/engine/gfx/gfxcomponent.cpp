/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <core/type/tagtype.hpp>

namespace eXl
{
  IMPLEMENT_RefC(GfxResource)
  IMPLEMENT_TAG_TYPE(GfxSpriteComponent);

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

    m_TextureData.AddTexture(OGLBaseAlgo::GetDiffuseTexture(), m_Texture.get());
    m_TextureData.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(), m_Texture.get());
    m_TextureData.AddData(OGLSpriteAlgo::GetSpriteColorUniform(), &m_SpriteInfo);
  }

  uint32_t SpriteMaterialInfo::Push(OGLDisplayList& iList)
  {
    iList.PushData(&m_TextureData);
    return 1;
  }

  GfxComponent::GfxComponent()
  {
    m_Transform.MakeIdentity();
    m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_Transform);
  }

  void GfxComponent::SetTransform(Matrix4f const& iTransform)
  {
    //m_PositionData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &iTransform);
    m_Transform = iTransform;
  }

  void GfxComponent::SetGeometry(GeometryInfo* iGeometry)
  {
    m_Geometry = iGeometry;
  }

  void GfxComponent::AddDraw(MaterialInfo* iMaterial, uint32_t iNumElems, uint32_t iOffset, uint8_t iLayer)
  {
    if(iMaterial)
    {
      Draw newDraw;
      newDraw.m_Material = iMaterial;
      newDraw.m_NumElements = iNumElems;
      newDraw.m_Offset = iOffset;
      newDraw.m_Layer = iLayer;

      m_Draws.push_back(newDraw);
    }
  }

  void GfxComponent::Push(OGLDisplayList& iList)
  {
    if(m_Geometry != nullptr && !m_Draws.empty())
    {
      if (m_PositionData.GetDataPtr(OGLBaseAlgo::GetWorldMatUniform()) == nullptr)
      {
        
      }
      iList.SetVAssembly(&m_Geometry->m_Assembly);
      for(auto& draw : m_Draws)
      {
        uint32_t const toPop = 1 + (draw.m_Material != nullptr 
          ? draw.m_Material->Push(iList)
          : 0);

        iList.PushData(&m_PositionData);
        iList.PushDraw(0x0100 | draw.m_Layer, m_Geometry->m_Command, draw.m_NumElements, draw.m_Offset);

        for (uint32_t i = 0; i < toPop; ++i)
        {
          iList.PopData();
        }
      }
    }
  }

	IntrusivePtr<OGLBuffer> GfxSpriteData::MakeSpriteGeometry(Vector2f iSize, bool iFlat)
	{
		float vtxLocData[20];
		memset(vtxLocData, 0, sizeof(vtxLocData));
		vtxLocData[0] = -0.5 * iSize.X();
		vtxLocData[1] = -0.5 * iSize.Y();
		vtxLocData[2] = 0.0;

		vtxLocData[5] = 0.5 * iSize.X();
		vtxLocData[6] = -0.5 * iSize.Y();
		vtxLocData[7] = 0.0;

		vtxLocData[10] = -0.5 * iSize.X();
		vtxLocData[11] = 0.5 * iSize.Y();
		vtxLocData[12] = iFlat ? 0.0 : 1.0;

		vtxLocData[15] = 0.5 * iSize.X();
		vtxLocData[16] = 0.5 * iSize.Y();
		vtxLocData[17] = iFlat ? 0.0 : 1.0;

		//vtxLocData[3] = vtxLocData[13] = iTileOffset.X() * texStep.X() + texStep.X() * 0.25 * 0.0;
		//vtxLocData[8] = vtxLocData[18] = (iTileOffset.X() + iTileSize.X()) * texStep.X() - texStep.X() * 0.25 * 0.0;
		//vtxLocData[4] = vtxLocData[9] = (iTileOffset.Y() + iTileSize.Y()) * texStep.Y() - texStep.Y() * 0.25 * 0.0;
		//vtxLocData[14] = vtxLocData[19] = (iTileOffset.Y()) * texStep.Y() + texStep.Y() * 0.25 * 0.0;

		vtxLocData[3] = vtxLocData[13] = 0.0;
		vtxLocData[8] = vtxLocData[18] = 1.0;
		vtxLocData[4] = vtxLocData[9] =  1.0;
		vtxLocData[14] = vtxLocData[19] = 0.0;

		return IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, sizeof(vtxLocData), vtxLocData));
		
		//m_SpriteInfo.color = Vector4f::ONE;
		//m_SpriteInfo.alphaMult = 1.0;
	};

  IntrusivePtr<OGLBuffer> GfxSpriteData::MakeSpriteIdxBuffer()
  {
    unsigned int indexData[] = { 0, 1, 2, 2, 1, 3 };
    return IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData));
  }

	void GfxSpriteData::Push(OGLDisplayList& iList)
	{
		iList.PushData(&m_PositionData);
		iList.SetVAssembly(&m_Geometry->m_Assembly);
		iList.PushData(&m_TextureData);
		iList.PushDraw(0x0100 + m_Layer, OGLDraw::TriangleList, 6, 0);
		iList.PopData();
		iList.PopData();
	}

  void GfxSpriteComponent::SetDesc(Desc const& iDesc)
  {
    *m_Desc = iDesc;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
  }

  GfxSpriteComponent::GfxSpriteComponent()
  {
  }

	void GfxSpriteComponent::SetSize(Vector2f const& iSize)
	{
		m_Desc->m_Size = iSize;
		if (m_SpriteData.IsAssigned())
		{
			m_System->SetSpriteDirty(*this);
		}
	}

  void GfxSpriteComponent::SetOffset(Vector2f const& iOffset)
  {
    m_Desc->m_Offset = iOffset;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
  }

	void GfxSpriteComponent::SetTileset(Tileset const* iTileset)
	{
    m_Desc->m_Tileset.Set(iTileset);
		if (m_SpriteData.IsAssigned())
		{
			m_System->SetSpriteDirty(*this);
		}
	}

	void GfxSpriteComponent::SetTileName(TileName iName)
	{
    m_Desc->m_TileName = iName;
		if (m_SpriteData.IsAssigned())
		{
			m_System->SetSpriteDirty(*this);
		}
	}

	void GfxSpriteComponent::SetAnimationSpeed(float iSpeed)
	{
    m_Desc->m_AnimSpeed = iSpeed;
		if (m_SpriteData.IsAssigned())
		{
			m_System->SetSpriteDirty(*this);
		}
	}

  void GfxSpriteComponent::SetRotateSprite(bool iRotate)
  {
    m_Desc->m_RotateSprite = iRotate;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
  }

  void GfxSpriteComponent::SetLayer(uint8_t iLayer)
  {
    m_Desc->m_Layer = iLayer;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
  }


  void GfxSpriteComponent::SetTint(Vector4f const& iTint)
  {
    m_Desc->m_Tint = iTint;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
  }

  void GfxSpriteComponent::SetFlat(bool iValue)
  {
    m_Desc->m_Flat = iValue;
    if (m_SpriteData.IsAssigned())
    {
      m_System->SetSpriteDirty(*this);
    }
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