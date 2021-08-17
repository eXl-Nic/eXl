#include <gen/floodfill.hpp>
//#include <gen/multigrid.hpp>
#include <math/segment.hpp>
#include <core/coredef.hpp>

//#include <gametk/image.hpp>
//#include <gametk/imagestreamer.hpp>

namespace eXl
{

  namespace
  {
    bool CheckCoord(Vector2i const& iPos, AABB2Di const& iBox)
    {
      return (iPos.X() >= iBox.m_Data[0].X() && iPos.X() < iBox.m_Data[1].X())
        && (iPos.Y() >= iBox.m_Data[0].Y() && iPos.Y() < iBox.m_Data[1].Y());
    }

    unsigned int GetOffset(Vector2i const& iPos, AABB2Di const& iBox)
    {
      return iPos.X() - iBox.m_Data[0].X() + (iPos.Y() - iBox.m_Data[0].Y())*(iBox.m_Data[1].X() - iBox.m_Data[0].X());
    }

#if 0
    void DumpGrid(Vector<char> const& iGrid, AABB2Di const& iBox, Vector<Vector2i> const& iBorders, Vector2i const& iCurPos, Vector2i const& iNeigh)
    {
      Vector2i size = iBox.GetSize();
      Vector<char> image(size.X() * size.Y() * 3);
      if (!image.empty())
      {
        for (unsigned int i = 0; i < iGrid.size(); ++i)
        {
          char value = iGrid[i];
          char* dest = &image[0] + 3 * i;
          switch (value)
          {
          case 0:
            dest[0] = 255; dest[1] = 255; dest[2] = 255;
            break;
          case -1:
          case -2:
            dest[0] = 0; dest[1] = 0; dest[2] = 0;
            break;
          case -3:
            dest[0] = 0; dest[1] = 0; dest[2] = 255;
            break;
          case -4:
            dest[0] = 192; dest[1] = 64; dest[2] = 0;
            break;
          default:
            dest[0] = 128; dest[1] = 128; dest[2] = 128;
            break;
          };
        }
        if (CheckCoord(iCurPos, iBox))
        {
          unsigned int offsetCur = GetOffset(iCurPos, iBox);
          image[3 * offsetCur + 0] = 255;
          image[3 * offsetCur + 1] = 0;
          image[3 * offsetCur + 2] = 0;
        }
        if (CheckCoord(iNeigh, iBox))
        {
          unsigned int offsetCur = GetOffset(iNeigh, iBox);
          image[3 * offsetCur + 0] = 0;
          image[3 * offsetCur + 1] = 255;
          image[3 * offsetCur + 2] = 0;
        }
        //if(!iBorders.empty())
        //{
        //  Vector2i curPt = iBorders[0];
        //  for(unsigned int i = 1; i<iBorders.size(); ++i)
        //  {
        //    Vector2i nextPt = iBorders[i];
        //    Vector2i diff = nextPt - curPt;
        //    unsigned int dist = Mathi::Max(Mathi::Abs(diff.X()), Mathi::Abs(diff.Y()));
        //    diff = diff / dist;
        //    for(unsigned int j = 0; j<dist; ++j)
        //    {
        //      Vector2i pt = (curPt + diff * j) / 2;
        //
        //    }
        //  }
        //}
        static int numSave = 0;
        Image* newImg = new Image(&image[0], size, Image::RGB, Image::Char, 1);
        void* oData = nullptr;
        size_t compressedSize;
        ImageStreamer::Get().Save(newImg, ImageStreamer::Png, compressedSize, oData);
        if (oData)
        {
          char fileName[256];
          sprintf(fileName, "D:\\img_%i_dbg.png", numSave);
          FILE* oFile = fopen(fileName, "wb");
          if (oFile)
          {
            fwrite(oData, 1, compressedSize, oFile);
            fclose(oFile);
          }
          free(oData);
        }
        ++numSave;
      }
    }
#endif
  }
  uint32_t GatherNeighbours(Vector2i const& iPos, Vector<char> const& iMap, AABB2Di const& iBox
    , Vector<uint32_t>& oCompMap, char const*(&oNeigh)[4], uint32_t* (&oNeighLabel)[4])
  {
    uint32_t neighMask = 0;
    if (CheckCoord(iPos - Vector2i::UNIT_X, iBox))
    {
      uint32_t const offset = GetOffset(iPos - Vector2i::UNIT_X, iBox);
      neighMask |= 1;
      oNeigh[0] = iMap.data() + offset;
      oNeighLabel[0] = oCompMap.data() + offset;
    }
    if (CheckCoord(iPos + Vector2i::UNIT_X, iBox))
    {
      uint32_t const offset = GetOffset(iPos + Vector2i::UNIT_X, iBox);
      neighMask |= 2;
      oNeigh[1] = iMap.data() + offset;
      oNeighLabel[1] = oCompMap.data() + offset;
    }
    if (CheckCoord(iPos - Vector2i::UNIT_Y, iBox))
    {
      uint32_t const offset = GetOffset(iPos - Vector2i::UNIT_Y, iBox);
      neighMask |= 4;
      oNeigh[2] = iMap.data() + offset;
      oNeighLabel[2] = oCompMap.data() + offset;
    }
    if (CheckCoord(iPos + Vector2i::UNIT_Y, iBox))
    {
      uint32_t const offset = GetOffset(iPos + Vector2i::UNIT_Y, iBox);
      neighMask |= 8;
      oNeigh[3] = iMap.data() + offset;
      oNeighLabel[3] = oCompMap.data() + offset;
    }

    return neighMask;
  }

