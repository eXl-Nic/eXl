/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/pathfinding/penumbratools.hpp>
#include <math/mathtools.hpp>
#include <math/segment.hpp>

#include <engine/common/debugtool.hpp>

namespace eXl
{

  void Penumbra::Start(Vec2 const& iOrigin, float iRadius, Vec2 const& iDesiredDir, float iLookAhead)
  {
    m_Obstacles.clear();
    m_OrderedPoints.clear();
    m_ValidDirs.clear();
    m_CasterPos = iOrigin;
    m_CasterDesiredDir = iDesiredDir;
    m_CasterRadius = iRadius;
    m_LookAheadDist = iLookAhead;
    m_Sorted = false;
    m_DesiredDirValid = true;
    m_DrawDebug = false;
  }

  void Penumbra::AddPoint(Vec2 const& iOrigin, float iRadius)
  {
    if((iRadius + m_CasterRadius) > 0)
    {
      Point newObstacle;

      Vec2 relPos = iOrigin - m_CasterPos;

      relPos = MathTools::GetLocal(m_CasterDesiredDir, relPos);

      newObstacle.m_Radius = (iRadius + m_CasterRadius);

      if(relPos.x > 0 && relPos.x < m_LookAheadDist && Mathf::Abs(relPos.y) < newObstacle.m_Radius)
      {
        newObstacle.m_Blocking = true;
        m_DesiredDirValid = false;
      }

      newObstacle.m_Distance = NormalizeAndGetLength(relPos);
      newObstacle.m_RelDir = relPos;

      m_Obstacles.push_back(newObstacle);
      m_OrderedPoints.push_back(m_Obstacles.size() - 1);
    }
  }

  Vec2 Penumbra::GetMidSegment(Vec2 const (&iRange)[2])
  {
    //Vec2 mid = (iRange[0] + iRange[1]) * 0.5;
    //float dist = mid.Normalize();
    //if(dist < Mathf::ZeroTolerance())
    //{
    //  mid = MathTools::GetPerp(iRange[0]);
    //}
    //mid *= Mathf::Sign(Segmentf::Cross(iRange[0], mid));
    //
    //return mid;
    return MathTools::ConeGetMidSegment(iRange);
  }

  void Penumbra::UpdateSightRange(Vec2 const (&iRange)[2], Vec2& oMid, float& oLowLimit)
  {
    MathTools::ConeUpdateRange(iRange, oMid, oLowLimit);
  }

  bool Penumbra::IsInSightRange(Vec2 const& iDir, Vec2 const& iMidSeg, float iLowLimit, float iEpsilon)
  {
    //return dot(iDir, iMidSeg) > (iLowLimit - iEpsilon);
    return MathTools::IsInCone(iDir, iMidSeg, iLowLimit, iEpsilon);
  }

  namespace 
  {
    bool ComparePoints(Vec2 const& iPt1, Vec2 const& iPt2)
    {
      float sign1 = Mathf::Sign(iPt1.y);
      float sign2 = Mathf::Sign(iPt2.y);

      if(sign1 == sign2)
      {
        //Trouble when equal ?
        return sign1 * -iPt1.x < sign1 * -iPt2.x;
      }

      return sign1 < sign2;
    }

    void FixupSegments(Vec2 (&iPts) [2])
    {
      if(iPts[0].x == -1.0)
      {
        if(iPts[1].y < 0.0)
        {
          iPts[0].y = -0.0;
        }
        else
        {
          iPts[0].y = 0.0;
        }
      }

      if(iPts[1].x == -1.0)
      {
        if(iPts[0].y < 0.0)
        {
          iPts[1].y = -0.0;
        }
        else
        {
          iPts[1].y = 0.0;
        }
      }
    }
  }

