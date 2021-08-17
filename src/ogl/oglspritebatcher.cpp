/**

  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <ogl/oglspritebatcher.hpp>

//#include "oglfontmanager.hpp"
#include <ogl/renderer/ogltextureloader.hpp>

#include <ogl/oglutils.hpp>

#include <ogl/renderer/oglrendercontext.hpp>
#include <ogl/renderer/oglbuffer.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/oglprogram.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>

#include <ogl/oglspritealgo.hpp>

#ifdef EXL_IMAGESTREAMER_ENABLED
#include <core/image/imagestreamer.hpp>
#endif

#define MINIMAL_HARDWARE_BUFFER_SIZE 120

namespace eXl 
{
  bool Bitmap::TestBox(AABB2Df const& iPickBox, AABB2Df const& iBoxPos, AABB2Df const& iBoxTex) const
  {
    if(m_Pixels.size() == 0)
      return true;

    AABB2Df commonBox;
    commonBox.SetCommonBox(iPickBox,iBoxPos);
    Vector2f range = iBoxPos.m_Data[1] - iBoxPos.m_Data[0];
    commonBox.m_Data[0].X() = (commonBox.m_Data[0].X() - iBoxPos.m_Data[0].X()) / range.X();
    commonBox.m_Data[1].X() = (commonBox.m_Data[1].X() - iBoxPos.m_Data[0].X()) / range.X();
    commonBox.m_Data[0].Y() = (commonBox.m_Data[0].Y() - iBoxPos.m_Data[0].Y()) / range.Y();
    commonBox.m_Data[1].Y() = (commonBox.m_Data[1].Y() - iBoxPos.m_Data[0].Y()) / range.Y();

    range = iBoxTex.m_Data[1] - iBoxTex.m_Data[0];
    commonBox.m_Data[0].X() = (commonBox.m_Data[0].X() * range.X()) + iBoxTex.m_Data[0].X();
    commonBox.m_Data[1].X() = (commonBox.m_Data[1].X() * range.X()) + iBoxTex.m_Data[0].X();
    commonBox.m_Data[0].Y() = (commonBox.m_Data[0].Y() * range.Y()) + iBoxTex.m_Data[0].Y();
    commonBox.m_Data[1].Y() = (commonBox.m_Data[1].Y() * range.Y()) + iBoxTex.m_Data[0].Y();

    AABB2Di pixBox;
    pixBox.m_Data[0].X() = commonBox.m_Data[0].X() * m_Width;
    pixBox.m_Data[1].X() = Mathf::Ceil(commonBox.m_Data[1].X() * m_Width);
    pixBox.m_Data[0].Y() = commonBox.m_Data[0].Y() * m_Height;
    pixBox.m_Data[1].Y() = Mathf::Ceil(commonBox.m_Data[1].Y() * m_Height);

    if(pixBox.MinX() >= pixBox.MaxX() || pixBox.MinY() >= pixBox.MaxY())
    {
      if(pixBox.MinX() >= m_Width)
      {
        if(pixBox.MinY() >= m_Height)
          return m_Pixels.back();
        else
          return m_Pixels[m_Width - 1 + pixBox.MinY() * m_Width];
      }
      else
      {
        if(pixBox.MinY() >= m_Height)
          return m_Pixels[pixBox.MinX() + (m_Height - 1) * m_Width];
        else
          return m_Pixels[pixBox.MinX() + pixBox.MinY() * m_Width];
      }
    }

    for(int i = pixBox.MinY() ; i< pixBox.MaxY(); ++i)
    {
      unsigned int baseOffset = i*m_Width + pixBox.MinX();
      unsigned int size = pixBox.MaxX() - pixBox.MinX();
      for(int j =  0; j< size; ++j)
      {
        if(m_Pixels[baseOffset])
        {
          return true;
        }
        baseOffset++;
      }
    }
    return false;
  }

  IMPLEMENT_RefC(SpriteDesc);

  SpriteDesc::SpriteDesc(AString const& iFileName)
    : m_FileName(iFileName)
  {

  }
 
  SpriteDesc::SpriteDesc(Image* iImage, Image::Storage iStorage)
  {
	  if (iImage)
	  {
		  if (iStorage == Image::Copy || iStorage == Image::Reference)
		  {
			  m_Image.emplace(Image(iImage->GetImageData(), iImage->GetSize(), iImage->GetComponents(), iImage->GetFormat(), iImage->GetRowStride(), iStorage));
		  }
		  else if (iStorage == Image::Adopt)
		  {
			  m_Image.emplace(Image(std::move(*iImage)));
		  }
	  }
  }

  SpriteDesc::~SpriteDesc()
  {
    
  }

  void SpriteDesc::AddAnim(AnimationDesc::Kind iAnimFlag,float iTime,Vector2f const& iOrig,Vector2f const& iStep,Vector2i const& iSize)
  {
    AnimationDesc desc = {iOrig, iStep, iSize, iAnimFlag ,iTime};
    m_Anims.push_back(desc);
  }

  void MakeBitmap(Image const* iImage, Bitmap& oBitmap)
  {
    Bitmap* bitmap = eXl_NEW Bitmap;
    bitmap->m_Width = iImage->GetSize().X();
    bitmap->m_Height = iImage->GetSize().Y();
    if(iImage->GetComponents() == Image::RGBA
      || iImage->GetComponents() == Image::BGRA
      )
    {
      bitmap->m_Pixels.resize(bitmap->m_Width * bitmap->m_Height);
      unsigned int k = 0;
      //unsigned char const* pixels = reinterpret_cast<unsigned char const*>(iImage->GetImageData());
      unsigned int alphaValue;
      if(iImage->GetFormat() != Image::Float)
      {
        //J'oublie le cas float allégrement....
        unsigned int size = iImage->GetFormat() == Image::Char ? 1 : (iImage->GetFormat() == Image::Short ? 2 : 4);
        unsigned int threshold = iImage->GetFormat() == Image::Char ? (1 << 6) - 1 : (iImage->GetFormat() == Image::Short ? (1 << 12) - 1 : (1<<24) - 1);
        for(unsigned int i = 0; i< bitmap->m_Height;++i)
        {
          unsigned char const* pixel = reinterpret_cast<unsigned char const*>(iImage->GetRow(i));
          for(unsigned int j = 0; j< bitmap->m_Width;++j)
          {
            alphaValue = 0;
            memcpy(&alphaValue,pixel + 3*size,size);
            if(alphaValue >= threshold)
            {
              bitmap->m_Pixels[k] = true;
              //printf("_");
            }
            else
            {
              bitmap->m_Pixels[k] = false;
              //printf("0");
            }
            pixel += 4*size;
            ++k;
          }
          //printf("\n");
        }
      }
    }
  }

  void SpriteDesc::Preload(unsigned int iPreloadFlags) const
  {

  }

  OGLTexture const* SpriteDesc::GetTexture() const
  {
    if(m_Texture != NULL)
    {
      return m_Texture.get();
    }
    else
    {
      if(Image const* img = GetImage())
      {
        m_Texture = OGLTextureLoader::CreateFromImage(img, true);
      }
      return m_Texture.get();
    }
    return nullptr;
  }

  void SpriteDesc::UnloadTexture() const
  {

  }

  Bitmap const* SpriteDesc::GetBitmap() const
  {
    if(m_Bitmap)
    {
      return &(*m_Bitmap);
    }
    else
    {
      if(Image const* img = GetImage())
      {
        m_Bitmap.emplace(Bitmap());
        MakeBitmap(img, *m_Bitmap);
      }
    }
    return nullptr;
  }

  Image const* SpriteDesc::GetImage() const
  {
    if(m_Image)
    {
      return &(*m_Image);
    }
    else
    {
#ifdef EXL_IMAGESTREAMER_ENABLED
      Image* img = ImageStreamer::Load(m_FileName);
      if(img)
      {
        m_Image.emplace(std::move(*img));
        eXl_DELETE(img);
      }
#endif
      if(m_Image)
      {
        m_Size = m_Image->GetSize();
      }
      return &(*m_Image);
    }
    return nullptr;
  }

  //IMPLEMENT_RTTI(OGLSpriteBatcher);

  void OGLSpriteBatcher::VertexChunk::Empty()
  {
    eXl_DELETE data;
    spriteDesc = NULL;
  }

  void OGLSpriteBatcher::EmptyChunks()
  {
    for(unsigned int i = 0; i<m_Chunks.size(); ++i)
    {
      m_Chunks[i].Empty();
    }
    m_Chunks.clear();
  }

  OGLSpriteBatcher::OGLSpriteBatcher(float iAltCoeff,Vector3f const& iDepth/*, OGLFontManager& iFontMgr*/)
    : m_Heap(0)
    , dirty(false)
    , needUpdate(false)
    , cacheSprites(true)
    , registerSprites(true)
    , alphaCoeff(1.0)
    , m_Depth(iDepth)
    , m_AltOffset(iAltCoeff)
    //, m_FontMgr(iFontMgr)
    , m_DefaultBuffer(nullptr)
    //, m_Loader(iLoader)
  {

  }

  OGLSpriteBatcher::~OGLSpriteBatcher()
  {
    EmptyChunks();
    if(m_DefaultBuffer != NULL)
    {
      eXl_DELETE m_DefaultBuffer;
    }
  }

  //void OGLSpriteBatcher::Visit(OGLNodeVisitor* iVisitor)
  //{
  //  iVisitor->VisitSpriteBatcher(this);
  //}

  void OGLSpriteBatcher::BatchRender(OGLDisplayList& iList, unsigned char iPrefix)
  {
    for(int i = -1; i<int(m_SpritesCol.size()); ++i)
    {
      std::vector<VertexChunk>::const_iterator currChunk, endChunk;
      bool decal = false;
      if(i == -1)
      {
        currChunk=m_Chunks.begin();
        endChunk=m_Chunks.end();
        iList.SetTechnique(OGLSpriteAlgo::GetSpriteTechnique());
        iList.SetVAssembly(&m_DefaultAssembly);
      }
      else
      {
        
        SpriteArray const& collec = m_SpritesCol[i];

        if(collec.pickId == 0)
          continue;

        bool decal = collec.decal;

        if(decal)
          iList.SetTechnique(OGLSpriteAlgo::GetFontTechnique());
        else
          iList.SetTechnique(OGLSpriteAlgo::GetSpriteTechnique());
        iList.SetVAssembly(&collec.m_Assembly);

        currChunk = collec.m_Chunks.begin();
        endChunk = collec.m_Chunks.end();
      }

      unsigned int vertexStart = 0;
      unsigned int vertexCount = 0;
      for (; currChunk!=endChunk; currChunk++)
      {
        iList.PushData(currChunk->data);

        //Assure que les fonts seront tjrs tracées au dessus.
        //iList.PushDraw(iPrefix << 24 | (currChunk->m_Layer << 1) | (decal ? 1 : 0), OGLDraw::TriangleList, currChunk->vertexCount, vertexStart);
        vertexStart += currChunk->vertexCount;
        iList.PopData();
      }
    }
  }

  void OGLSpriteBatcher::StealSprites(Vector<SpriteElement>& oElems, Vector<VertexChunk>& oChunks, OGLVAssembly& oAssembly, OGLBuffer*& oBuffer)
  {
    oElems = std::move(m_Sprites);
    oChunks = std::move(m_Chunks);
    oAssembly = m_DefaultAssembly;
    m_DefaultAssembly.m_Attribs.clear();
    m_DefaultAssembly.m_IBuffer = nullptr;
    m_DefaultAssembly.m_IOffset = 0;

    oBuffer = m_DefaultBuffer;
    m_DefaultBuffer = 0;
  }

  void OGLSpriteBatcher::StealCollections(Vector<SpriteArray>& oCollecs)
  {
    oCollecs = std::move(m_SpritesCol);
  }

  inline bool OGLSpriteBatcher::CompareSprite::operator ()(SpriteElement const * const& iElem1, SpriteElement const* const& iElem2)
  {
    if(iElem1->layer == iElem2->layer)
    {
      if(iElem1->texHandle == iElem2->texHandle)
      {
        return iElem1->alpha < iElem2->alpha;
      }
      else return iElem1->texHandle < iElem2->texHandle;
    }
    else return iElem1->layer < iElem2->layer;
  }
  
 
  
  AABB2Df const OGLSpriteBatcher::FULL_SPRITE(-1000.0f, -1000.0f, -1000.0f, -1000.0f);

  void OGLSpriteBatcher::SetAlphaCoeff(float iCoeff)
  {
    alphaCoeff = iCoeff;
  }

  
  void OGLSpriteBatcher::FillSpriteElem(SpriteElement& spriteElement, AABB2Df const& destRect, AABB2Df const& spriteRect, Vector2i const& iSpriteSize)
  {
    spriteElement.boxPos = destRect;
    spriteElement.tx1 = spriteElement.ty1 = 0.0f;
    spriteElement.tx2 = spriteElement.ty2 = 1.0f;
 
    if(spriteRect != FULL_SPRITE && spriteRect.GetSize() != Vector2f::ZERO) 
    {
        spriteElement.tx1 = (float)(spriteRect.m_Data[0].X() / iSpriteSize.X());
        spriteElement.ty1 = (float)(spriteRect.m_Data[0].Y() / iSpriteSize.Y());
        spriteElement.tx2 = (float)(spriteRect.m_Data[1].X() / iSpriteSize.X());
        spriteElement.ty2 = (float)(spriteRect.m_Data[1].Y() / iSpriteSize.Y());
    }
  }

  
  unsigned int OGLSpriteBatcher::AddSprite(void* iData,SpriteDesc const* iSprite, float alpha, unsigned int layer,unsigned int iOrient, AABB2Df const& destRect, AABB2Df const& spriteRect)
  {
    if(iSprite == NULL)
      return 0;
    // Retrieve pointer to texture resource
    OGLTexture const* texturePtr = reinterpret_cast<OGLTexture const*>(iSprite->GetTexture());
    if(texturePtr == NULL)
      return 0;
 
    // Get texture handle from texture resource
    SpriteElement spriteElement;
    spriteElement.texHandle = iSprite;
 
    // This is the size of the original image data (pixels)
    Vector2i iSpriteSize(texturePtr->GetSize().X(), texturePtr->GetSize().Y());

    FillSpriteElem(spriteElement,destRect,spriteRect,iSpriteSize);
 
    // save alpha value
    spriteElement.alpha = alpha;
    spriteElement.layer = layer;
    spriteElement.pickId = iData;
    spriteElement.orient = iOrient;
 
    // Add this sprite to our render list
    if(registerSprites)
    {
      unsigned int pos = m_Heap.Get();
      if(pos>=m_Sprites.size())
      {
        m_Sprites.push_back(spriteElement);
      }
      else
      {
        m_Sprites[pos] = spriteElement;
      }
      dirty = true;
      return pos + 1;
    }
    else
    {
      m_Sprites.push_back(spriteElement);
      return 0;
    }
  }

  
  void OGLSpriteBatcher::UpdateSprite(unsigned int iId, AABB2Df const& destRect)
  {
    if(iId > 0 && (iId - 1) <= m_Sprites.size())
    {
      SpriteElement& spriteElement = m_Sprites[iId - 1];

      if(spriteElement.texHandle == NULL)
        return;

      spriteElement.boxPos = destRect;

      needUpdate = true;
    }
  }

  
  void OGLSpriteBatcher::UpdateSprite(unsigned int iId, AABB2Df const& destRect, unsigned int iOrient, AABB2Df const& spriteRect)
  {
    if(iId > 0 && (iId - 1) <= m_Sprites.size())
    {
      SpriteElement& spriteElement = m_Sprites[iId - 1];

      if(spriteElement.texHandle == NULL)
        return;

      // This is the size of the original image data (pixels)
      Image::Size texSize = spriteElement.texHandle->GetTexture()->GetSize();

      Vector2i iSpriteSize(texSize.X(), texSize.Y());

      FillSpriteElem(spriteElement,destRect,spriteRect,iSpriteSize);
      spriteElement.orient = iOrient;

      needUpdate = true;
    }
  }

  
  void OGLSpriteBatcher::Hide(unsigned int iId)
  {
    if(registerSprites)
    {
      if(iId > 0 && (iId - 1) <= m_Sprites.size())
      {
        m_Sprites[iId - 1].pickId = NULL;
        dirty = true;
      }
    }
  }

  
  void OGLSpriteBatcher::Unhide(unsigned int iId, void* iData)
  {
    if(registerSprites)
    {
      if(iId > 0 && (iId - 1) <= m_Sprites.size())
      {
        m_Sprites[iId - 1].pickId = iData;
        dirty = true;
      }
    }
  }

  
  void OGLSpriteBatcher::RemoveSprite(unsigned int iId)
  {
    if(registerSprites)
    {
      if(iId > 0 && (iId - 1) <= m_Sprites.size())
      {
        m_Heap.Return(iId - 1);
        m_Sprites[iId - 1].pickId = NULL;
        m_Sprites[iId - 1].texHandle = NULL;
        dirty = true;
      }
    }
  }

  inline bool OGLSpriteBatcher::CompareLayer::operator ()(SpriteElement const * const& iElem1, SpriteElement const* const& iElem2)
  {
    return iElem1->layer < iElem2->layer;
  }

  namespace
  {
    bool CircleTest(Vector2f const& iPos, float iRadius, AABB2Df const& iBox)
    {
      if(iBox.Contains(iPos))
        return true;
      else
      {
        //Test segment X == X0
        float dist = iBox.m_Data[0].X() - iPos.X();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          float y = Mathf::Sqrt(dist) + iPos.Y();
          if(Mathf::ZERO_TOLERANCE < iBox.m_Data[1].Y() - y && y - iBox.m_Data[0].Y() > Mathf::ZERO_TOLERANCE)
            return true;
        }
        //Test segment X == X1
        dist = iBox.m_Data[1].X() - iPos.X();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          float y = Mathf::Sqrt(dist) + iPos.Y();
          if(Mathf::ZERO_TOLERANCE < iBox.m_Data[1].Y() - y && y - iBox.m_Data[0].Y() > Mathf::ZERO_TOLERANCE)
            return true;
        }

        //TestSegment Y == Y0
        dist = iBox.m_Data[0].Y() - iPos.Y();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          float x = Mathf::Sqrt(dist) + iPos.X();
          if(Mathf::ZERO_TOLERANCE < iBox.m_Data[1].X() - x && x - iBox.m_Data[0].X() > Mathf::ZERO_TOLERANCE)
            return true;
        }
        //Test segment Y == Y1
        dist = iBox.m_Data[1].Y() - iPos.Y();   //(x0-xc)
        dist = (iRadius + dist)*(iRadius - dist); //R^2 - (x0-xc)^2
        if(dist > 0)
        {
          float x = Mathf::Sqrt(dist) + iPos.X();
          if(Mathf::ZERO_TOLERANCE < iBox.m_Data[1].X() - x && x - iBox.m_Data[0].X() > Mathf::ZERO_TOLERANCE)
            return true;
        }
      }
      return false;
    }
  }

