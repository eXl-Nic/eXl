/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/maptiler.hpp>

#include <engine/map/tilinggroup.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <math/mathtools.hpp>
#include <numeric>
#include <bitset>

namespace eXl
{
  namespace MapTiler
  {
    bool TexGroup::operator==(TexGroup const& iOther) const
    {
      return m_Tileset == iOther.m_Tileset
        && m_Name == iOther.m_Name
        && m_VtxScaling == iOther.m_VtxScaling
        && m_VtxOffset == iOther.m_VtxOffset
        && m_Tiling == iOther.m_Tiling;
    }

    size_t hash_value(TexGroup const& iGroup)
    {
      size_t value = ptrdiff_t(iGroup.m_Tileset);
      boost::hash_combine(value, iGroup.m_Name);
      boost::hash_combine(value, iGroup.m_VtxScaling.x);
      boost::hash_combine(value, iGroup.m_VtxScaling.y);
      boost::hash_combine(value, iGroup.m_VtxOffset.x);
      boost::hash_combine(value, iGroup.m_VtxOffset.y);
      boost::hash_combine(value, iGroup.m_Tiling.x);
      boost::hash_combine(value, iGroup.m_Tiling.y);
      return value;
    }

    Vector<float>& Batcher::GetGroup(TexGroup const& iGroup)
    {
      uint32_t groupIdx = textures.insert(std::make_pair(iGroup, textures.size())).first->second;

      allTiles.resize(Mathi::Max(groupIdx + 1, allTiles.size()));
      return allTiles[groupIdx];
    }

    void Batcher::Finalize(GfxSystem& iGfx, ObjectHandle iObject, uint8_t iLayer)
    {
      Vector<uint32_t> numVtx;
      Vector<uint32_t> offset;
      uint32_t totNumVtx = 0;
      for (auto const& group : allTiles)
      {
        offset.push_back(totNumVtx);
        numVtx.push_back(group.size() / 5);
        totNumVtx += group.size() / 5;
      }
      if (totNumVtx == 0)
      {
        return;
      }
#ifdef EXL_WITH_OGL
      OGLBuffer* buffer;
      OGLVAssembly assembly;

      buffer = OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, totNumVtx * 5 * sizeof(float), nullptr);

      float* bufferData = (float*)buffer->MapBuffer(OGLBufferAccess::WRITE);
      float* dest = bufferData;
      for (auto const& group : allTiles)
      {
        std::copy(group.begin(), group.end(), dest);
        dest += group.size();
      }

      buffer->UnmapBuffer();

      assembly.m_IBuffer = NULL;
      assembly.m_Attribs.clear();
      assembly.AddAttrib(buffer, OGLBaseAlgo::GetPosAttrib(), 3, 5 * sizeof(float), 0);
      assembly.AddAttrib(buffer, OGLBaseAlgo::GetTexCoordAttrib(), 2, 5 * sizeof(float), 3 * sizeof(float));

      Mat4 identMatrix = Identity<Mat4>();

      auto geom = eXl_NEW GeometryInfo;
      geom->m_Assembly = assembly;
      geom->m_Vertices = buffer;
      geom->m_Command = OGLDraw::TriangleList;

      GfxComponent& comp = iGfx.CreateComponent(iObject);
      comp.SetProgram(iGfx.GetSpriteProgram());
      comp.SetGeometry(geom);
      comp.SetTransform(identMatrix);

      for (auto groupEntry : textures)
      {
        TexGroup const& group = groupEntry.first;
        IntrusivePtr<OGLTexture> tex = group.m_Tileset->GetTexture(group.m_Name);
        
        if (numVtx[groupEntry.second] > 0)
        {
          auto mat = eXl_NEW SpriteMaterialInfo;
          mat->m_SpriteInfo.alphaMult = 1.0;
          mat->m_SpriteInfo.tint = One<Vec4>();
          mat->m_SpriteInfo.tcOffset = group.m_VtxOffset;
          mat->m_SpriteInfo.tcScaling = group.m_VtxScaling;
          mat->m_SpriteInfo.texSize = group.m_Tiling;
          mat->m_SpriteInfo.imageSize = MathTools::ToFVec(tex->GetSize());
          mat->m_Texture = tex;
          mat->SetupData();

          comp.AddDraw(mat).NumElements(numVtx[groupEntry.second]).Offset(offset[groupEntry.second]).Layer(iLayer).End();
        }
      }
#endif
    }