  void Penumbra::AddSegment(Segmentf const& iSeg, float iRadius)
  {
    if(iSeg.m_Ext1 != iSeg.m_Ext2 || (m_CasterRadius + iRadius) > 0 )
    {
      m_Obstacles.push_back(Point());
      m_OrderedPoints.push_back(m_Obstacles.size() - 1);
      Point& newObstacle = m_Obstacles.back();
      newObstacle.m_Radius = m_CasterRadius + iRadius;

      Vec2 relPos[] = {iSeg.m_Ext1 - m_CasterPos, iSeg.m_Ext2 - m_CasterPos};

      relPos[0] = MathTools::GetLocal(m_CasterDesiredDir, relPos[0]);
      relPos[1] = MathTools::GetLocal(m_CasterDesiredDir, relPos[1]);

      if(Segmentf::Cross(relPos[0], relPos[1]) < 0)
      {
        std::swap(relPos[0], relPos[1]);
      }

      float distance1 = NormalizeAndGetLength(relPos[0]);
      float distance2 = NormalizeAndGetLength(relPos[1]);

      if(relPos[0].y == 0.0 && relPos[1].y == 0.0)
      {
        return AddPoint(distance1 > distance2 ? relPos[1] : relPos[0], iRadius);
      }

      FixupSegments(relPos);

      float verticalFactor = Mathf::Min(newObstacle.m_Radius / distance1, 1.0);
      float horizontalFactor = Mathf::Sqrt(1.0 - verticalFactor * verticalFactor);

      newObstacle.m_BlockingSegment.m_Ext1 = (horizontalFactor * relPos[0] - verticalFactor * MathTools::GetPerp(relPos[0]));

      verticalFactor = Mathf::Min(newObstacle.m_Radius / distance2, 1.0);
      horizontalFactor = Mathf::Sqrt(1.0 - verticalFactor * verticalFactor);

      newObstacle.m_BlockingSegment.m_Ext2 = (horizontalFactor * relPos[1] + verticalFactor * MathTools::GetPerp(relPos[1]));

      FixupSegments(reinterpret_cast<Vec2 (&)[2]>(newObstacle.m_BlockingSegment));

      float lowLimit;
      UpdateSightRange(reinterpret_cast<Vec2 const (&)[2]>(newObstacle.m_BlockingSegment), newObstacle.m_RelDir, lowLimit);

      if(IsInSightRange(UnitX<Vec2>(), newObstacle.m_RelDir, lowLimit))
      {
        m_DesiredDirValid = false;
      }
    }
  }

  void Penumbra::AddBox(Vec2 const& iDimensions, Vec2 const& iOrigin, Vec2 const& iDirection, float iRadius)
  {
    bool blockingObs = false;

    if(iDimensions.x + (m_CasterRadius + iRadius) > 0 || iDimensions.y + (m_CasterRadius + iRadius) > 0)
    {
      //{
      //  Vec2 projOrig = MathTools::GetLocal(iDirection, m_CasterPos);
      //  Vec2 projDir = MathTools::GetLocal(iDirection, m_CasterDesiredDir);
      //  AABB2Df testBox(iOrigin, iDimensions);
      //  if(testBox.SegmentTest(projOrig, projDir * m_LookAheadDist))
      //  {
      //    blockingObs = true;
      //    m_DesiredDirValid = false;
      //  }
      //}

      Point pts[2];

      pts[0].m_Radius = pts[1].m_Radius = m_CasterRadius + iRadius;
      pts[0].m_Blocking = pts[1].m_Blocking = blockingObs;

      Vec2 corners[] = 
      {
        iOrigin,
        iOrigin + iDimensions.x * iDirection,
        iOrigin + iDimensions.y * MathTools::Perp(iDirection),
        iOrigin + iDimensions.x * iDirection + iDimensions.y * MathTools::Perp(iDirection),
      };

      //Wasteful...

      AddSegment(Segmentf({corners[0], corners[1]}), iRadius);
      AddSegment(Segmentf({corners[0], corners[2]}), iRadius);
      AddSegment(Segmentf({corners[3], corners[1]}), iRadius);
      AddSegment(Segmentf({corners[3], corners[2]}), iRadius);
    }
  }