#if 0
  void OGLSpriteBatcher::Pick(Vector3f const& iPos, Vector3f const& iRay, float iRadius, unsigned int iMaxElem, std::vector<PickedElement>& oList, void* iUnpickable)
  {
    std::set<SpriteElement const *,CompareLayer > picked;

    Vector2f finalPos = Vector2f(iPos.X(),iPos.Y());
    
    for(unsigned int i = 0; i < m_Sprites.size(); ++i)
    {
      if(m_Sprites[i].pickId != NULL && m_Sprites[i].pickId != iUnpickable)
      {
        //if(offsetBox.Intersect(m_Sprites[i].boxPos))
        if(CircleTest(finalPos,iRadius,m_Sprites[i].boxPos))
        {
          picked.insert(&m_Sprites[i]);
        }
      }
    }

    float const boxHalfSize = iRadius * (1 + 1/ Mathf::Sqrt(2.0f))/2.0;

    AABB2Df pixTestBox(finalPos, Vector2f(2*boxHalfSize,2*boxHalfSize));

    unsigned int numPick = 0;
    std::set<SpriteElement const *,CompareLayer >::reverse_iterator iter = picked.rbegin();
    std::set<SpriteElement const *,CompareLayer >::reverse_iterator iterEnd = picked.rend();
    for (;iter!=iterEnd && numPick < iMaxElem ;++iter)
    {
      SpriteElement const* elem = (*iter);
      Bitmap const* bitmap = elem->texHandle->GetBitmap();
      AABB2Df texBox;
      texBox.m_Data[0].X() = elem->tx1;
      texBox.m_Data[1].X() = elem->tx2;
      texBox.m_Data[0].Y() = elem->ty1;
      texBox.m_Data[1].Y() = elem->ty2;
      if(bitmap->TestBox(pixTestBox,elem->boxPos,texBox))
      {
        oList.push_back(PickedElement(elem->pickId,elem->layer));
        ++numPick;
      }
    }
  }
