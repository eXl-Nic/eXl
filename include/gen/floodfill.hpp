/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <gen/gen_exp.hpp>
#include <math/aabb2dpolygon.hpp>

namespace eXl
{
  //Pour le moment, ne marche que sur une seule composante connexe.
  class EXL_GEN_API FloodFill
  {
  public:
    template <typename T>
    struct ValidOperator
    {
      inline bool operator()(T iVal) const {return !(!iVal);}
    };

    template <typename T, typename ValidOp = ValidOperator<T> >
    static void MakePolygon(Vector<T> const& iBitmap, AABB2Di const& iBox, ValidOp const& iOp, AABB2DPolygoni& oPoly);

    template <typename T, typename ValidOp = ValidOperator<T> >
    static void MakePolygons(Vector<T> const& iBitmap, AABB2Di const& iBox, ValidOp const& iOp, Vector<AABB2DPolygoni>& oPolys);

    static void MakePolygon(Vector<char>& iExtendedBitmap, AABB2Di const& iExtendedBox, AABB2DPolygoni& oPoly);

    static uint32_t ExtractComponents(Vector<char> const& iMap, AABB2Di const& iBox, Vector<uint32_t>& oCompMap);

    static unsigned int MakeGradientMap(AABB2DPolygoni const& iPoly, Vector<unsigned int>& oValues, Vector<Vector2i>& oGradient);

  protected:

    static constexpr char In_Tag = 0;
    static constexpr char Out_Tag = -1;
    static constexpr char OutVisited_Tag = -2;
    static constexpr char Corner_Tag = -3;
    static constexpr char Border_Tag = -4;

    static void Fill(Vector2i const& iStartPt, Vector<char>& iGrid , Vector<Vector2i>& oBorder, bool iExt, AABB2Di const& iBox);
    static bool ExamineNeigh(Vector<Vector2i>& ioList, Vector<char>& iGrid, Vector2i const& iPos, AABB2Di const& iBox);
  };

  template <typename T, typename ValidOp>
  void FloodFill::MakePolygon(Vector<T> const& iBitmap, AABB2Di const& iBox, ValidOp const& iOp, AABB2DPolygoni& oPoly)
  {
    AABB2Di extendedBox = iBox;
    extendedBox.m_Data[0] = extendedBox.m_Data[0] - Vector2i::ONE;
    extendedBox.m_Data[1] = extendedBox.m_Data[1] + Vector2i::ONE;
    Vector2i dim = extendedBox.GetSize();
    Vector<char> grid(dim.X() * dim.Y(), Out_Tag);
    int32_t offsetOrig = 0;
    for(int32_t i = 0; i< dim.Y() - 2; ++i)
    {
      int32_t offsetDest = 1 + (i + 1) * dim.X();
      for(int32_t j = 0; j< dim.X() - 2; ++j)
      {
        grid[offsetDest] = iOp(iBitmap[offsetOrig]) ? In_Tag : Out_Tag;
        ++offsetOrig;
        ++offsetDest;
      }
    }

    MakePolygon(grid, extendedBox, oPoly);
  }

  template <typename T, typename ValidOp>
  void FloodFill::MakePolygons(Vector<T> const& iBitmap, AABB2Di const& iBox, ValidOp const& iOp, Vector<AABB2DPolygoni>& oPolys)
  {
    oPolys.clear();

    AABB2Di extendedBox = iBox;
    extendedBox.m_Data[0] = extendedBox.m_Data[0] - Vector2i::ONE;
    extendedBox.m_Data[1] = extendedBox.m_Data[1] + Vector2i::ONE;
    Vector2i dim = extendedBox.GetSize();
    Vector<char> grid(dim.X() * dim.Y(), Out_Tag);
    unsigned int offsetOrig = 0;
    for (int32_t i = 0; i < dim.Y() - 2; ++i)
    {
      int32_t offsetDest = 1 + (i + 1) * dim.X();
      for (int32_t j = 0; j < dim.X() - 2; ++j)
      {
        grid[offsetDest] = iOp(iBitmap[offsetOrig]) ? In_Tag : Out_Tag;
        ++offsetOrig;
        ++offsetDest;
      }
    }

    Vector<uint32_t> compMap;
    uint32_t const numComps = ExtractComponents(grid, extendedBox, compMap);

    for (uint32_t compIdx = 0; compIdx < numComps; ++compIdx)
    {
      for (int32_t i = 0; i < dim.Y(); ++i)
      {
        int32_t offsetDest = i * dim.X();
        for (int32_t j = 0; j < dim.X(); ++j)
        {
          if (compMap[offsetDest] == compIdx)
          {
            grid[offsetDest] = In_Tag;
          }
          else
          {
            grid[offsetDest] = Out_Tag;
          }
          ++offsetDest;
        }
      }
      AABB2DPolygoni poly;
      MakePolygon(grid, extendedBox, poly);

      oPolys.push_back(std::move(poly));
    }
  }
}