  void Penumbra::Point::ComputeTangents()
  {
    if(m_BlockingSegment.m_Ext1 == m_BlockingSegment.m_Ext2)
    {
      float verticalFactor = Mathf::Min(m_Radius / m_Distance, 1.0);
      float horizontalFactor = Mathf::Sqrt(1.0 - verticalFactor * verticalFactor);
      //float distToTan = m_Distance * horizontalFactor;

      m_BlockingSegment.m_Ext1 = /*distToTan **/ (horizontalFactor * m_RelDir - verticalFactor * MathTools::GetPerp(m_RelDir));
      m_BlockingSegment.m_Ext2 = /*distToTan **/ (horizontalFactor * m_RelDir + verticalFactor * MathTools::GetPerp(m_RelDir));

      FixupSegments(reinterpret_cast<Vec2 (&)[2]>(m_BlockingSegment));
    }
  }

  void Penumbra::GenTangentsAndSortPoints()
  {
    if(!m_Sorted)
    {
      for(auto& obstacle : m_Obstacles)
      {
        obstacle.ComputeTangents();
      }

      auto sortPredicate = [this] (uint32_t const& iPt1, uint32_t const& iPt2)
      {
        Vec2 const& pt1 = m_Obstacles[iPt1].m_BlockingSegment.m_Ext1;
        Vec2 const& pt2 = m_Obstacles[iPt2].m_BlockingSegment.m_Ext1;

        return ComparePoints(pt1, pt2);
      };
      

      std::sort(m_OrderedPoints.begin(), m_OrderedPoints.end(), sortPredicate);
    }
  }

  Vec2 Penumbra::GetWorldDirection(Vec2 const& iDir)
  {
    return m_CasterDesiredDir * iDir.x + MathTools::GetPerp(m_CasterDesiredDir) * iDir.y;
  }

  Vec2 Penumbra::GetWorldPosition(Vec2 const& iPos)
  {
    return m_CasterPos + GetWorldDirection(iPos);
  }