    struct MapPattern
    {
      Vec2i m_Size;
      Vec2i m_Position;
      Vector<bool> m_Pattern;

      bool operator == (MapPattern const& iOther) const
      {
        return m_Size == iOther.m_Size && iOther.m_Position == iOther.m_Position
          && m_Pattern == iOther.m_Pattern;
      }
    };

    size_t hash_value(MapPattern const& iPattern)
    {
      size_t hash = iPattern.m_Size.x;
      boost::hash_combine(hash, iPattern.m_Size.y);
      boost::hash_combine(hash, iPattern.m_Position.x);
      boost::hash_combine(hash, iPattern.m_Position.y);
//#ifndef __ANDROID__
//      for (auto word : iPattern.m_Pattern._Myvec)
//      {
//        boost::hash_combine(hash, word);
//      }
//#else
      size_t numCells = iPattern.m_Pattern.size();
      size_t curWritten = 0;
      std::bitset<sizeof(size_t) * 8> curEntry;
      size_t const bitSetSize = 8 * sizeof(size_t);
      for (size_t i = 0; i < numCells; ++i)
      {
        curEntry[i % bitSetSize] = iPattern.m_Pattern[i];
        ++curWritten;
        if (curWritten % bitSetSize == 0)
        {
          curWritten = 0;
          boost::hash_combine(hash, curEntry.to_ullong());
        }
      }
      if (curWritten != 0)
      {
        boost::hash_combine(hash, curEntry.to_ullong());
      }
//#endif
      return hash;
    }

    using PatternsDatabase = UnorderedMap<MapPattern, Vector<Vec2i>>;