#endif
  
  Bitmap const* OGLSpriteBatcher::GetBitmap(unsigned int iId)
  {
    if(iId < m_Sprites.size())
    {
      if(m_Sprites[iId].pickId != NULL)
      {
        return m_Sprites[iId].texHandle->GetBitmap();
      }
    }
    return NULL;
  }

  
  OGLSpriteBatcher::SpriteElement const* OGLSpriteBatcher::GetElement(unsigned int iId)
  {
    if(iId > 0 && (iId - 1) < m_Sprites.size())
    {
      if(m_Sprites[iId - 1].pickId != NULL)
        return &m_Sprites[iId - 1];
    }
    return NULL;
  }

  OGLSpriteBatcher::SpriteArray const* OGLSpriteBatcher::GetArray(unsigned int iId)
  {
    if(iId > 0 && (iId - 1) < m_SpritesCol.size())
    {
      if(m_SpritesCol[iId - 1].pickId != NULL)
        return &m_SpritesCol[iId - 1];
    }
    return NULL;
  }

  
  float* OGLSpriteBatcher::WriteGeomData(float* buffer, SpriteElement const* elem)
  {
    float z = m_Depth.X() + m_Depth.Z()*float(elem->layer);
    float tcOrig[] = {elem->tx1,elem->tx2,elem->ty1,elem->ty2};
    if(elem->orient & SpriteDesc::Flip_H)
    {
      float save = tcOrig[0];
      tcOrig[0] = tcOrig[1];
      tcOrig[1] = save;
    }
    if(elem->orient & SpriteDesc::Flip_V)
    {
      float save = tcOrig[2];
      tcOrig[2] = tcOrig[3];
      tcOrig[3] = save;
    }

    float tc[] = {tcOrig[0],tcOrig[3],
                  tcOrig[1],tcOrig[3],
                  tcOrig[1],tcOrig[2],
                  tcOrig[0],tcOrig[2]};

    unsigned int offset = 2 * (elem->orient & SpriteDesc::RotationMask) >> SpriteDesc::RotationShift;


    // 1st point (left bottom)
    *buffer=elem->boxPos.m_Data[0].X(); buffer++;
    *buffer=elem->boxPos.m_Data[0].Y(); buffer++;
    *buffer=z; buffer++;
    *buffer=tc[(0 + offset) % 8]; buffer++;
    *buffer=tc[(1 + offset) % 8]; buffer++;

    // 2st point (right top)
    *buffer=elem->boxPos.m_Data[1].X(); buffer++;
    *buffer=elem->boxPos.m_Data[1].Y(); buffer++;
    *buffer=z + m_AltOffset; buffer++;
    *buffer=tc[(4 + offset) % 8]; buffer++;
    *buffer=tc[(5 + offset) % 8]; buffer++;

    // 3rd point (left top)
    *buffer=elem->boxPos.m_Data[0].X(); buffer++;
    *buffer=elem->boxPos.m_Data[1].Y(); buffer++;
    *buffer=z + m_AltOffset; buffer++;
    *buffer=tc[(6 + offset) % 8]; buffer++;
    *buffer=tc[(7 + offset) % 8]; buffer++;
 
    // 4th point (left bottom)
    *buffer=elem->boxPos.m_Data[0].X(); buffer++;
    *buffer=elem->boxPos.m_Data[0].Y(); buffer++;
    *buffer=z; buffer++;
    *buffer=tc[(0 + offset) % 8]; buffer++;
    *buffer=tc[(1 + offset) % 8]; buffer++;

    // 5th point (right bottom)
    *buffer=elem->boxPos.m_Data[1].X(); buffer++;
    *buffer=elem->boxPos.m_Data[0].Y(); buffer++;
    *buffer=z; buffer++;
    *buffer=tc[(2 + offset) % 8]; buffer++;
    *buffer=tc[(3 + offset) % 8]; buffer++;

    // 6th point (right top)
    *buffer=elem->boxPos.m_Data[1].X(); buffer++;
    *buffer=elem->boxPos.m_Data[1].Y(); buffer++;
    *buffer=z + m_AltOffset; buffer++;
    *buffer=tc[(4 + offset) % 8]; buffer++;
    *buffer=tc[(5 + offset) % 8]; buffer++;

    return buffer;
  }

  
  void OGLSpriteBatcher::Update()
  {
    unsigned int newSize;

    newSize = (registerSprites ? (int)(m_Heap.GetCount()) : m_Sprites.size())*6;

    if (newSize<MINIMAL_HARDWARE_BUFFER_SIZE)
      newSize=MINIMAL_HARDWARE_BUFFER_SIZE;
 
    // grow hardware buffer if needed
    if (!m_DefaultBuffer || m_DefaultBuffer->GetBufferSize() <newSize*5*sizeof(float))
    {
      //DestroyHardwareBuffer(m_DefaultBuffer);
      if(m_DefaultBuffer)
        eXl_DELETE m_DefaultBuffer;

      m_DefaultBuffer = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER,newSize*5*sizeof(float),NULL);

      m_DefaultAssembly.m_IBuffer = NULL;
      m_DefaultAssembly.m_Attribs.clear();
      m_DefaultAssembly.AddAttrib(m_DefaultBuffer,OGLBaseAlgo::GetPosAttrib(),3,5*sizeof(float),0);
      m_DefaultAssembly.AddAttrib(m_DefaultBuffer,OGLBaseAlgo::GetTexCoordAttrib(),2,5*sizeof(float),3*sizeof(float));
    }
 
    // If we have no sprites this frame, bail here
    if (m_Sprites.empty() && m_SpritesCol.empty())
      return;

    if(dirty)
    {
      EmptyChunks();
      //Sort
      std::multiset<SpriteElement*,CompareSprite > sortedList;
      for(unsigned int i = 0; i< m_Sprites.size(); ++i)
      {
        SpriteElement& elem = m_Sprites[i];
        if(elem.pickId != NULL)
        {
          sortedList.insert(&elem);
        }
      }

      // write quads to the hardware buffer, and remember chunks
      float* buffer;
 
      buffer=(float*)m_DefaultBuffer->MapBuffer(OGLBufferAccess::WRITE);
      if(buffer != NULL)
      {
        unsigned int offset = 0;
        
        std::multiset<SpriteElement *,CompareSprite >::iterator currSpr = sortedList.begin();
        std::multiset<SpriteElement *,CompareSprite >::iterator endSpr = sortedList.end();

        if(currSpr != endSpr)
        {
          VertexChunk thisChunk;
          
          //thisChunk.texHandle=(*currSpr)->texHandle.get();
          thisChunk.vertexCount=0;
          unsigned int curLayer = (*currSpr)->layer;
          float curAlpha = (*currSpr)->alpha;
          SpriteDesc const* curSpriteDesc = (*currSpr)->texHandle.get();

          while (currSpr!=endSpr)
          {
            SpriteElement* elem = (*currSpr);
            
            elem->offsetInBuffer = offset;

            //thisChunk.alpha = elem->alpha;

            buffer = WriteGeomData(buffer,elem);

            // remember this chunk
            thisChunk.vertexCount+=6;

            offset += 30;

            currSpr++;
            if (currSpr==endSpr || curSpriteDesc!=(*currSpr)->texHandle.get() || curAlpha != (*currSpr)->alpha || curLayer != (*currSpr)->layer)
            {
              OGLShaderData* newData = eXl_NEW OGLShaderData;
              thisChunk.data = newData;
              thisChunk.m_Layer = curLayer;
              thisChunk.spriteDesc = curSpriteDesc;

              OGLTexture const* oglTexture = reinterpret_cast<OGLTexture const*>(curSpriteDesc->GetTexture());

              SpriteColor color;
              color.alphaMult = curAlpha;
              newData->AddTexture(OGLBaseAlgo::GetDiffuseTexture(),oglTexture);
              newData->AddData(OGLSpriteAlgo::GetSpriteColorUniform(),&color);

              m_Chunks.push_back(thisChunk);
              if (currSpr!=endSpr)
              {
                curSpriteDesc = (*currSpr)->texHandle.get();
                curAlpha = (*currSpr)->alpha;
                curLayer = (*currSpr)->layer;
                thisChunk.vertexCount=0;
              }
            }
          }
        }
      
        //hardwareBuffer->unlock();
        m_DefaultBuffer->UnmapBuffer();
      }
      else
        LOG_ERROR<<"Unable to map vertex buffer"<<"\n";

      if(cacheSprites)
      {
        dirty = false;
        needUpdate = false;
      }

    }
    else if(needUpdate)
    {
      float* initBuffer;
      initBuffer=(float*)m_DefaultBuffer->MapBuffer(OGLBufferAccess::WRITE);
      if(initBuffer != NULL)
      {
        for(unsigned int i=0;i<m_Sprites.size();++i)
        {
          SpriteElement* elem = &m_Sprites[i];
          if(elem->pickId == NULL)
            continue;

          float* buffer = initBuffer + elem->offsetInBuffer;
 
          buffer = WriteGeomData(buffer,elem);
        }
 
        //hardwareBuffer->unlock();
        m_DefaultBuffer->UnmapBuffer();
      }
      else
        LOG_ERROR<<"Unable to map vertex buffer"<<"\n";

      needUpdate = false;
    }
  }
  
  unsigned int OGLSpriteBatcher::CreateSpriteCollec(
    void* iData, 
    unsigned int layer, 
    unsigned int alpha, 
    unsigned iNum, 
    SpriteDesc const* const* iSprite, 
    AABB2Df const* destRect, 
    AABB2Df const* spriteRect, 
    unsigned char const* iOrient)
  {

    if(iNum == 0 || iData == 0)
      return 0 ;

    //OGLNode* parentNode = GetParent();

    std::vector<SpriteElement > sprites(iNum);
    for(unsigned int i = 0; i < iNum; ++i)
    {
      OGLTexture const* texturePtr = reinterpret_cast<OGLTexture const*>(iSprite[i]->GetTexture());
      if(texturePtr == NULL)
        continue;

      // Get texture handle from texture resource
      SpriteElement& spriteElement = sprites[i];
      spriteElement.texHandle = iSprite[i];
 
      // This is the size of the original image data (pixels)
      Vector2i iSpriteSize(texturePtr->GetSize().X(), texturePtr->GetSize().Y());

      FillSpriteElem(spriteElement,destRect[i],spriteRect[i],iSpriteSize);
 
      // save alpha value
      spriteElement.alpha = alpha;
      spriteElement.layer = layer;
      spriteElement.orient = iOrient[i];
      //if(parentNode->GetFlags() & GfxNodeDesc::FlipSpriteH)
      {
        if(iOrient[i] & SpriteDesc::Flip_H)
          spriteElement.orient &= ~SpriteDesc::Flip_H;
        else
          spriteElement.orient |= SpriteDesc::Flip_H;
      }
      //if(parentNode->GetFlags() & GfxNodeDesc::FlipSpriteV)
      {
        if(iOrient[i] & SpriteDesc::Flip_V)
          spriteElement.orient &= ~SpriteDesc::Flip_V;
        else
          spriteElement.orient |= SpriteDesc::Flip_V;
      }
    }

    std::multiset<SpriteElement*,CompareSprite > sortedList;
    for(unsigned int i = 0; i< sprites.size(); ++i)
    {
      SpriteElement& elem = sprites[i];
      sortedList.insert(&elem);
    }

    unsigned int colId = m_HeapCol.Get();
    if(colId>=m_SpritesCol.size())
    {
      m_SpritesCol.push_back(SpriteArray());
    }
      
    SpriteArray& newCol = m_SpritesCol[colId];

    unsigned int size = iNum*6;
  
    newCol.m_Buffer = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER,size*5*sizeof(float),NULL);
    newCol.m_Assembly.m_IBuffer = NULL;
    newCol.m_Assembly.AddAttrib(newCol.m_Buffer,OGLBaseAlgo::GetPosAttrib(),3,5*sizeof(float),0);
    newCol.m_Assembly.AddAttrib(newCol.m_Buffer,OGLBaseAlgo::GetTexCoordAttrib(),2,5*sizeof(float),3*sizeof(float));
    newCol.pickId = iData;
    newCol.decal = false;

    // write quads to the hardware buffer, and remember chunks
    float* buffer;
 
    buffer=(float*)newCol.m_Buffer->MapBuffer(OGLBufferAccess::WRITE);

    std::multiset<SpriteElement *,CompareSprite >::iterator currSpr = sortedList.begin();
    std::multiset<SpriteElement *,CompareSprite >::iterator endSpr = sortedList.end();

    VertexChunk thisChunk;
    //thisChunk.texHandle=(*currSpr)->texHandle;
    SpriteDesc const* curSpriteDesc = (*currSpr)->texHandle.get();
    float curAlpha = (*currSpr)->alpha;
    thisChunk.vertexCount=0;
    while (currSpr!=endSpr)
    {
      SpriteElement* elem = (*currSpr);
      
      //thisChunk.alpha = elem->alpha;

      buffer = WriteGeomData(buffer,elem);
 
      // remember this chunk
      thisChunk.vertexCount+=6;

      currSpr++;
      if (currSpr==endSpr || curSpriteDesc!=(*currSpr)->texHandle.get() || curAlpha != (*currSpr)->alpha)
      {
        OGLShaderData* newData = eXl_NEW OGLShaderData;
        thisChunk.data = newData;
        thisChunk.m_Layer = layer;
        thisChunk.spriteDesc = curSpriteDesc;

        OGLTexture const* oglTexture = reinterpret_cast<OGLTexture const*>(curSpriteDesc->GetTexture());

        SpriteColor color;
        color.alphaMult = curAlpha;
        newData->AddTexture(OGLBaseAlgo::GetDiffuseTexture(),oglTexture);
        newData->AddData(OGLSpriteAlgo::GetSpriteColorUniform(),&color);
               

        newCol.m_Chunks.push_back(thisChunk);
        if (currSpr!=endSpr)
        {
          curSpriteDesc = (*currSpr)->texHandle.get();
          curAlpha = (*currSpr)->alpha;
          thisChunk.vertexCount=0;
        }
      }
    }


    newCol.m_Buffer->UnmapBuffer();

    return colId + 1;
  }

  
  void OGLSpriteBatcher::EraseSpriteCollec(unsigned int iId)
  {
    if((iId - 1) < m_SpritesCol.size() && m_SpritesCol[iId - 1].pickId != 0)
    {
      SpriteArray& col = m_SpritesCol[iId - 1];
      eXl_DELETE col.m_Buffer;
      col.m_Buffer = NULL;
      col.pickId = 0;
      for(unsigned int i = 0 ; i< col.m_Chunks.size(); ++i)
      {
        col.m_Chunks[i].Empty();
      }
      col.m_Chunks.clear();
      col.m_Assembly.m_Attribs.clear();
      m_HeapCol.Return(iId-1);
    }
    else
    {
      LOG_WARNING << "Invalid sprite collection deletion" << "\n";
    }
  }