  Vec2 Penumbra::FindBestDir(Vec2 const& prevDir)
  {
    if(m_Obstacles.empty() || m_DesiredDirValid)
    {
      return m_CasterDesiredDir;
    }

    GenTangentsAndSortPoints();

    uint32_t startIdx = 0;
    Point* curPt = &m_Obstacles[m_OrderedPoints[startIdx]];

    if(m_DrawDebug)
    {
      auto drawer = DebugTool::GetDrawer();

      drawer->DrawLine(Vec3(m_CasterPos, 0), Vec3(m_CasterPos + m_CasterDesiredDir * m_LookAheadDist, 0), Vec4(1.0, 1.0, 1.0, 1.0));
    }

    Vec2 firstPoint = curPt->m_BlockingSegment.m_Ext1;
    Vec2 lastPoint = curPt->m_BlockingSegment.m_Ext2;
    
    Vec2 sightRange[2];
    sightRange[0] = firstPoint;
    sightRange[1] = m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext1;

    Vec2 midSeg;
    float lowLimit;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    bool closedBehind = IsInSightRange(m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2, midSeg, lowLimit);

    if(!closedBehind)
    {
      Vec2 ext1 = normalize(firstPoint);
      Vec2 ext2 = normalize(m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2);

      m_ValidDirs.push_back(ext1);
      m_ValidDirs.push_back(ext2);
    }
    sightRange[1] = lastPoint;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    if(!IsInSightRange(UnitX<Vec2>(), midSeg, lowLimit))
    {
      m_ValidDirs.push_back(UnitX<Vec2>());
    }

    auto checkDirs = [&]()
    {
      for(int dirIdx = 0; dirIdx< m_ValidDirs.size(); ++dirIdx)
      {
        if(IsInSightRange(m_ValidDirs[dirIdx], midSeg, lowLimit, -0.01))
        {
          m_ValidDirs[dirIdx] = m_ValidDirs.back();
          m_ValidDirs.pop_back();
          --dirIdx;
        }
      }
    };

    for(uint32_t obsIdx = 0; obsIdx < m_Obstacles.size(); ++obsIdx)
    {
      uint32_t nextIdx = m_OrderedPoints[obsIdx];

      Point* nextPt = &m_Obstacles[nextIdx];

      Segmentf const& blockingSeg = nextPt->m_BlockingSegment;

      if(m_DrawDebug)
      {
        auto drawer = DebugTool::GetDrawer();

        Vec2 curPos1 = GetWorldPosition(blockingSeg.m_Ext1);
        Vec2 curPos2 = GetWorldPosition(blockingSeg.m_Ext2);

        drawer->DrawLine(Vec3(curPos1, 0), Vec3(curPos2, 0), Vec4(1.0, 1.0, 1.0, 1.0));

        uint32_t const numSeg = 16;
        float const angleStep = Mathf::Pi() * 2 / numSeg;
        
        Vec2 curPos = GetWorldPosition(nextPt->m_RelDir * nextPt->m_Distance);
        
        for(uint32_t seg = 0; seg < numSeg; ++seg)
        {
          float angle = seg * angleStep;
          Vec2 arc(Mathf::Cos(angle), Mathf::Sin(angle));
          Vec2 nextArc(Mathf::Cos(angle + angleStep), Mathf::Sin(angle + angleStep));
          drawer->DrawLine(Vec3(curPos + nextPt->m_Radius * arc, 0), 
            Vec3(curPos + nextPt->m_Radius * nextArc, 0), 
            nextPt->m_Blocking ? Vec4(1.0, 0.0, 0.0, 1.0) : Vec4(1.0, 1.0, 1.0, 0.5));
        }
      }

      if(!IsInSightRange(blockingSeg.m_Ext1, midSeg, lowLimit))
      {
        //Min tangent is after the current max tangent, there is room to go.
        
        Vec2 validDir2 = blockingSeg.m_Ext1;
        Vec2 validDir1 = sightRange[1];
        //validDir2.Normalize();
        //validDir1.Normalize();

        Vec2 midDir = validDir1 + validDir2;
        float midLen = length(midDir);
        

        if(m_DrawDebug)
        {
          Vec2 absPos1 = GetWorldPosition(sightRange[1]);
          Vec2 absPos2 = GetWorldPosition(blockingSeg.m_Ext1);
          
          auto drawer = DebugTool::GetDrawer();
          
          drawer->DrawLine(Vec3(m_CasterPos, 0), Vec3(absPos1, 0), Vec4(0.0, 0.5, 0.5, 1.0));
          drawer->DrawLine(Vec3(m_CasterPos, 0), Vec3(absPos2, 0), Vec4(0.0, 0.5, 0.5, 1.0));
        }
        
        sightRange[0] = blockingSeg.m_Ext1;
        sightRange[1] = blockingSeg.m_Ext2;

        UpdateSightRange(sightRange, midSeg, lowLimit);

        if(0 && midLen > 0)
        {
          m_ValidDirs.push_back(midDir / midLen);
        }
        else
        {
          m_ValidDirs.push_back(validDir1);
          m_ValidDirs.push_back(validDir2);
        }
      }
      else
      {
        bool updateSightRange = false;
        if(!IsInSightRange(blockingSeg.m_Ext1, midSeg, lowLimit))
        {
          updateSightRange = true;
          sightRange[0] = blockingSeg.m_Ext1;
        }

        if(!IsInSightRange(blockingSeg.m_Ext2, midSeg, lowLimit))
        {
          updateSightRange |= true;
          sightRange[1] = blockingSeg.m_Ext2;
          
          //lastPoint = sightRange[1];
        }

        if(updateSightRange)
        {
          UpdateSightRange(sightRange, midSeg, lowLimit);
          checkDirs();
        }
      }
    }

    //sightRange[0] = firstPoint;
    //sightRange[1] = lastPoint;
    //
    //UpdateSightRange(sightRange, midSeg, lowLimit);
    //Is the range opened at the back ?
    Vec2 bestDir;
    bool foundValidDir = !m_ValidDirs.empty();
    if(foundValidDir)
    {
      Vec2 locPrev = MathTools::GetLocal(m_CasterDesiredDir, prevDir);

      bestDir = m_ValidDirs[0];
      float score = /*bestDir.x + 0.1 */ dot(locPrev, bestDir);
      for(int dirIdx = 1; dirIdx<m_ValidDirs.size(); ++dirIdx)
      {
        Vec2 const& newDir = m_ValidDirs[dirIdx];
        float newScore = /*newDir.x + 0.1 */ dot(newDir, locPrev);
        if(score < newScore)
        {
          bestDir = newDir;
          score = newScore;
        }
      }
    }

    if(foundValidDir)
    {
      Vec2 absDir = GetWorldDirection(bestDir);

      if(m_DrawDebug)
      {
        auto drawer = DebugTool::GetDrawer();

        drawer->DrawLine(Vec3(m_CasterPos, 0), Vec3(m_CasterPos + absDir * 10.0, 0), Vec4(1.0, 0.0, 1.0, 1.0));
      }
      return absDir;
    }
    else
    {
      return Zero<Vec2>();
    }
  }

