/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/gfxsystem.hpp>
#include "gfxdebugdrawer.hpp"

#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/oglutils.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  namespace
  {
    Image BitmapToImage(uint8_t const* iBitmap, Image::Size iImageSize)
    {
      uint32_t numPixels = iImageSize.x * iImageSize.y;
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

  void GfxDebugDrawer::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);

    OGLSemanticManager& semantics = iSys.GetSemanticManager();

    OGLLineAlgo::Init(semantics);
    OGLSpriteAlgo::Init(semantics);
    m_LineProgram.reset(OGLLineAlgo::CreateProgram(semantics));
    m_TriangleProgram.reset(OGLSpriteAlgo::CreateSpriteProgram(semantics));

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
    m_DebugTexture = OGLTextureLoader::CreateFromImage(&whiteImage, true);
    m_IdentMatrix = Identity<Mat4>();
  }

  void GfxDebugDrawer::Push(OGLDisplayList& iList, float iDelta)
  {
    iList.SetDepth(false, false);

    spriteInfo.clear();
    colors.clear();
    lineNums.clear();
    boxNums.clear();
    convexNumPts.clear();

    spriteInfo.resize(m_Colors.size());
    colors.resize(m_Colors.size());
    lineNums.resize(m_Colors.size(), 0);
    boxNums.resize(m_Colors.size(), 0);
    convexNumPts.resize(m_Colors.size(), 0);

    for (auto const& matEntry : m_Colors)
    {
      spriteInfo[matEntry.second].alphaMult = 1.0f;
      spriteInfo[matEntry.second].tint = matEntry.first;
      colors[matEntry.second].AddData(OGLSpriteAlgo::GetSpriteColorUniform(), spriteInfo.data() + matEntry.second);
      colors[matEntry.second].AddData(OGLLineAlgo::GetColor(), &spriteInfo[matEntry.second].tint);
      colors[matEntry.second].AddData(OGLBaseAlgo::GetWorldMatUniform(), &m_IdentMatrix);
      colors[matEntry.second].AddTexture(OGLBaseAlgo::GetDiffuseTexture(), m_DebugTexture.get());

      if (m_Lines.size() > matEntry.second)
      {
        lineNums[matEntry.second] = m_Lines[matEntry.second].size();
      }
      if (m_Boxes.size() > matEntry.second)
      {
        boxNums[matEntry.second] = m_Boxes[matEntry.second].size();
      }
      if (m_Convex.size() > matEntry.second)
      {
        for (auto const& cvx : m_Convex[matEntry.second])
        {
          convexNumPts[matEntry.second] += 3 * cvx.size();
        }
      }
    }

    uint32_t totNumLines = 0;
    for (auto& offset : lineNums)
    {
      uint32_t numLines = offset;
      offset = totNumLines;
      totNumLines += numLines;
    }
    uint32_t totNumBoxes = 0;
    for (auto& offset : boxNums)
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

    if (!m_Lines.empty())
    {
      iList.SetProgram(m_LineProgram.get());
      size_t const bufferSize = totNumLines * sizeof(Vec3);

      if (!m_GeomLines || m_GeomLines->GetBufferSize() < bufferSize)
      {
        m_GeomLines = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, bufferSize, nullptr);
      }

      for (uint32_t i = 0; i < m_Lines.size(); ++i)
      {
        auto const& linesDL = m_Lines[i];

        size_t dataSize = linesDL.size() * sizeof(Vec3);
        m_GeomLines->SetData(lineNums[i] * sizeof(Vec3), dataSize, (void*)linesDL.data());
      }

      m_GeomAssemblyL.m_Attribs.clear();
      m_GeomAssemblyL.AddAttrib(m_GeomLines, OGLBaseAlgo::GetPosAttrib(), 3, sizeof(Vec3), 0);
      m_GeomAssemblyL.m_IBuffer = nullptr;

      iList.SetVAssembly(&m_GeomAssemblyL);
      iList.SetDepth(false, false);

      for (auto const& matEntry : m_Colors)
      {
        if (m_Lines.size() > matEntry.second)
        {
          iList.PushData(colors.data() + matEntry.second);

          auto const& linesDL = m_Lines[matEntry.second];

          iList.PushDraw(0x1000, OGLDraw::LineList, linesDL.size(), lineNums[matEntry.second], 0);

          iList.PopData();
        }
      }
    }

    if (!m_Boxes.empty()
      || !m_Convex.empty())
    {
      iList.SetProgram(m_TriangleProgram.get());

      Vector<Vec3> triangles;
      triangles.resize(totNumBoxes * 6 + totNumConvexPt);

      for (uint32_t i = 0; i < m_Boxes.size(); ++i)
      {
        auto const& boxDL = m_Boxes[i];

        Vec3* ptData = triangles.data() + 6 * boxNums[i];

        for (uint32_t j = 0; j < boxDL.size(); ++j)
        {
          auto const& box = boxDL[j];

          *(ptData++) = (Vec3(box.m_Data[0].x, box.m_Data[0].y, 0.0));
          *(ptData++) = (Vec3(box.m_Data[1].x, box.m_Data[0].y, 0.0));
          *(ptData++) = (Vec3(box.m_Data[0].x, box.m_Data[1].y, 0.0));
          *(ptData++) = (Vec3(box.m_Data[1].x, box.m_Data[0].y, 0.0));
          *(ptData++) = (Vec3(box.m_Data[1].x, box.m_Data[1].y, 0.0));
          *(ptData++) = (Vec3(box.m_Data[0].x, box.m_Data[1].y, 0.0));
        }
      }

      for (uint32_t i = 0; i < m_Convex.size(); ++i)
      {
        auto const& cvxDL = m_Convex[i];
        Vec3* ptData = triangles.data() + convexOffset[i];
        for (auto const& cvx : cvxDL)
        {
          if (cvx.empty())
          {
            continue;
          }
          Vec2 midPt;
          for (auto const& pt : cvx)
          {
            midPt += pt;
          }
          midPt /= cvx.size();
          for (uint32_t j = 0; j < cvx.size(); ++j)
          {
            uint32_t nextPt = (j + 1) % cvx.size();
            *(ptData++) = Vec3(midPt.x, midPt.y, 0.0);
            *(ptData++) = Vec3(cvx[j].x, cvx[j].y, 0.0);
            *(ptData++) = Vec3(cvx[nextPt].x, cvx[nextPt].y, 0.0);
          }
        }
      }

      {
        size_t const totBufferSize = (totNumBoxes * 6 + totNumConvexPt) * sizeof(Vec3);
        if (m_GeomBoxes == nullptr || m_GeomBoxes->GetBufferSize() < totBufferSize)
        {
          m_GeomBoxes = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, totBufferSize, (void*)triangles.data());
        }
      }
      //debugGeomBoxes->SetData(0, triangles.size() * sizeof(Vec3), (void*)triangles.data());
      m_GeomAssemblyB.m_Attribs.clear();
      m_GeomAssemblyB.AddAttrib(m_GeomBoxes, OGLBaseAlgo::GetPosAttrib(), 3, sizeof(Vec3), 0);
      m_GeomAssemblyB.m_IBuffer = nullptr;

      iList.SetVAssembly(&m_GeomAssemblyB);
      iList.SetDepth(false, false);

      for (auto const& matEntry : m_Colors)
      {
        if (m_Boxes.size() > matEntry.second
          || m_Convex.size() > matEntry.second)
        {
          iList.PushData(colors.data() + matEntry.second);
          if (m_Boxes.size() > matEntry.second)
          {
            auto const& boxDL = m_Boxes[matEntry.second];
            iList.PushDraw(0x1000, OGLDraw::TriangleList, boxDL.size() * 6, boxNums[matEntry.second] * 6, 0);
          }
          if (m_Convex.size() > matEntry.second)
          {
            iList.PushDraw(0x1000, OGLDraw::TriangleList, convexNumPts[matEntry.second], convexOffset[matEntry.second], 0);
          }
          iList.PopData();
        }
      }
    }

    m_Lines.clear();
    m_Colors.clear();
    m_Boxes.clear();
    m_Convex.clear();
  }

  void GfxDebugDrawer::DrawLine(const Vec3& iFrom, const Vec3& iTo, const Vec4& iColor, bool iScreenSpace)
  {
    auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
    while (m_Lines.size() <= matEntry->second)
    {
      m_Lines.push_back(Vector<Vec3>());
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

  void GfxDebugDrawer::DrawBox(AABB2Df const& iBox, const Vec4& iColor, bool iScreenSpace)
  {
    auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
    while (m_Boxes.size() <= matEntry->second)
    {
      m_Boxes.push_back(Vector<AABB2Df>());
    }

    auto& boxes = m_Boxes[matEntry->second];
    boxes.push_back(iBox);
  }

  void GfxDebugDrawer::DrawConvex(Vector<Vec2> const& iConvex, const Vec4& iColor, bool iScreenSpace)
  {
    auto matEntry = m_Colors.insert(std::make_pair(iColor, (uint32_t)m_Colors.size())).first;
    while (m_Convex.size() <= matEntry->second)
    {
      m_Convex.push_back(Vector<Vector<Vec2>>());
    }

    auto& boxes = m_Convex[matEntry->second];
    boxes.push_back(iConvex);
  }
}