    PatternsDatabase CreatePatternDatabase(TilingGroup const& iGroup, AABB2Di const& iMapSize, Vector<AABB2DPolygoni> const& iShapes, Vector<bool> const& iMap)
    {
      Vec2i spaceDims = iMapSize.GetSize();

      PatternsDatabase database;
      UnorderedSet<MapPattern> pointSizes;
      UnorderedSet<MapPattern> xLineSizes;
      UnorderedSet<MapPattern> yLineSizes;
      for (auto& pattern : iGroup.m_Patterns)
      {
        MapPattern newPattern;
        newPattern.m_Size = pattern.second.patternSize;
        newPattern.m_Position = pattern.second.anchor;
        if (newPattern.m_Position.x < 0)
        {
          if (newPattern.m_Position.y >= 0)
          {
            xLineSizes.insert(newPattern);
          }
        }
        else
        {
          if (newPattern.m_Position.y < 0)
          {
            yLineSizes.insert(newPattern);
          }
          else
          {
            pointSizes.insert(newPattern);
          }
        }
      }

      for (auto const& shape : iShapes)
      {
        for (int32_t i = -1; i < (int32_t)shape.Holes().size(); ++i)
        {
          auto const& points = i >= 0 ? shape.Holes()[i] : shape.Border();

          for (uint32_t curIdx = 0; curIdx < points.size(); ++curIdx)
          {
            uint32_t nextIdx = (curIdx + 1) % points.size();

            Vec2i curPt = points[curIdx];
            Vec2i curDir = points[nextIdx] - points[curIdx];
            if (curDir == Zero<Vec2i>())
            {
              continue;
            }

            int32_t dirIdx = curDir.x != 0 ? 0 : 1;
            int32_t dirSign = curDir[dirIdx] > 0 ? 1 : -1;
            uint32_t lineLen = Mathi::Abs(curDir[dirIdx]);

            auto const& linePattern = curDir.x != 0 ? xLineSizes : yLineSizes;
            UnorderedSet<MapPattern> const* sizesToConsider[] = { &pointSizes, &linePattern };
            for (uint32_t dbIdx = 0; dbIdx < 2; dbIdx++)
            {
              auto curSize = sizesToConsider[dbIdx];

              Vec2i startPt = curPt;
              if (dirSign == -1 && dbIdx == 1)
              {
                startPt[dirIdx] -= 1;
              }

              for (auto const& pattern : *curSize)
              {
                Vec2i iterPt = startPt;
                uint32_t numIter = (dbIdx == 0 ? 1 : lineLen);
                if (pattern.m_Position.x < 0)
                {
                  numIter += 2 * (pattern.m_Size.x - 1);
                  iterPt.x -= (pattern.m_Size.x - 1) * dirSign;
                }
                else
                {
                  iterPt.x -= pattern.m_Position.x;
                }
                if (pattern.m_Position.y < 0)
                {
                  numIter += 2 * (pattern.m_Size.y - 1);
                  iterPt.y -= (pattern.m_Size.y - 1) * dirSign;
                }
                else
                {
                  iterPt.y -= pattern.m_Position.y;
                }

                for (uint32_t iter = 0; iter < numIter; ++iter, iterPt[dirIdx] += dirSign)
                {
                  //Vec2i sampleOrigin = iterPt;

                  AABB2Di samplingBox(iterPt, pattern.m_Size);

                  if (samplingBox.m_Data[0].x < iMapSize.m_Data[0].x || samplingBox.m_Data[0].y < iMapSize.m_Data[0].y
                    || samplingBox.m_Data[1].x > iMapSize.m_Data[1].x || samplingBox.m_Data[1].y > iMapSize.m_Data[1].y)
                  {
                    continue;
                  }

                  MapPattern curPattern = pattern;
                  curPattern.m_Pattern.resize(curPattern.m_Size.x * curPattern.m_Size.y);
                  for (int32_t y = 0; y < curPattern.m_Size.y; ++y)
                  {
                    for (int32_t x = 0; x < curPattern.m_Size.x; ++x)
                    {
                      Vec2i pos = Vec2i(x, y) + samplingBox.m_Data[0] - iMapSize.m_Data[0];
                      uint32_t offsetPattern = x + y * curPattern.m_Size.x;
                      uint32_t offsetMap = pos.x + pos.y * spaceDims.x;

                      curPattern.m_Pattern[offsetPattern] = iMap[offsetMap];
                    }
                  }

                  database.insert(std::make_pair(curPattern, Vector<Vec2i>())).first->second.push_back(iterPt);
                }
              }
            }
          }
        }
      }

      return database;
    }

    struct PatternAssign
    {
      Vec2 m_Pos;
      uint32_t m_Pattern;
      uint32_t m_AnchorDist;
      uint32_t m_MatchSize;
      uint32_t m_DrawElement;
    };