  Vector<Segmentf> Penumbra::GetAllOpenings()
  {
    Vector<Segmentf> results;
    if(m_Obstacles.empty())
    {
      results.push_back(Segmentf({-m_CasterDesiredDir, -m_CasterDesiredDir}));
    }

    GenTangentsAndSortPoints();

    Point* curPt = &m_Obstacles[m_OrderedPoints[0]];

    Vec2 firstPoint = curPt->m_BlockingSegment.m_Ext1;
    Vec2 lastPoint = curPt->m_BlockingSegment.m_Ext2;

    Vec2 sightRange[2];
    sightRange[0] = firstPoint;
    sightRange[1] = m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext1;

    Vec2 midSeg;
    float lowLimit;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    bool closedBehind = IsInSightRange(m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2, midSeg, lowLimit);

    if(!closedBehind)
    {
      results.push_back(Segmentf({m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2, firstPoint}));
    }

    sightRange[1] = lastPoint;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    auto updateResults = [&]()
    {
      for(int openingIdx = 0; openingIdx< results.size(); ++openingIdx)
      {
        auto& curRange = results[openingIdx];
        bool minInRange = IsInSightRange(curRange.m_Ext1, midSeg, lowLimit, -0.01);
        bool maxInRange = IsInSightRange(curRange.m_Ext2, midSeg, lowLimit, -0.01);

        if(minInRange)
        {
          if(maxInRange)
          {
            curRange = results.back();
            results.pop_back();
            --openingIdx;
          }
          else
          {
            curRange.m_Ext1 = sightRange[1];
          }
        }
        else if(maxInRange)
        {
          curRange.m_Ext2 = sightRange[0];
        }
      }
    };

    for(uint32_t obsIdx = 0; obsIdx < m_Obstacles.size(); ++obsIdx)
    {
      uint32_t nextIdx = m_OrderedPoints[obsIdx];

      Point* nextPt = &m_Obstacles[nextIdx];

      Segmentf const& blockingSeg = nextPt->m_BlockingSegment;

      if(!IsInSightRange(blockingSeg.m_Ext1, midSeg, lowLimit))
      {
        Segmentf openSeg({sightRange[1], blockingSeg.m_Ext1});
        
        sightRange[0] = blockingSeg.m_Ext1;
        sightRange[1] = blockingSeg.m_Ext2;

        UpdateSightRange(sightRange, midSeg, lowLimit);

        updateResults();

        results.push_back(openSeg);
      }
      else
      {
        bool updateSightRange = false;
        if(!IsInSightRange(blockingSeg.m_Ext1, midSeg, lowLimit))
        {
          updateSightRange = true;
          sightRange[0] = blockingSeg.m_Ext1;
        }

        if(!IsInSightRange(blockingSeg.m_Ext2, midSeg, lowLimit))
        {
          updateSightRange |= true;
          sightRange[1] = blockingSeg.m_Ext2;
        }

        if(updateSightRange)
        {
          UpdateSightRange(sightRange, midSeg, lowLimit);
          
          updateResults();
        }
      }
    }
    
    for(auto& range : results)
    {
      range.m_Ext1 = GetWorldDirection(range.m_Ext1);
      range.m_Ext2 = GetWorldDirection(range.m_Ext2);
    }

    return results;
  }

  void Penumbra::DrawDebug()
  {
    m_DrawDebug = true;
  }
}