  uint32_t FloodFill::ExtractComponents(Vector<char> const& iMap, AABB2Di const& iBox, Vector<uint32_t>& oCompMap)
  {
    oCompMap.resize(iMap.size(), Out_Tag);
    Vector<std::unique_ptr<Set<uint32_t>>> setStorage;
    Vector<Set<uint32_t>*> equivalenceSet;

    //Vector2i size = iBox.GetSize();

    uint32_t maxSet = 0;

    for (int32_t y = iBox.m_Data[0].Y(); y < iBox.m_Data[1].Y(); ++y)
    {
      for (int32_t x = iBox.m_Data[0].X(); x < iBox.m_Data[1].X(); ++x)
      {
        Vector2i curPt(x, y);
        uint32_t offset = GetOffset(curPt, iBox);
        if (iMap[offset] == In_Tag)
        {
          char const* neigh[4] = {0};
          uint32_t* neighLabel[4] = {0};
          uint32_t const neighMask = GatherNeighbours(curPt, iMap, iBox, oCompMap, neigh, neighLabel);

          uint32_t minSetLabel = oCompMap[offset];
          for (uint32_t i = 0; i < 2; ++i)
          {
            uint32_t predecessor = i * 2;
            if (neighMask & 1 << predecessor
              && *neigh[predecessor] == In_Tag
              && *neighLabel[predecessor] != Out_Tag)
            {
              minSetLabel = Mathi::Min(*neighLabel[predecessor], minSetLabel);
            }
          }

          if (minSetLabel == Out_Tag)
          {
            minSetLabel = maxSet++;
            setStorage.push_back(std::make_unique<Set<uint32_t>>());
            setStorage.back()->insert(minSetLabel);
            equivalenceSet.push_back(setStorage.back().get());
          }

          oCompMap[offset] = minSetLabel;
          for (uint32_t i = 0; i < 4; ++i)
          {
            if (neighMask & 1 << i
              && *neigh[i] == In_Tag)
            {
              if (*neighLabel[i] != Out_Tag
                && equivalenceSet[*neighLabel[i]] != equivalenceSet[minSetLabel])
              {
                equivalenceSet[minSetLabel]->insert(equivalenceSet[*neighLabel[i]]->begin(), equivalenceSet[*neighLabel[i]]->end());
                equivalenceSet[*neighLabel[i]] = equivalenceSet[minSetLabel];
              }
              *neighLabel[i] = minSetLabel;
            }
          }
        }
      }
    }

    if (maxSet == 0)
    {
      return 0;
    }

    uint32_t numLabels = 0;
    Map<uint32_t, uint32_t> uniqueLabels;
    Vector<uint32_t> repLabel;
    repLabel.reserve(maxSet);
    for (auto const& equivClass : equivalenceSet)
    {
      auto insertRes = uniqueLabels.insert(std::make_pair(*equivClass->begin(), numLabels));
      if (insertRes.second)
      {
        ++numLabels;
      }
      repLabel.push_back(insertRes.first->second);
    }
    
    for (int32_t y = iBox.m_Data[0].Y(); y < iBox.m_Data[1].Y(); ++y)
    {
      for (int32_t x = iBox.m_Data[0].X(); x < iBox.m_Data[1].X(); ++x)
      {
        Vector2i curPt(x, y);
        uint32_t offset = GetOffset(curPt, iBox);
        if (iMap[offset] == In_Tag)
        {
          oCompMap[offset] = repLabel[oCompMap[offset]];
        }
      }
    }

    return uniqueLabels.size();
  }

