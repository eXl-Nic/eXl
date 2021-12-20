/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>

#include <engine/gfx/gfxcomponent.hpp>

namespace eXl
{
  class GfxSpriteRenderNode : public GfxRenderNode
  {
  public:

    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override;
    void Push(OGLDisplayList& iList, float iDelta) override;
    TransformUpdateCallback GetTransformUpdateCallback() override;
    UpdateCallback GetDeleteCallback() override;

    void AddObject(ObjectHandle);
    GfxSpriteComponent* GetComponent(ObjectHandle);
    void SetSpriteDirty(GfxSpriteComponent& iComp);

    OGLCompiledProgram const* GetSpriteProgram() { return m_SpriteProgram.get(); }

  protected:

    static SpriteDataTable::Handle GetSpriteData(GfxSpriteComponent& iComp);
    static Vector2f const& GetSpriteOffset(GfxSpriteComponent& iComp);
    void RemoveObject(ObjectHandle);
    void PrepareSprites();

    UniquePtr<OGLCompiledProgram const> m_SpriteProgram;

    SpriteDataTable m_SpriteData;
    SpriteComponents m_SpriteComp;
    Vector<SpriteComponents::Handle> m_ObjectToSpriteComp;
    UnorderedSet<GfxSpriteComponent*> m_DirtyComponents;
    UnorderedMap<Vector3f, IntrusivePtr<GeometryInfo>> m_SpriteGeomCache;
    IntrusivePtr<OGLBuffer> m_DefaultSpriteIdxBuffer;
  };
}