/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/idgenerator.hpp>
#include <core/image/image.hpp>
#include <math/aabb2d.hpp>
//#include <gametk/gfx/fontmanager.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/oglrendercommand.hpp>
//#include <gametk/bitmap.hpp>
//#include <gametk/spritedesc.hpp>

#include <math/vector2.hpp>
#include <math/aabb2d.hpp>

namespace eXl
{
  class Image;

  struct EXL_OGL_API Bitmap : public HeapObject
  {
    bool TestBox(AABB2Df const& iPickBox, AABB2Df const& iBoxPos, AABB2Df const& iBoxTex) const;
    uint32_t m_Width;
    uint32_t m_Height;
    Vector<bool> m_Pixels;
  };

  struct AnimationDesc
  {
    enum Kind
    {
      Loop,
      Once,
      PingPong
    };

    Vector2f anim;
    Vector2f step;
    Vector2i size;
    Kind animKind;
    float time;
  };

  class EXL_OGL_API SpriteDesc : public HeapObject
  {
    DECLARE_RefC;

  public:

    enum SpriteFlags
    {
      Pickable = 1 << 0,
      RotationShift = 1,
      RotationMask = 3 << RotationShift,
      Rot_0    = 0 << 1,
      Rot_90   = 1 << 1,
      Rot_180  = 2 << 1,
      Rot_270  = 3 << 1,
      FlipShift = 3,
      FlipMask = 3 << (FlipShift + 0),
      Flip_H   = 1 << (FlipShift + 0),
      Flip_V   = 1 << (FlipShift + 1)
    };

    SpriteDesc(AString const& iFileName);
	SpriteDesc(Image* iImage, Image::Storage iStorage);
    ~SpriteDesc();

    void AddAnim(AnimationDesc::Kind iAnimFlag,float iTime,Vector2f const& iOrig,Vector2f const& iStep,Vector2i const& iSize);

    AString const& GetFileName()const{return m_FileName;}

    Vector<AnimationDesc> const& GetAnimationTable()const{return m_Anims;}

    enum
    {
      PreloadImage   = 1<<0,
      PreloadBitmap  = 1<<1,
      PreloadTexture = 1<<2
    };

    void Preload(uint32_t iPreloadFlags) const;

    OGLTexture const* GetTexture() const;

    void UnloadTexture() const;

    Bitmap const* GetBitmap() const;

    Image const* GetImage() const;

  protected:

    Vector<AnimationDesc> m_Anims;
    AString m_FileName;

    mutable Image::Size m_Size;
    mutable IntrusivePtr<OGLTexture> m_Texture;
    mutable boost::optional<Image>   m_Image;
    mutable boost::optional<Bitmap>  m_Bitmap;
  };

  class OGLTexture;
  //class OGLFontManager;
  class OGLTextureLoader;
  class OGLDisplayList;

  enum OSMetrics 
  {
    OSPRITE_METRIC_ABS,
    OSPRITE_METRIC_PIXELS,
    OSPRITE_METRIC_RELATIVE,
    OSPRITE_METRIC_SCREEN
  };

  class EXL_OGL_API OGLSpriteBatcher
  {
  public: 

    struct SpriteElement
    {
      //~SpriteElement();
      AABB2Df boxPos;// sprite coordinates
      float tx1, ty1, tx2, ty2;// texture coordinates
      void* pickId;
      IntrusivePtr<SpriteDesc const> texHandle;// texture handle
      float alpha;
      uint32_t offsetInBuffer;
      unsigned short layer;
      unsigned short orient;
    };

    struct VertexChunk 
    {
      void Empty();
      IntrusivePtr<SpriteDesc const> spriteDesc;
      OGLShaderData const* data;
      uint32_t vertexCount;
      uint32_t m_Layer;
    };

    struct SpriteArray
    {
      void* pickId;
      std::vector<VertexChunk> m_Chunks;
      OGLVAssembly m_Assembly;
      OGLBuffer* m_Buffer;
      bool decal;
    };

    static AABB2Df const FULL_SPRITE;
    //static AABB2Df const FULL_SCREEN;