  bool FloodFill::ExamineNeigh(Vector<Vector2i>& ioList, Vector<char>& iGrid, Vector2i const& iPos, AABB2Di const& iBox)
  {
    if (CheckCoord(iPos, iBox))
    {
      if (iGrid[GetOffset(iPos, iBox)] == Out_Tag)
      {
        iGrid[GetOffset(iPos, iBox)] = OutVisited_Tag;
        ioList.push_back(iPos);
      }
      else if (iGrid[GetOffset(iPos, iBox)] == In_Tag)
      {
        return true;
      }
    }
    return false;
  }

  void FloodFill::MakePolygon(Vector<char>& iExtendedBitmap, AABB2Di const& iExtendedBox, AABB2DPolygoni& oPoly)
  {
    Vector<Vector2i> outBorder;
    Fill(iExtendedBox.m_Data[0], iExtendedBitmap, outBorder, true, iExtendedBox);

    oPoly = AABB2DPolygoni(outBorder);

    Vector<Vector<Vector2i> > holes;

    Vector2i dim = iExtendedBox.GetSize();

    for (int i = 0; i < dim.Y() - 2; ++i)
    {
      unsigned int offsetDest = 1 + (i + 1) * dim.X();
      for (int j = 0; j < dim.X() - 2; ++j)
      {
        if (iExtendedBitmap[offsetDest] == Out_Tag)
        {
          holes.push_back(Vector<Vector2i>());
          Fill(Vector2i(j + 1, i + 1) + iExtendedBox.m_Data[0], iExtendedBitmap, holes.back(), false, iExtendedBox);
        }
        ++offsetDest;
      }
    }
    oPoly.Holes() = holes;

    oPoly.Translate(Vector2i::ONE);
    oPoly.Scale(1, 2);

    oPoly.RemoveUselessPoints();
  }