    void ComputeGfxForBlock(Batcher& oBatcher, AABB2Di const& iFullSize, Blocks const& iBlock)
    {
      TilingGroup const* group = iBlock.group;
      group->GetTileset().Load();
      Tileset const* mapSet = group->GetTileset().Get();

      Vec2i spaceDims = iFullSize.GetSize();
      uint32_t const items = spaceDims.x * spaceDims.y;
      uint32_t const subItems = items * 4;

      Vector<bool> blockBitmap(items, false);
      TilingGroup::PatternName defaultName("z_Default");
      Vector<TilingGroup::PatternName> orderedNames;
      Vector<TilingPattern const*> patterns;
      UnorderedMap<TilingGroup::PatternName, uint32_t> namesIdx;
      Vector<uint32_t> patternMatchSize;
      orderedNames.push_back(defaultName);
      namesIdx.insert(std::make_pair(defaultName, 0));
      patternMatchSize.push_back(1);

      for (auto const& pattern : group->m_Patterns)
      {
        orderedNames.push_back(pattern.first);
        namesIdx.insert(std::make_pair(pattern.first, orderedNames.size() - 1));
        uint32_t matchSize = std::accumulate(pattern.second.pattern.begin(), pattern.second.pattern.end(), 0,
          [](uint32_t sum, TilingGroupLocConstraint constraint)
        {
          return sum + (constraint != TilingGroupLocConstraint::Undefined ? 1 : 0);
        });
        patternMatchSize.push_back(matchSize);
        patterns.push_back(&pattern.second);
      }

      PatternAssign defaultAssign;
      defaultAssign.m_Pattern = -1;
      defaultAssign.m_MatchSize = 0;
      defaultAssign.m_AnchorDist = 0;
      defaultAssign.m_Pos = Zero<Vec2>();
      defaultAssign.m_DrawElement = 0;

      Vector<PatternAssign> patternAssign(subItems, defaultAssign);

      for (auto const& island : iBlock.islands)
      {
        Vector<AABB2Di> boxes;
        island.GetBoxes(boxes);
        for (auto const& box : boxes)
        {
          for (int y = box.m_Data[0].y; y < box.m_Data[1].y; ++y)
          {
            for (int x = box.m_Data[0].x; x < box.m_Data[1].x; ++x)
            {
              uint32_t offset = (y - iFullSize.m_Data[0].y) * spaceDims.x + x - iFullSize.m_Data[0].x;
              eXl_ASSERT(offset < items);
              blockBitmap[offset] = true;
            }
          }
        }
      }

      PatternsDatabase database = CreatePatternDatabase(*group, iFullSize, iBlock.islands, blockBitmap);
      for (auto const& island : iBlock.islands)
      {
        Vector<AABB2Di> boxes;
        island.GetBoxes(boxes);
        for (auto const& box : boxes)
        {
          for (int y = box.m_Data[0].y * 2; y < box.m_Data[1].y * 2; ++y)
          {
            PatternAssign* row = patternAssign.data() 
              + (y - iFullSize.m_Data[0].y * 2) * (spaceDims.x * 2) 
              + (box.m_Data[0].x - iFullSize.m_Data[0].x) * 2;
            int32_t yParity = (y - iFullSize.m_Data[0].y * 2) % 2;
            for (int x = box.m_Data[0].x * 2;x < box.m_Data[1].x * 2; ++x, ++row)
            {
              int32_t xParity = (x - iFullSize.m_Data[0].x * 2) % 2;
              row->m_Pattern = 0;
              row->m_MatchSize = 1;
              row->m_AnchorDist = xParity + yParity;
              row->m_Pos = Vec2(xParity, yParity) * 0.5;
            }
          }
        }
      }

      //UnorderedMap<TilingGroup::PatternName, Vector<Vec2i>> matchingPos;

      Tile const* defaultTile = mapSet->Find(group->m_DefaultTile);
      Vec2i defaultTileSize = defaultTile->m_Size;

      for (auto const& pattern : group->m_Patterns)
      {
        TilingPattern const& tilingP = pattern.second;
        for (auto const& mapPattern : database)
        {
          MapPattern const& mapP = mapPattern.first;
          if (pattern.second.patternSize == mapP.m_Size
            && pattern.second.anchor == mapP.m_Position)
          {
            bool patternMatches = true;
            for (uint32_t i = 0; i < tilingP.pattern.size() && patternMatches; ++i)
            {
              if (tilingP.pattern[i] != TilingGroupLocConstraint::Undefined)
              {
                bool expectedVal = tilingP.pattern[i] == TilingGroupLocConstraint::SameGroup;
                patternMatches &= expectedVal == mapP.m_Pattern[i];
              }
            }

            if (patternMatches)
            {
              //auto& posArray = matchingPos.insert(std::make_pair(pattern.first, Vector<Vec2i>())).first->second;
              //posArray.insert(posArray.end(), mapPattern.second.begin(), mapPattern.second.end());
              uint32_t patternIdx = namesIdx[pattern.first];
              uint32_t matchSize = patternMatchSize[patternIdx];
              Vec2i anchorSubPos = pattern.second.anchor * 2;
              for (Vec2i position : mapPattern.second)
              {
                Vec2i subpixelPos = position * 2;
                for (uint32_t elemIdx = 0; elemIdx < tilingP.drawElement.size(); ++elemIdx)
                {
                  auto const& element = tilingP.drawElement[elemIdx];
                  Vec2i finalPos = subpixelPos + element.m_Position * 2;

                  Tile const* elemTile = mapSet->Find(element.m_Name);
                  Vec2i scaledSize(elemTile->m_Size.x / defaultTileSize.x,
                    elemTile->m_Size.y / defaultTileSize.y);

                  scaledSize.x = Mathi::Max(scaledSize.x, 1);
                  scaledSize.y = Mathi::Max(scaledSize.y, 1);

                  for (int32_t y = 0; y < scaledSize.y * 2; ++y)
                  {
                    for (int32_t x = 0; x < scaledSize.x * 2; ++x)
                    {
                      Vec2i subTile = element.m_Position * 2 + Vec2i(x, y);
                      Vec2i anchorDiff = subTile - anchorSubPos;
                      if (anchorSubPos.x > subTile.x)
                      {
                        anchorDiff.x += 1;
                      }
                      if (anchorSubPos.y > subTile.y)
                      {
                        anchorDiff.y += 1;
                      }
                      if (pattern.second.anchor.x < 0)
                      {
                        anchorDiff.x = 0;
                      }
                      if (pattern.second.anchor.y < 0)
                      {
                        anchorDiff.y = 0;
                      }

                      uint32_t anchorDist = Mathi::Abs(anchorDiff.x) + Mathi::Abs(anchorDiff.y);
                      uint32_t offset = (x + finalPos.x - iFullSize.m_Data[0].x * 2) 
                        + (y + finalPos.y - iFullSize.m_Data[0].y * 2) * spaceDims.x * 2;
                      Vec2 subPos = Vec2(x, y) * 0.5;
                      PatternAssign& curAssign = patternAssign[offset];
                      if (curAssign.m_MatchSize > matchSize)
                      {
                        continue;
                      }
                      if (curAssign.m_MatchSize == matchSize
                        && curAssign.m_AnchorDist < anchorDist)
                      {
                        continue;
                      }
                      curAssign.m_AnchorDist = anchorDist;
                      curAssign.m_MatchSize = matchSize;
                      curAssign.m_Pattern = patternIdx;
                      curAssign.m_Pos = subPos;
                    }
                  }
                }
              }
            }
          }
        }
      }

      float vtxData[][5] =
      {
        {-0.5, -0.5, 0.0, 0.0, 1.0},
        { 0.5, -0.5, 0.0, 1.0, 1.0},
        {-0.5,  0.5, 0.0, 0.0, 0.0},
        {-0.5,  0.5, 0.0, 0.0, 0.0},
        { 0.5, -0.5, 0.0, 1.0, 1.0},
        { 0.5,  0.5, 0.0, 1.0, 0.0},
      };

      const float worldScaling = defaultTileSize.x / EngineCommon::s_WorldToPixel;

      Vector<std::pair<uint32_t, uint32_t>> completeTiles(spaceDims.x * spaceDims.y, std::make_pair(-1, 0));
      Map<Vec2i, Vector<std::pair<uint32_t, uint32_t>>> bigCompleteTiles;
      for (int32_t y = 0; y < spaceDims.y; ++y)
      {
        PatternAssign* row = patternAssign.data() + y * 2 * (spaceDims.x * 2);
        PatternAssign* upperRow = patternAssign.data() + (y * 2 + 1) * (spaceDims.x * 2);
        for (int32_t x = 0;x < spaceDims.x; ++x, row += 2, upperRow += 2)
        {
          if (row[0].m_Pattern == -1)
          {
            continue;
          }

          TilingPattern const* pattern;
          Vec2i tileSize = One<Vec2i>();
          TilingDrawElement const* element;
          Tile const* elemTile = nullptr;
          if (row[0].m_Pattern != 0)
          {
            pattern = patterns[row[0].m_Pattern - 1];
            element = &pattern->drawElement[row[0].m_DrawElement];
            elemTile = mapSet->Find(element->m_Name);
            tileSize = Vec2i(elemTile->m_Size.x / defaultTileSize.x,
              elemTile->m_Size.y / defaultTileSize.y);

            tileSize.x = Mathi::Max(tileSize.x, 1);
            tileSize.y = Mathi::Max(tileSize.y, 1);
          }

          if (tileSize == One<Vec2i>()
            && row[0].m_Pattern == row[1].m_Pattern
            && row[0].m_Pattern == upperRow[0].m_Pattern
            && row[0].m_Pattern == upperRow[1].m_Pattern
            && row[0].m_Pos == Zero<Vec2>() && row[1].m_Pos == UnitX<Vec2>() * 0.5
            && upperRow[0].m_Pos == UnitY<Vec2>() * 0.5 && upperRow[1].m_Pos == One<Vec2>() * 0.5
            )
          {
            completeTiles[y * spaceDims.x + x] = std::make_pair(row[0].m_Pattern, row[0].m_DrawElement);
            row[0].m_Pattern = row[1].m_Pattern = -1;
            upperRow[0].m_Pattern = upperRow[1].m_Pattern = -1;
            continue;
          }

          if (tileSize != One<Vec2i>())
          {
            // TODO :
            //eXl_ASSERT(false);
            continue;
          }
        }
      }

      for (int32_t y = 0; y < spaceDims.y * 2; ++y)
      {
        PatternAssign* row = patternAssign.data() + y * (spaceDims.x * 2);
        for (int32_t x = 0;x < spaceDims.x * 2; ++x, ++row)
        {
          if (row->m_Pattern == -1)
          {
            continue;
          }

          Tile const* elemTile = nullptr;
          if (row[0].m_Pattern != 0)
          {
            TilingPattern const* pattern = patterns[row[0].m_Pattern - 1];
            TilingDrawElement const* element = &pattern->drawElement[row[0].m_DrawElement];
            elemTile = mapSet->Find(element->m_Name);
          }
          else
          {
            elemTile = defaultTile;
          }

          TexGroup group;
          group.m_Tileset = mapSet;
          group.m_Name = elemTile->m_ImageName;

          Vector<float>& curGroup = oBatcher.GetGroup(group);
          Vec2i imgSize = mapSet->GetImageSize(elemTile->m_ImageName);
          Vec2i tileOffset = elemTile->m_Frames.size() > 0 ? elemTile->m_Frames[0] : Zero<Vec2i>();
          Vec2 tileTCSize(float(elemTile->m_Size.x) / imgSize.x, float(elemTile->m_Size.y) / imgSize.y);
          Vec2 scale(float(defaultTileSize.x) / imgSize.x, float(defaultTileSize.y) / imgSize.y);
          Vec2 offset(float(tileOffset.x) / imgSize.x, tileTCSize.y + float(tileOffset.y) / imgSize.y);

          //Vec2 scaleTC = Vec2(float(defaultTileSize.x) / elemTile->m_Size.x,
          //  float(defaultTileSize.y) / elemTile->m_Size.y);
          //Vec2 scaleTC = One<Vec2>();
          
          Vec2 finalPos(x / 2 + iFullSize.m_Data[0].x, y / 2 + iFullSize.m_Data[0].y) ;
          finalPos += Vec2(row->m_Pos.x - Mathf::Floor(row->m_Pos.x), row->m_Pos.y - Mathf::Floor(row->m_Pos.y));

          for (auto const& vtx : vtxData)
          {
            curGroup.push_back(worldScaling * (0.5 * (0.5 + vtx[0]) + finalPos.x));
            curGroup.push_back(worldScaling * (0.5 * (0.5 + vtx[1]) + finalPos.y));
            curGroup.push_back(vtx[2]);
            curGroup.push_back(offset.x + scale.x * (row->m_Pos.x + 0.5 * vtx[3]));
            //float yPos = 1.0 - (0.5 * scale.y * (1.0 - vtx[4]) + scale.y * row->m_Pos.y);
            //float yPos = (0.5 - row->m_Pos.y) + 0.5 * vtx[4];
            curGroup.push_back(offset.y - scale.y * (row->m_Pos.y + 0.5 * (1.0 - vtx[4])));
          }
        }
      }

      for (uint32_t patternIdx = 0; patternIdx < orderedNames.size(); ++patternIdx)
      {
        uint32_t drawElementIdx = 0;
        TileName tileName = patternIdx == 0 ? group->m_DefaultTile : patterns[patternIdx - 1]->drawElement[drawElementIdx].m_Name;

        Tile const* curTile = mapSet->Find(tileName);

        Vec2i imgSize = mapSet->GetImageSize(curTile->m_ImageName);
        Vec2i tileOffset = curTile->m_Frames.size() > 0 ? curTile->m_Frames[0] : Zero<Vec2i>();
        Vec2 scale(float(curTile->m_Size.x) / imgSize.x, float(curTile->m_Size.y) / imgSize.y);
        Vec2 offset(float(tileOffset.x) / imgSize.x, float(tileOffset.y) / imgSize.y);

        TexGroup group;
        group.m_Tileset = mapSet;
        group.m_Name = curTile->m_ImageName;
        group.m_VtxOffset = offset;
        group.m_VtxScaling = scale;
        group.m_Tiling = scale;

        Vector<float>& curGroup = oBatcher.GetGroup(group);

        Vector<AABB2Di> horizontalBoxes;
          
        for (int32_t y = 0; y < spaceDims.y; ++y)
        {
          std::pair<uint32_t, uint32_t>* row = completeTiles.data() + y * spaceDims.x;
          for (int32_t x = 0;x < spaceDims.x; ++x, ++row)
          {
            if (row->first == patternIdx)
            {
              if (horizontalBoxes.empty()
                || horizontalBoxes.back().m_Data[0].y != y
                || horizontalBoxes.back().m_Data[1].x != x)
              {
                horizontalBoxes.push_back(AABB2Di(Vec2i(x, y), One<Vec2i>()));
              }
              else
              {
                horizontalBoxes.back().m_Data[1].x++;
              }
            }
          }
        }

        std::sort(horizontalBoxes.begin(), horizontalBoxes.end(), [](AABB2Di const& iBox1, AABB2Di const& iBox2)
        {
          if (iBox1.m_Data[0].x == iBox2.m_Data[0].x)
          {
            return iBox1.m_Data[0].y < iBox2.m_Data[0].y;
          }
          return iBox1.m_Data[0].x < iBox2.m_Data[0].x;
        });

        Vector<AABB2Di> boxes;
        for (uint32_t i = 0; i<horizontalBoxes.size(); ++i)
        {
          if (boxes.empty())
          {
            boxes.push_back(horizontalBoxes[i]);
          }
          else
          {
            AABB2Di& prevBox = boxes.back();
            AABB2Di const& curBox = horizontalBoxes[i];
            if (prevBox.m_Data[0].x == curBox.m_Data[0].x
              && prevBox.m_Data[1].x == curBox.m_Data[1].x
              && prevBox.m_Data[1].y == curBox.m_Data[0].y)
            {
              prevBox.m_Data[1].y++;
            }
            else
            {
              boxes.push_back(curBox);
            }
          }
        }

        for (auto const& box : boxes)
        {
          Vec2i finalPos = box.m_Data[0] + iFullSize.m_Data[0];
          Vec2i size = box.GetSize();
        
          for (auto const& vtx : vtxData)
          {
            curGroup.push_back(worldScaling * ((0.5 + vtx[0]) * size.x + finalPos.x));
            curGroup.push_back(worldScaling * ((0.5 + vtx[1]) * size.y + finalPos.y));
            curGroup.push_back(vtx[2]);
            curGroup.push_back(vtx[3] * size.x);
            curGroup.push_back(vtx[4] * size.y);
          }
        }
      }
    }
  }
}