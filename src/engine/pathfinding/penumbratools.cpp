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

  void Penumbra::Start(Vector2f const& iOrigin, float iRadius, Vector2f const& iDesiredDir, float iLookAhead)
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

  void Penumbra::AddPoint(Vector2f const& iOrigin, float iRadius)
  {
    if((iRadius + m_CasterRadius) > 0)
    {
      Point newObstacle;

      Vector2f relPos = iOrigin - m_CasterPos;

      relPos = MathTools::GetLocal(m_CasterDesiredDir, relPos);

      newObstacle.m_Radius = (iRadius + m_CasterRadius);

      if(relPos.X() > 0 && relPos.X() < m_LookAheadDist && Mathf::Abs(relPos.Y()) < newObstacle.m_Radius)
      {
        newObstacle.m_Blocking = true;
        m_DesiredDirValid = false;
      }

      newObstacle.m_Distance = relPos.Normalize();
      newObstacle.m_RelDir = relPos;

      m_Obstacles.push_back(newObstacle);
      m_OrderedPoints.push_back(m_Obstacles.size() - 1);
    }
  }

  Vector2f Penumbra::GetMidSegment(Vector2f const (&iRange)[2])
  {
    //Vector2f mid = (iRange[0] + iRange[1]) * 0.5;
    //float dist = mid.Normalize();
    //if(dist < Mathf::ZERO_TOLERANCE)
    //{
    //  mid = MathTools::GetPerp(iRange[0]);
    //}
    //mid *= Mathf::Sign(Segmentf::Cross(iRange[0], mid));
    //
    //return mid;
    return MathTools::ConeGetMidSegment(iRange);
  }

  void Penumbra::UpdateSightRange(Vector2f const (&iRange)[2], Vector2f& oMid, float& oLowLimit)
  {
    MathTools::ConeUpdateRange(iRange, oMid, oLowLimit);
  }

  bool Penumbra::IsInSightRange(Vector2f const& iDir, Vector2f const& iMidSeg, float iLowLimit, float iEpsilon)
  {
    //return iDir.Dot(iMidSeg) > (iLowLimit - iEpsilon);
    return MathTools::IsInCone(iDir, iMidSeg, iLowLimit, iEpsilon);
  }

  namespace 
  {
    bool ComparePoints(Vector2f const& iPt1, Vector2f const& iPt2)
    {
      float sign1 = Mathf::Sign(iPt1.Y());
      float sign2 = Mathf::Sign(iPt2.Y());

      if(sign1 == sign2)
      {
        //Trouble when equal ?
        return sign1 * -iPt1.X() < sign1 * -iPt2.X();
      }

      return sign1 < sign2;
    }

    void FixupSegments(Vector2f (&iPts) [2])
    {
      if(iPts[0].X() == -1.0)
      {
        if(iPts[1].Y() < 0.0)
        {
          iPts[0].Y() = -0.0;
        }
        else
        {
          iPts[0].Y() = 0.0;
        }
      }

      if(iPts[1].X() == -1.0)
      {
        if(iPts[0].Y() < 0.0)
        {
          iPts[1].Y() = -0.0;
        }
        else
        {
          iPts[1].Y() = 0.0;
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

      Vector2f relPos[] = {iSeg.m_Ext1 - m_CasterPos, iSeg.m_Ext2 - m_CasterPos};

      relPos[0] = MathTools::GetLocal(m_CasterDesiredDir, relPos[0]);
      relPos[1] = MathTools::GetLocal(m_CasterDesiredDir, relPos[1]);

      if(Segmentf::Cross(relPos[0], relPos[1]) < 0)
      {
        std::swap(relPos[0], relPos[1]);
      }

      float distance1 = relPos[0].Normalize();
      float distance2 = relPos[1].Normalize();

      if(relPos[0].Y() == 0.0 && relPos[1].Y() == 0.0)
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

      FixupSegments(reinterpret_cast<Vector2f (&)[2]>(newObstacle.m_BlockingSegment));

      float lowLimit;
      UpdateSightRange(reinterpret_cast<Vector2f const (&)[2]>(newObstacle.m_BlockingSegment), newObstacle.m_RelDir, lowLimit);

      if(IsInSightRange(Vector2f::UNIT_X, newObstacle.m_RelDir, lowLimit))
      {
        m_DesiredDirValid = false;
      }
    }
  }

  void Penumbra::AddBox(Vector2f const& iDimensions, Vector2f const& iOrigin, Vector2f const& iDirection, float iRadius)
  {
    bool blockingObs = false;

    if(iDimensions.X() + (m_CasterRadius + iRadius) > 0 || iDimensions.Y() + (m_CasterRadius + iRadius) > 0)
    {
      //{
      //  Vector2f projOrig = MathTools::GetLocal(iDirection, m_CasterPos);
      //  Vector2f projDir = MathTools::GetLocal(iDirection, m_CasterDesiredDir);
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

      Vector2f corners[] = 
      {
        iOrigin,
        iOrigin + iDimensions.X() * iDirection,
        iOrigin + iDimensions.Y() * MathTools::Perp(iDirection),
        iOrigin + iDimensions.X() * iDirection + iDimensions.Y() * MathTools::Perp(iDirection),
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

      FixupSegments(reinterpret_cast<Vector2f (&)[2]>(m_BlockingSegment));
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
        Vector2f const& pt1 = m_Obstacles[iPt1].m_BlockingSegment.m_Ext1;
        Vector2f const& pt2 = m_Obstacles[iPt2].m_BlockingSegment.m_Ext1;

        return ComparePoints(pt1, pt2);
      };
      

      std::sort(m_OrderedPoints.begin(), m_OrderedPoints.end(), sortPredicate);
    }
  }

  Vector2f Penumbra::GetWorldDirection(Vector2f const& iDir)
  {
    return m_CasterDesiredDir * iDir.X() + MathTools::GetPerp(m_CasterDesiredDir) * iDir.Y();
  }

  Vector2f Penumbra::GetWorldPosition(Vector2f const& iPos)
  {
    return m_CasterPos + GetWorldDirection(iPos);
  }

  Vector2f Penumbra::FindBestDir(Vector2f const& prevDir)
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

      drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(m_CasterPos + m_CasterDesiredDir * m_LookAheadDist), Vector4f(1.0, 1.0, 1.0, 1.0));
    }

    Vector2f firstPoint = curPt->m_BlockingSegment.m_Ext1;
    Vector2f lastPoint = curPt->m_BlockingSegment.m_Ext2;
    
    Vector2f sightRange[2];
    sightRange[0] = firstPoint;
    sightRange[1] = m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext1;

    Vector2f midSeg;
    float lowLimit;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    bool closedBehind = IsInSightRange(m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2, midSeg, lowLimit);

    if(!closedBehind)
    {
      Vector2f ext1 = firstPoint;
      Vector2f ext2 = m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext2;

      ext1.Normalize();
      ext2.Normalize();

      m_ValidDirs.push_back(ext1);
      m_ValidDirs.push_back(ext2);
    }
    sightRange[1] = lastPoint;

    UpdateSightRange(sightRange, midSeg, lowLimit);

    if(!IsInSightRange(Vector2f::UNIT_X, midSeg, lowLimit))
    {
      m_ValidDirs.push_back(Vector2f::UNIT_X);
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

        Vector2f curPos1 = GetWorldPosition(blockingSeg.m_Ext1);
        Vector2f curPos2 = GetWorldPosition(blockingSeg.m_Ext2);

        drawer->DrawLine(MathTools::To3DVec(curPos1), MathTools::To3DVec(curPos2), Vector4f(1.0, 1.0, 1.0, 1.0));

        uint32_t const numSeg = 16;
        float const angleStep = Mathf::PI * 2 / numSeg;
        
        Vector2f curPos = GetWorldPosition(nextPt->m_RelDir * nextPt->m_Distance);
        
        for(uint32_t seg = 0; seg < numSeg; ++seg)
        {
          float angle = seg * angleStep;
          Vector2f arc(Mathf::Cos(angle), Mathf::Sin(angle));
          Vector2f nextArc(Mathf::Cos(angle + angleStep), Mathf::Sin(angle + angleStep));
          drawer->DrawLine(MathTools::To3DVec(curPos + nextPt->m_Radius * arc), 
            MathTools::To3DVec(curPos + nextPt->m_Radius * nextArc), 
            nextPt->m_Blocking ? Vector4f(1.0, 0.0, 0.0, 1.0) : Vector4f(1.0, 1.0, 1.0, 0.5));
        }
      }

      if(!IsInSightRange(blockingSeg.m_Ext1, midSeg, lowLimit))
      {
        //Min tangent is after the current max tangent, there is room to go.
        
        Vector2f validDir2 = blockingSeg.m_Ext1;
        Vector2f validDir1 = sightRange[1];
        //validDir2.Normalize();
        //validDir1.Normalize();

        Vector2f midDir = validDir1 + validDir2;
        float midLen = midDir.Length();
        

        if(m_DrawDebug)
        {
          Vector2f absPos1 = GetWorldPosition(sightRange[1]);
          Vector2f absPos2 = GetWorldPosition(blockingSeg.m_Ext1);
          
          auto drawer = DebugTool::GetDrawer();
          
          drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(absPos1), Vector4f(0.0, 0.5, 0.5, 1.0));
          drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(absPos2), Vector4f(0.0, 0.5, 0.5, 1.0));
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
    Vector2f bestDir;
    bool foundValidDir = !m_ValidDirs.empty();
    if(foundValidDir)
    {
      Vector2f locPrev = MathTools::GetLocal(m_CasterDesiredDir, prevDir);

      bestDir = m_ValidDirs[0];
      float score = /*bestDir.X() + 0.1 */ locPrev.Dot(bestDir);
      for(int dirIdx = 1; dirIdx<m_ValidDirs.size(); ++dirIdx)
      {
        Vector2f const& newDir = m_ValidDirs[dirIdx];
        float newScore = /*newDir.X() + 0.1 */ newDir.Dot(locPrev);
        if(score < newScore)
        {
          bestDir = newDir;
          score = newScore;
        }
      }
    }

    if(foundValidDir)
    {
      Vector2f absDir = GetWorldDirection(bestDir);

      if(m_DrawDebug)
      {
        auto drawer = DebugTool::GetDrawer();

        drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(m_CasterPos + absDir * 10.0), Vector4f(1.0, 0.0, 1.0, 1.0));
      }
      return absDir;
    }
    else
    {
      return Vector2f::ZERO;
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

    Vector2f firstPoint = curPt->m_BlockingSegment.m_Ext1;
    Vector2f lastPoint = curPt->m_BlockingSegment.m_Ext2;

    Vector2f sightRange[2];
    sightRange[0] = firstPoint;
    sightRange[1] = m_Obstacles[m_OrderedPoints.back()].m_BlockingSegment.m_Ext1;

    Vector2f midSeg;
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