  void FloodFill::Fill(Vector2i const& iStartPt, Vector<char>& iGrid , Vector<Vector2i>& oBorder, bool iExt, AABB2Di const& iBox)
  {
    Vector<Vector2i> ioList;
    ioList.push_back(iStartPt);

    Vector2i origPos = iStartPt;
    Vector2i borderStart(iBox.m_Data[1]);

    while(!ioList.empty())
    {
      Vector2i curPt = ioList.back();
      ioList.pop_back();
      iGrid[GetOffset(curPt, iBox)] = Corner_Tag;

      if(ExamineNeigh(ioList, iGrid, curPt + Vector2i::UNIT_X, iBox))
      {
        borderStart = curPt < borderStart ? curPt : borderStart;
        iGrid[GetOffset(curPt, iBox)] = Border_Tag;
      }
      if(ExamineNeigh(ioList, iGrid, curPt - Vector2i::UNIT_X, iBox))
      {
        borderStart = curPt < borderStart ? curPt : borderStart;
        iGrid[GetOffset(curPt, iBox)] = Border_Tag;
      }
      if(ExamineNeigh(ioList, iGrid, curPt + Vector2i::UNIT_Y, iBox))
      {
        borderStart = curPt < borderStart ? curPt : borderStart;
        iGrid[GetOffset(curPt, iBox)] = Border_Tag;
      }
      if(ExamineNeigh(ioList, iGrid, curPt - Vector2i::UNIT_Y, iBox))
      {
        borderStart = curPt < borderStart ? curPt : borderStart;
        iGrid[GetOffset(curPt, iBox)] = Border_Tag;
      }
    }


    int nextDir[2][4] = {{2,3,1,0}, {3,2,0,1}};

    Vector2i curBorderPos = borderStart;
    int startBorderDir = 4;
    for(int dir = 0; dir<4; ++dir)
    {
      Vector2i expectedFilledPos;
      int expectedDir = nextDir[iExt][dir];
      expectedFilledPos.m_Data[expectedDir / 2] = 2*(expectedDir%2) - 1;
      expectedFilledPos += borderStart;

      if(CheckCoord(expectedFilledPos, iBox)
        && iGrid[GetOffset(expectedFilledPos, iBox)] == In_Tag)
      {
        startBorderDir = dir;
        break;
      }
    }

    if (startBorderDir == 4)
    {
      return;
    }

    Vector2i expectedFilledPos;
    int expectedDir = nextDir[iExt][startBorderDir];
    expectedFilledPos.m_Data[expectedDir / 2] = 2*(expectedDir%2) - 1;
    expectedFilledPos.m_Data[startBorderDir / 2] = 1 - 2*(startBorderDir%2);
    unsigned int curBorderDir = startBorderDir;
    oBorder.push_back(curBorderPos * 2 + expectedFilledPos);

    do
    {
      Vector2i neigh;
      neigh.m_Data[curBorderDir / 2] = 2*(curBorderDir%2) - 1;
      neigh += curBorderPos;

      Vector2i neighExpected;
      neighExpected.m_Data[expectedDir / 2] = 2*(expectedDir%2) - 1;
      neighExpected += neigh;

      if(CheckCoord(neigh, iBox)
        && CheckCoord(neighExpected, iBox))
      {
        char value = iGrid[GetOffset(neigh, iBox)];
        char valueExpected = iGrid[GetOffset(neighExpected, iBox)];
        if (value == Border_Tag && valueExpected == Border_Tag)
        {
          value = Corner_Tag;
        }

        switch(value)
        {
        case In_Tag:
          //internal corner
        {
          oBorder.push_back(neigh * 2 + expectedFilledPos);
          curBorderDir = nextDir[!iExt][curBorderDir];
          expectedFilledPos = Vector2i::ZERO;
          expectedDir = nextDir[iExt][curBorderDir];
          expectedFilledPos.m_Data[expectedDir / 2] = 2*(expectedDir%2) - 1;
          expectedFilledPos.m_Data[curBorderDir / 2] = 1 - 2*(curBorderDir%2);
        }
        break;
        case Out_Tag:
        case OutVisited_Tag:
          //Should not happen
          //DumpGrid(iGrid, iBox, oBorder, curBorderPos, neigh);
          eXl_ASSERT_MSG(false, "BadFloodFill");
          return;
          break;
        case Corner_Tag:
          //external corner
        {
          oBorder.push_back(neigh * 2 + expectedFilledPos);
          curBorderDir = nextDir[iExt][curBorderDir];
          neigh.m_Data[curBorderDir / 2] += 2*(curBorderDir%2) - 1;
          bool cond = CheckCoord(neigh, iBox) && iGrid[GetOffset(neigh, iBox)] == -4;
          eXl_ASSERT_MSG(cond, "Bad");
          if(!cond)
          {
            //DumpGrid(iGrid, iBox, oBorder, curBorderPos, neigh);
            return;
          }
          curBorderPos = neigh;
          expectedFilledPos = Vector2i::ZERO;
          expectedDir = nextDir[iExt][curBorderDir];
          expectedFilledPos.m_Data[expectedDir / 2] = 2*(expectedDir%2) - 1;
          expectedFilledPos.m_Data[curBorderDir / 2] = 1 - 2*(curBorderDir%2);
        }
        break;
        case Border_Tag:
          //Continue
          curBorderPos = neigh;
          break;
        };
      }
      else
      {
        //DumpGrid(iGrid, iBox, oBorder, curBorderPos, neigh);
        eXl_ASSERT_MSG(false, "BadPath");
        break;
      }
    }
    while(curBorderPos != borderStart || curBorderDir != startBorderDir);
  }