#if 0
  unsigned int OGLSpriteBatcher::CreateText(String const& iFont, String const& iText, unsigned int layer, unsigned int iColor, Vector2f const& iPos, Vector2f const& iScale, FontManager::Anchor iAnchor)
  {
    std::vector<AABB2Df> pos;
    pos.reserve(iText.length());
    std::vector<AABB2Df> tc;
    tc.reserve(iText.length());

    OGLTexture* texturePtr = m_FontMgr.WriteText(iFont, iText, iScale, pos, tc, iAnchor);
    if(texturePtr == NULL)
      return 0;

    Vector2i iSpriteSize = texturePtr->GetSize();

    OGLNode* parentNode = GetParent();

    std::vector<SpriteElement > sprites(pos.size());
    for(unsigned int i = 0; i < pos.size(); ++i)
    {
      // Get texture handle from texture resource
      SpriteElement& spriteElement = sprites[i];
      spriteElement.texHandle = NULL;
 
      // This is the size of the original image data (pixels)
      //AABB2Df destRect(pos[i].GetCenter() + iPos, Vector2f(pos[i].GetSize().X(), pos[i].GetSize().Y()));
      AABB2Df destRect(pos[i]);
      destRect.m_Data[0] = destRect.m_Data[0] + iPos;
      destRect.m_Data[1] = destRect.m_Data[1] + iPos;
      FillSpriteElem(spriteElement,destRect,tc[i],iSpriteSize);
 
      // save alpha value
      spriteElement.alpha = float(iColor >> 24) / 255.0;
      spriteElement.layer = layer;
      spriteElement.orient = GfxSprite::Rot_0;
      if(parentNode->GetFlags() & GfxNodeDesc::FlipSpriteH)
      {
        spriteElement.orient |= GfxSprite::Flip_H;
      }
      if(parentNode->GetFlags() & GfxNodeDesc::FlipSpriteV)
      {
        spriteElement.orient |= GfxSprite::Flip_V;
      }
    }

    unsigned int colId = m_HeapCol.Get();
    if(colId>=m_SpritesCol.size())
    {
      m_SpritesCol.push_back(SpriteArray());
    }
      
    SpriteArray& newCol = m_SpritesCol[colId];

    unsigned int size = pos.size()*6;
  
    newCol.m_Buffer = OGLBuffer::CreateBuffer(GL_ARRAY_BUFFER,size*5*sizeof(float),NULL);
    newCol.m_Assembly.m_IBuffer = NULL;
    newCol.m_Assembly.AddAttrib(newCol.m_Buffer,OGLBaseAlgo::GetPosAttrib(),3,5*sizeof(float),0);
    newCol.m_Assembly.AddAttrib(newCol.m_Buffer,OGLBaseAlgo::GetTexCoordAttrib(),2,5*sizeof(float),3*sizeof(float));
    newCol.pickId = (void*)1;
    newCol.decal = true;
    //newCol.color = iColor;

    float* buffer;
 
    buffer=(float*)newCol.m_Buffer->MapBuffer(GL_WRITE_ONLY);

    VertexChunk thisChunk;
    //thisChunk.texHandle=texturePtr;
    thisChunk.vertexCount=pos.size() * 6;
    //thisChunk.alpha = 
    
    SpriteColor color;
    color.alphaMult = float(iColor >> 24) / 255.0f;
    color.color = Vector4f(iColor & 255,(iColor >> 8) & 255,(iColor >> 16) & 255,255) / 255.0f;
    OGLShaderData* newData = eXl_NEW OGLShaderData;
    newData->AddData(OGLSpriteAlgo::GetSpriteColorUniform(),&color);
    newData->AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(),texturePtr);
    //float curAlpha = 
    thisChunk.data = newData;
    thisChunk.m_Layer = layer;

    for(unsigned int i = 0; i<sprites.size();++i)
    {
      SpriteElement* elem = &sprites[i];
      buffer = WriteGeomData(buffer,elem);
    }

    newCol.m_Chunks.push_back(thisChunk);

    newCol.m_Buffer->UnmapBuffer();

    return colId + 1;
  }
#endif
  
  //void OGLSpriteBatcher::EraseText(unsigned int iId)
  //{
  //  EraseSpriteCollec(iId);
  //}

  //void OGLSpriteBatcher::ShutdownAPI(GfxSys* iSys)
  //{
  //  EmptyChunks();
  //  if(m_DefaultBuffer != NULL)
  //  {
  //    eXl_DELETE m_DefaultBuffer;
  //    m_DefaultBuffer = NULL;
  //  }
  //  m_DefaultAssembly.m_Attribs.clear();
  //  dirty = true;
  //}
  //
  //void OGLSpriteBatcher::RestoreAPI(GfxSys* iSys)
  //{
  //
  //}
}