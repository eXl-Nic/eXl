/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/spriterenderer.hpp>

namespace eXl
{
  class GfxSpriteRenderNode : public GfxRenderNode
  {
    DECLARE_RTTI(GfxSpriteRenderNode, GfxRenderNode);
  public:

    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override;
    void Push(OGLDisplayList& iList, float iDelta) override;
    TransformUpdateCallback GetTransformUpdateCallback() override;
    UpdateCallback GetDeleteCallback() override;

    void AddObject(ObjectHandle);
    void SetSpriteDirty(ObjectHandle iObj);

    OGLCompiledProgram const* GetSpriteProgram() { return m_Renderer->m_SpriteProgram.get(); }

  protected:

    void RemoveObject(ObjectHandle);

    Optional<DenseGameDataStorage<GfxSpriteData>> m_SpriteData;
    Optional<SpriteRenderer> m_Renderer;
  };
}