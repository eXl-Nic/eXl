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
      boost::hash_combine(value, iGroup.m_VtxScaling.X());
      boost::hash_combine(value, iGroup.m_VtxScaling.Y());
      boost::hash_combine(value, iGroup.m_VtxOffset.X());
      boost::hash_combine(value, iGroup.m_VtxOffset.Y());
      boost::hash_combine(value, iGroup.m_Tiling.X());
      boost::hash_combine(value, iGroup.m_Tiling.Y());
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

      Matrix4f identMatrix;
      identMatrix.MakeIdentity();

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
          mat->m_SpriteInfo.tint = Vector4f::ONE;
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
      Vector2i m_Size;
      Vector2i m_Position;
      Vector<bool> m_Pattern;

      bool operator == (MapPattern const& iOther) const
      {
        return m_Size == iOther.m_Size && iOther.m_Position == iOther.m_Position
          && m_Pattern == iOther.m_Pattern;
      }
    };

    size_t hash_value(MapPattern const& iPattern)
    {
      size_t hash = iPattern.m_Size.X();
      boost::hash_combine(hash, iPattern.m_Size.Y());
      boost::hash_combine(hash, iPattern.m_Position.X());
      boost::hash_combine(hash, iPattern.m_Position.Y());
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

    using PatternsDatabase = UnorderedMap<MapPattern, Vector<Vector2i>>;

    PatternsDatabase CreatePatternDatabase(TilingGroup const& iGroup, AABB2Di const& iMapSize, Vector<AABB2DPolygoni> const& iShapes, Vector<bool> const& iMap)
    {
      Vector2i spaceDims = iMapSize.GetSize();

      PatternsDatabase database;
      UnorderedSet<MapPattern> pointSizes;
      UnorderedSet<MapPattern> xLineSizes;
      UnorderedSet<MapPattern> yLineSizes;
      for (auto& pattern : iGroup.m_Patterns)
      {
        MapPattern newPattern;
        newPattern.m_Size = pattern.second.patternSize;
        newPattern.m_Position = pattern.second.anchor;
        if (newPattern.m_Position.X() < 0)
        {
          if (newPattern.m_Position.Y() >= 0)
          {
            xLineSizes.insert(newPattern);
          }
        }
        else
        {
          if (newPattern.m_Position.Y() < 0)
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

            Vector2i curPt = points[curIdx];
            Vector2i curDir = points[nextIdx] - points[curIdx];
            if (curDir == Vector2i::ZERO)
            {
              continue;
            }

            int32_t dirIdx = curDir.X() != 0 ? 0 : 1;
            int32_t dirSign = curDir.m_Data[dirIdx] > 0 ? 1 : -1;
            uint32_t lineLen = Mathi::Abs(curDir.m_Data[dirIdx]);

            auto const& linePattern = curDir.X() != 0 ? xLineSizes : yLineSizes;
            UnorderedSet<MapPattern> const* sizesToConsider[] = { &pointSizes, &linePattern };
            for (uint32_t dbIdx = 0; dbIdx < 2; dbIdx++)
            {
              auto curSize = sizesToConsider[dbIdx];

              Vector2i startPt = curPt;
              if (dirSign == -1 && dbIdx == 1)
              {
                startPt.m_Data[dirIdx] -= 1;
              }

              for (auto const& pattern : *curSize)
              {
                Vector2i iterPt = startPt;
                uint32_t numIter = (dbIdx == 0 ? 1 : lineLen);
                if (pattern.m_Position.X() < 0)
                {
                  numIter += 2 * (pattern.m_Size.X() - 1);
                  iterPt.X() -= (pattern.m_Size.X() - 1) * dirSign;
                }
                else
                {
                  iterPt.X() -= pattern.m_Position.X();
                }
                if (pattern.m_Position.Y() < 0)
                {
                  numIter += 2 * (pattern.m_Size.Y() - 1);
                  iterPt.Y() -= (pattern.m_Size.Y() - 1) * dirSign;
                }
                else
                {
                  iterPt.Y() -= pattern.m_Position.Y();
                }

                for (uint32_t iter = 0; iter < numIter; ++iter, iterPt.m_Data[dirIdx] += dirSign)
                {
                  //Vector2i sampleOrigin = iterPt;

                  AABB2Di samplingBox(iterPt, pattern.m_Size);

                  if (samplingBox.m_Data[0].X() < iMapSize.m_Data[0].X() || samplingBox.m_Data[0].Y() < iMapSize.m_Data[0].Y()
                    || samplingBox.m_Data[1].X() > iMapSize.m_Data[1].X() || samplingBox.m_Data[1].Y() > iMapSize.m_Data[1].Y())
                  {
                    continue;
                  }

                  MapPattern curPattern = pattern;
                  curPattern.m_Pattern.resize(curPattern.m_Size.X() * curPattern.m_Size.Y());
                  for (int32_t y = 0; y < curPattern.m_Size.Y(); ++y)
                  {
                    for (int32_t x = 0; x < curPattern.m_Size.X(); ++x)
                    {
                      Vector2i pos = (Vector2i(x, y) + samplingBox.m_Data[0]) - iMapSize.m_Data[0];
                      uint32_t offsetPattern = x + y * curPattern.m_Size.X();
                      uint32_t offsetMap = pos.X() + pos.Y() * spaceDims.X();

                      curPattern.m_Pattern[offsetPattern] = iMap[offsetMap];
                    }
                  }

                  database.insert(std::make_pair(curPattern, Vector<Vector2i>())).first->second.push_back(iterPt);
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
      Vector2f m_Pos;
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

      Vector2i spaceDims = iFullSize.GetSize();
      uint32_t const items = spaceDims.X() * spaceDims.Y();
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
      defaultAssign.m_Pos = Vector2f::ZERO;
      defaultAssign.m_DrawElement = 0;

      Vector<PatternAssign> patternAssign(subItems, defaultAssign);

      for (auto const& island : iBlock.islands)
      {
        Vector<AABB2Di> boxes;
        island.GetBoxes(boxes);
        for (auto const& box : boxes)
        {
          for (int y = box.m_Data[0].Y(); y < box.m_Data[1].Y(); ++y)
          {
            for (int x = box.m_Data[0].X(); x < box.m_Data[1].X(); ++x)
            {
              uint32_t offset = (y - iFullSize.m_Data[0].Y()) * spaceDims.X() + (x - iFullSize.m_Data[0].X());
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
          for (int y = box.m_Data[0].Y() * 2; y < box.m_Data[1].Y() * 2; ++y)
          {
            PatternAssign* row = patternAssign.data() 
              + (y - iFullSize.m_Data[0].Y() * 2) * (spaceDims.X() * 2) 
              + (box.m_Data[0].X() - iFullSize.m_Data[0].X()) * 2;
            int32_t yParity = (y - iFullSize.m_Data[0].Y() * 2) % 2;
            for (int x = box.m_Data[0].X() * 2; x < box.m_Data[1].X() * 2; ++x, ++row)
            {
              int32_t xParity = (x - iFullSize.m_Data[0].X() * 2) % 2;
              row->m_Pattern = 0;
              row->m_MatchSize = 1;
              row->m_AnchorDist = xParity + yParity;
              row->m_Pos = Vector2f(xParity, yParity) * 0.5;
            }
          }
        }
      }

      //UnorderedMap<TilingGroup::PatternName, Vector<Vector2i>> matchingPos;

      Tile const* defaultTile = mapSet->Find(group->m_DefaultTile);
      Vector2i defaultTileSize = defaultTile->m_Size;

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
              //auto& posArray = matchingPos.insert(std::make_pair(pattern.first, Vector<Vector2i>())).first->second;
              //posArray.insert(posArray.end(), mapPattern.second.begin(), mapPattern.second.end());
              uint32_t patternIdx = namesIdx[pattern.first];
              uint32_t matchSize = patternMatchSize[patternIdx];
              Vector2i anchorSubPos = pattern.second.anchor * 2;
              for (Vector2i position : mapPattern.second)
              {
                Vector2i subpixelPos = position * 2;
                for (uint32_t elemIdx = 0; elemIdx < tilingP.drawElement.size(); ++elemIdx)
                {
                  auto const& element = tilingP.drawElement[elemIdx];
                  Vector2i finalPos = subpixelPos + element.m_Position * 2;

                  Tile const* elemTile = mapSet->Find(element.m_Name);
                  Vector2i scaledSize(elemTile->m_Size.X() / defaultTileSize.X(),
                    elemTile->m_Size.Y() / defaultTileSize.Y());

                  scaledSize.X() = Mathi::Max(scaledSize.X(), 1);
                  scaledSize.Y() = Mathi::Max(scaledSize.Y(), 1);

                  for (int32_t y = 0; y < scaledSize.Y() * 2; ++y)
                  {
                    for (int32_t x = 0; x < scaledSize.X() * 2; ++x)
                    {
                      Vector2i subTile = element.m_Position * 2 + Vector2i(x, y);
                      Vector2i anchorDiff = subTile - anchorSubPos;
                      if (anchorSubPos.X() > subTile.X())
                      {
                        anchorDiff.X() += 1;
                      }
                      if (anchorSubPos.Y() > subTile.Y())
                      {
                        anchorDiff.Y() += 1;
                      }
                      if (pattern.second.anchor.X() < 0)
                      {
                        anchorDiff.X() = 0;
                      }
                      if (pattern.second.anchor.Y() < 0)
                      {
                        anchorDiff.Y() = 0;
                      }

                      uint32_t anchorDist = Mathi::Abs(anchorDiff.X()) + Mathi::Abs(anchorDiff.Y());
                      uint32_t offset = (x + finalPos.X() - iFullSize.m_Data[0].X() * 2) 
                        + (y + finalPos.Y() - iFullSize.m_Data[0].Y() * 2) * spaceDims.X() * 2;
                      Vector2f subPos = Vector2f(x, y) * 0.5;
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

      const float worldScaling = defaultTileSize.X() / EngineCommon::s_WorldToPixel;

      Vector<std::pair<uint32_t, uint32_t>> completeTiles(spaceDims.X() * spaceDims.Y(), std::make_pair(-1, 0));
      Map<Vector2i, Vector<std::pair<uint32_t, uint32_t>>> bigCompleteTiles;
      for (int32_t y = 0; y < spaceDims.Y(); ++y)
      {
        PatternAssign* row = patternAssign.data() + y * 2 * (spaceDims.X() * 2);
        PatternAssign* upperRow = patternAssign.data() + (y * 2 + 1) * (spaceDims.X() * 2);
        for (int32_t x = 0; x < spaceDims.X(); ++x, row += 2, upperRow += 2)
        {
          if (row[0].m_Pattern == -1)
          {
            continue;
          }

          TilingPattern const* pattern;
          Vector2i tileSize = Vector2i::ONE;
          TilingDrawElement const* element;
          Tile const* elemTile = nullptr;
          if (row[0].m_Pattern != 0)
          {
            pattern = patterns[row[0].m_Pattern - 1];
            element = &pattern->drawElement[row[0].m_DrawElement];
            elemTile = mapSet->Find(element->m_Name);
            tileSize = Vector2i(elemTile->m_Size.X() / defaultTileSize.X(),
              elemTile->m_Size.Y() / defaultTileSize.Y());

            tileSize.X() = Mathi::Max(tileSize.X(), 1);
            tileSize.Y() = Mathi::Max(tileSize.Y(), 1);
          }

          if (tileSize == Vector2i::ONE
            && row[0].m_Pattern == row[1].m_Pattern
            && row[0].m_Pattern == upperRow[0].m_Pattern
            && row[0].m_Pattern == upperRow[1].m_Pattern
            && row[0].m_Pos == Vector2f::ZERO && row[1].m_Pos == UnitX<Vector2f>() * 0.5
            && upperRow[0].m_Pos == UnitY<Vector2f>() * 0.5 && upperRow[1].m_Pos == Vector2f::ONE * 0.5
            )
          {
            completeTiles[y * spaceDims.X() + x] = std::make_pair(row[0].m_Pattern, row[0].m_DrawElement);
            row[0].m_Pattern = row[1].m_Pattern = -1;
            upperRow[0].m_Pattern = upperRow[1].m_Pattern = -1;
            continue;
          }

          if (tileSize != Vector2i::ONE)
          {
            // TODO :
            //eXl_ASSERT(false);
            continue;
          }
        }
      }

      for (int32_t y = 0; y < spaceDims.Y() * 2; ++y)
      {
        PatternAssign* row = patternAssign.data() + y * (spaceDims.X() * 2);
        for (int32_t x = 0; x < spaceDims.X() * 2; ++x, ++row)
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
          Vector2i imgSize = mapSet->GetImageSize(elemTile->m_ImageName);
          Vector2i tileOffset = elemTile->m_Frames.size() > 0 ? elemTile->m_Frames[0] : Vector2i::ZERO;
          Vector2f tileTCSize(float(elemTile->m_Size.X()) / imgSize.X(), float(elemTile->m_Size.Y()) / imgSize.Y());
          Vector2f scale(float(defaultTileSize.X()) / imgSize.X(), float(defaultTileSize.Y()) / imgSize.Y());
          Vector2f offset(float(tileOffset.X()) / imgSize.X(), tileTCSize.Y() + float(tileOffset.Y()) / imgSize.Y());

          //Vector2f scaleTC = Vector2f(float(defaultTileSize.X()) / elemTile->m_Size.X(),
          //  float(defaultTileSize.Y()) / elemTile->m_Size.Y());
          //Vector2f scaleTC = Vector2f::ONE;
          
          Vector2f finalPos(x / 2 + iFullSize.m_Data[0].X(), y / 2 + iFullSize.m_Data[0].Y()) ;
          finalPos += Vector2f(row->m_Pos.X() - Mathf::Floor(row->m_Pos.X()), row->m_Pos.Y() - Mathf::Floor(row->m_Pos.Y()));

          for (auto const& vtx : vtxData)
          {
            curGroup.push_back(worldScaling * (0.5 * (0.5 + vtx[0]) + finalPos.X()));
            curGroup.push_back(worldScaling * (0.5 * (0.5 + vtx[1]) + finalPos.Y()));
            curGroup.push_back(vtx[2]);
            curGroup.push_back(offset.X() + scale.X() * (row->m_Pos.X() + 0.5 * vtx[3]));
            //float yPos = 1.0 - (0.5 * scale.Y() * (1.0 - vtx[4]) + scale.Y() * row->m_Pos.Y());
            //float yPos = (0.5 - row->m_Pos.Y()) + 0.5 * vtx[4];
            curGroup.push_back(offset.Y() - scale.Y() * (row->m_Pos.Y() + 0.5 * (1.0 - vtx[4])));
          }
        }
      }

      for (uint32_t patternIdx = 0; patternIdx < orderedNames.size(); ++patternIdx)
      {
        uint32_t drawElementIdx = 0;
        TileName tileName = patternIdx == 0 ? group->m_DefaultTile : patterns[patternIdx - 1]->drawElement[drawElementIdx].m_Name;

        Tile const* curTile = mapSet->Find(tileName);

        Vector2i imgSize = mapSet->GetImageSize(curTile->m_ImageName);
        Vector2i tileOffset = curTile->m_Frames.size() > 0 ? curTile->m_Frames[0] : Vector2i::ZERO;
        Vector2f scale(float(curTile->m_Size.X()) / imgSize.X(), float(curTile->m_Size.Y()) / imgSize.Y());
        Vector2f offset(float(tileOffset.X()) / imgSize.X(), float(tileOffset.Y()) / imgSize.Y());

        TexGroup group;
        group.m_Tileset = mapSet;
        group.m_Name = curTile->m_ImageName;
        group.m_VtxOffset = offset;
        group.m_VtxScaling = scale;
        group.m_Tiling = scale;

        Vector<float>& curGroup = oBatcher.GetGroup(group);

        Vector<AABB2Di> horizontalBoxes;
          
        for (int32_t y = 0; y < spaceDims.Y(); ++y)
        {
          std::pair<uint32_t, uint32_t>* row = completeTiles.data() + y * spaceDims.X();
          for (int32_t x = 0; x < spaceDims.X(); ++x, ++row)
          {
            if (row->first == patternIdx)
            {
              if (horizontalBoxes.empty()
                || horizontalBoxes.back().m_Data[0].Y() != y
                || horizontalBoxes.back().m_Data[1].X() != x)
              {
                horizontalBoxes.push_back(AABB2Di(Vector2i(x, y), Vector2i::ONE));
              }
              else
              {
                horizontalBoxes.back().m_Data[1].X()++;
              }
            }
          }
        }

        std::sort(horizontalBoxes.begin(), horizontalBoxes.end(), [](AABB2Di const& iBox1, AABB2Di const& iBox2)
        {
          if (iBox1.m_Data[0].X() == iBox2.m_Data[0].X())
          {
            return iBox1.m_Data[0].Y() < iBox2.m_Data[0].Y();
          }
          return iBox1.m_Data[0].X() < iBox2.m_Data[0].X();
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
            if (prevBox.m_Data[0].X() == curBox.m_Data[0].X()
              && prevBox.m_Data[1].X() == curBox.m_Data[1].X()
              && prevBox.m_Data[1].Y() == curBox.m_Data[0].Y())
            {
              prevBox.m_Data[1].Y()++;
            }
            else
            {
              boxes.push_back(curBox);
            }
          }
        }

        for (auto const& box : boxes)
        {
          Vector2i finalPos = box.m_Data[0] + iFullSize.m_Data[0];
          Vector2i size = box.GetSize();
        
          for (auto const& vtx : vtxData)
          {
            curGroup.push_back(worldScaling * ((0.5 + vtx[0]) * size.X() + finalPos.X()));
            curGroup.push_back(worldScaling * ((0.5 + vtx[1]) * size.Y() + finalPos.Y()));
            curGroup.push_back(vtx[2]);
            curGroup.push_back(vtx[3] * size.X());
            curGroup.push_back(vtx[4] * size.Y());
          }
        }
      }
    }
  }
}