    /// Default constructor
    OGLSpriteBatcher(float iAltCoeff, Vector3f const& iDepth/*, OGLFontManager& iFontMgr*/);

    /// Destructor
    virtual ~OGLSpriteBatcher();
 
    //void Shutdown(void);

    void SetAlphaCoeff(float iAlpha);

    uint32_t AddSprite(void* iData, SpriteDesc const* iSprite, float alpha,uint32_t layer, uint32_t iOrient, AABB2Df const& destRect , AABB2Df const& spriteRect = FULL_SPRITE);

    void UpdateSprite(uint32_t iId, AABB2Df const& destRect);

    void UpdateSprite(uint32_t iId, AABB2Df const& destRect, uint32_t iOrient, AABB2Df const& spriteRect);

    void RemoveSprite(uint32_t iId);

    uint32_t CreateSpriteCollec(void* iData, uint32_t layer, uint32_t alpha, unsigned iNum, SpriteDesc const* const* iSprite, AABB2Df const* destRect, AABB2Df const* spriteRect, uint8_t const* iOrient);

    void EraseSpriteCollec(uint32_t iId);

    //uint32_t CreateText(String const& iFont, String const& iText, uint32_t iLayer, uint32_t iColor, Vector2f const& iPos, Vector2f const& iScale, FontManager::Anchor iAnchor);

    //void EraseText(uint32_t iId);

    void Hide(uint32_t iId);

    void Unhide(uint32_t iId, void* iData);

    //void Pick(Vector3f const& iPos, Vector3f const& iRay, float iRadius, uint32_t iMaxElem, std::vector<PickedElement>& oList, void* iUnpickable);

    Bitmap const* GetBitmap(uint32_t iElem);

    void Update();

    void BatchRender(OGLDisplayList& iList, uint8_t iPrefix);

    //void ShutdownAPI(GfxSys* iSys);

    //void RestoreAPI(GfxSys* iSys);

    SpriteElement const* GetElement(uint32_t iId);

    SpriteArray const* GetArray(uint32_t iId);

    inline OGLVAssembly const* GetDefaultAssembly() const{return &m_DefaultAssembly;}
    //inline OGLTextureLoader* GetTextureLoader() const{return m_Loader;}
 
    void StealSprites(Vector<SpriteElement>& oElems, Vector<VertexChunk>& oChunks, OGLVAssembly& oAssembly, OGLBuffer*& oBuffer);

    void StealCollections(Vector<SpriteArray>& oCollecs);

  protected:

    //void Visit(OGLNodeVisitor* iVisitor);

    struct CompareSprite
    {
      inline bool operator ()(SpriteElement const * const& iElem1, SpriteElement const* const& iElem2);
    };

    struct CompareLayer
    {
      inline bool operator ()(SpriteElement const * const& iElem1, SpriteElement const* const& iElem2);
    };

    void FillSpriteElem(SpriteElement& iElem, AABB2Df const& spriteRect, AABB2Df const& texRect, Vector2i const& iSpriteSize);

    float* WriteGeomData(float* oGeom, SpriteElement const* iElem);
 
    OGLTexture* LoadSprite(const String& spriteName);

    //void convertScreenMetrics(OSMetrics metricFrom, const float sx, const float sy, OSMetrics metricTo, float& dx, float& dy);
 
    //OGLTextureLoader* m_Loader;
    //OGLFontManager& m_FontMgr;

    Vector<SpriteElement> m_Sprites;

    Vector<VertexChunk>   m_Chunks;

    void EmptyChunks();

    OGLVAssembly m_DefaultAssembly;
    OGLBuffer* m_DefaultBuffer;

    IdGenerator m_HeapCol;
    Vector<SpriteArray> m_SpritesCol;

    IdGenerator m_Heap; 
    //Set<SpriteElement*,CompareSprite> 
    bool dirty;
    bool needUpdate;

    bool cacheSprites;
    bool registerSprites;
    
    float alphaCoeff;
    float m_AltOffset;

    Vector3f m_Depth;
  };
}