  unsigned int FloodFill::MakeGradientMap(AABB2DPolygoni const& iPoly, Vector<unsigned int>& oValues, Vector<Vector2i>& oGradient)
  {
    Vector<AABB2DPolygoni> currentPoly;
    Vector<AABB2DPolygoni> nextPoly;
    currentPoly.push_back(iPoly);

    AABB2Di box = iPoly.GetAABB();
    Vector2i dim = box.GetSize();
    oValues.resize(dim.X() * dim.Y(), 0);
    oGradient.resize(dim.X() * dim.Y(), Vector2i::ZERO);

    unsigned int currentValue = 1;
    while(!currentPoly.empty())
    {
      nextPoly.clear();
      for(auto const& poly : currentPoly)
      {
        Vector<AABB2DPolygoni> shrinked;
        poly.Shrink(1, shrinked);
        Vector<AABB2DPolygoni> remains;

        remains.emplace_back(std::move(poly));
        for(auto& poly : shrinked)
        {
          Vector<AABB2DPolygoni> temp;
          for(auto polyR : remains)
          {
            Vector<AABB2DPolygoni> rem;
            polyR.Difference(poly, rem);
            for(auto diff : rem)
            {
              temp.emplace_back(std::move(diff));
            }
          }
          remains.swap(temp);
        }
        for(auto polyR : remains)
        {
          Vector<AABB2Di> boxes;
          polyR.GetBoxes(boxes);
          for(auto curBox : boxes)
          {
            for(int i = curBox.m_Data[0].Y(); i<curBox.m_Data[1].Y(); ++i)
            {
              for(int j = curBox.m_Data[0].X(); j<curBox.m_Data[1].X(); ++j)
              {
                unsigned int offsetI = GetOffset(Vector2i(j,i), box);
                oValues[offsetI] = currentValue;
              }
            }
          }
        }
        for(auto& poly : shrinked)
        {
          nextPoly.emplace_back(std::move(poly));
        }
      }
      ++currentValue;
      nextPoly.swap(currentPoly);
    }

    //Now compute gradient
    unsigned int offset = 0;
    for(int i = box.m_Data[0].Y(); i< box.m_Data[1].Y(); ++i)
    {
      for(int j = box.m_Data[0].X(); j< box.m_Data[1].X(); ++j)
      {
        Vector2i grad;
        int neighSizeX = 0;
        int neighSizeY = 0;
        int value = oValues[offset];
        if( i > box.m_Data[0].Y())
        {
          grad.Y() += value - oValues[offset - dim.X()];
          ++neighSizeY;
        }
        if( i < box.m_Data[1].Y() - 1)
        {
          grad.Y() += oValues[offset + dim.X()] - value;
          ++neighSizeY;
        }
        if( j > box.m_Data[0].X())
        {
          grad.X() += value - oValues[offset - 1];
          ++neighSizeX;
        }
        if( j < box.m_Data[1].X() - 1)
        {
          grad.X() += oValues[offset + 1] - value;
          ++neighSizeX;
        }
        neighSizeX = Mathi::Clamp(neighSizeX, 1, 2);
        neighSizeY = Mathi::Clamp(neighSizeY, 1, 2);
        oGradient[offset].X() = grad.X() / neighSizeX;
        oGradient[offset].Y() = grad.Y() / neighSizeY;
        ++offset;
      }
    }
    return currentValue;
  }
}