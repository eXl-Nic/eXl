/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include <engine/common/debugtool.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglrendercommand.hpp>

namespace eXl
{
  class OGLCompiledProgram;
  class OGLTexture;
  class OGLDisplayList;
  class OGLBuffer;

  class GfxDebugDrawer : public DebugTool::Drawer, public GfxRenderNode
  {
    friend GfxSystem;

  public:

    void Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle) override;
    void Push(OGLDisplayList& iList, float iDelta) override;
    void DrawLine(const Vector3f& iFrom, const Vector3f& iTo, const Vector4f& iColor, bool iScreenSpace = false) override;
    void DrawBox(AABB2Df const& iBox, const Vector4f& iColor, bool iScreenSpace = false) override;
    void DrawConvex(Vector<Vector2f> const& iConvex, const Vector4f& iColor, bool iScreenSpace) override;

    OGLCompiledProgram const* GetLineProgram() { return m_LineProgram.get(); }

  protected:
    
    UniquePtr<OGLCompiledProgram const> m_TriangleProgram;
    UniquePtr<OGLCompiledProgram const> m_LineProgram;
    IntrusivePtr<OGLTexture> m_DebugTexture;
    IntrusivePtr<OGLBuffer> m_GeomLines;
    IntrusivePtr<OGLBuffer> m_GeomBoxes;

    Matrix4f m_IdentMatrix;
    OGLShaderData m_WorldTransform;
    OGLVAssembly m_GeomAssemblyL;
    OGLVAssembly m_GeomAssemblyB;

    // Debugdrawer data
    Vector<Vector<Vector3f>> m_Lines;
    Vector<Vector<AABB2Df>> m_Boxes;
    Vector<Vector<Vector<Vector2f>>> m_Convex;
    Map<Vector4f, uint32_t> m_Colors;

    // Rendering data
    Vector<SpriteColor> spriteInfo;
    Vector<OGLShaderData> colors;
    Vector<uint32_t> lineNums;
    Vector<uint32_t> boxNums;
    Vector<uint32_t> convexNumPts;

    float m_CurrentScreenSize;
  };
}