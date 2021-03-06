/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/pathfinding/velocityobstacle.hpp>
#include <engine/pathfinding/penumbratools.hpp>
#include <math/mathtools.hpp>
#include <gen/poissonsampling.hpp>

#include <engine/common/debugtool.hpp>

namespace eXl
{
  static Vector<Vec2> const& GetSamples()
  {
    static Vector<Vec2> s_RandomSet = []
    {
      Vector<Vec2> circlePoints;
      Random* rand = Random::CreateDefaultRNG(0);
      PoissonDiskSampling sampler(Polygoni(AABB2Di(-100, -100, 100, 100)), *rand);
      sampler.Sample(7.5, 7.5, 256);

      Vector<Vec2d> points;
      sampler.GetLayer(0, points);

      for (auto const& pt : points)
      {
        if (length(pt) < 100)
        {
          circlePoints.push_back(Vec2(pt.x / 100, pt.y / 100));
        }
      }

      delete rand;

      return circlePoints;
    }();

    return s_RandomSet;
  }

  VelocityObstacle::VelocityObstacle(Random& iRand)
    : m_Rand(iRand)
  {

  }

  void VelocityObstacle::Start(void* iActor, Vec2 const& iOrigin, float iRadius, Vec2 const& iDesiredDir, float iMaxVelocity)
  {
    m_Obstacles.clear();
    m_CircleArcs.clear();
    m_CasterPos = iOrigin;
    m_CasterDesiredDir = iDesiredDir;
    if (iDesiredDir == Zero<Vec2>())
    {
      m_CasterDesiredDir = UnitX<Vec2>();
      m_PureAvoidance = true;
    }
    else
    {
      m_PureAvoidance = false;
    }
    m_PerpDir = MathTools::GetPerp(m_CasterDesiredDir);
    m_CasterRadius = iRadius;
    m_MaxVelocity = iMaxVelocity;
    m_MaxVelocitySq = m_MaxVelocity * m_MaxVelocity;
    m_DrawDebug = false;
    m_CircleCut = false;
    m_WholeCircleCut = false;
    m_Checked.clear();
    m_Actor = iActor;
    m_InterParams.m_UsrEvts.clear();
    m_RandPts.clear();
    m_RandPtsScore.clear();
    m_PolyMesh.Clear();
    m_VisitedSeg.clear();
    m_FacesSeg.clear();
    m_FacesAlloc.clear();
  }

  void VelocityObstacle::Arc::Complement()
  {
    m_MidVec *= -1;
    m_LowLimit *= -1;
  }

  static const float s_sqrt2 = sqrt(2.0f);

  void VelocityObstacle::AddObstacle(Vec2 const& iWorldOrig, Vec2 const (&iWorldVecDir)[2], float iDistance)
  {
    static uint32_t numExec = 0;
    ++numExec;

    Obstacle newObstacle;

    Vec2 iOrig(dot(iWorldOrig, m_CasterDesiredDir), dot(iWorldOrig, m_PerpDir));

    float const otherVel = length(iOrig);

    bool velInside = otherVel - Mathf::ZeroTolerance() < m_MaxVelocity;

    uint32_t numArcs = 0;
    Arc circleArcs[2];

    Vec2 vecDir[2];
    vecDir[0] = Vec2(dot(iWorldVecDir[0], m_CasterDesiredDir), dot(iWorldVecDir[0], m_PerpDir));
    vecDir[1] = Vec2(dot(iWorldVecDir[1], m_CasterDesiredDir), dot(iWorldVecDir[1], m_PerpDir));

    AABB2Df velocityBox = AABB2Df::FromMinAndSize(-One<Vec2>() * m_MaxVelocity, One<Vec2>() * 2 * m_MaxVelocity);

    for (int32_t i = 0; i < 2; ++i)
    {
      float dotDir = dot(iOrig, vecDir[i]);
      if (!velInside && dotDir > 0)
      {
        continue;
      }
      Vec2 projPt = iOrig - vecDir[i] * dotDir;
      float kSq = m_MaxVelocitySq - (otherVel * otherVel - dotDir * dotDir);

      if (kSq > Mathf::ZeroTolerance())
      {
        Vec2 ray = vecDir[i] * (otherVel + s_sqrt2 * m_MaxVelocity);
        auto boxPt = velocityBox.SegmentTest(iOrig + ray, -ray * 2.0, 0.0);
        //
        //eXl_ASSERT(!(!boxPt));
        //float k = Mathf::Sqrt(kSq);
        //auto circlePt = projPt + k * vecDir[i];
        //eXl_ASSERT(Mathf::Abs(circlePt.Length() - m_MaxVelocity) < Mathf::ZeroTolerance());

        Vec2(&exts)[2] = reinterpret_cast<Vec2(&)[2]>(newObstacle.m_Segs[i]);

        if (velInside)
        {
          exts[1 - i] = iOrig;
          exts[i] = *boxPt;
          //exts[i] = circlePt;
        }
        else
        {
          exts[i] = *boxPt;

          boxPt = velocityBox.SegmentTest(iOrig - ray, ray * 2.0, 0.0);
          eXl_ASSERT(!(!boxPt));

          exts[1 - i] = *boxPt;

          //exts[1 - i] = projPt - k * vecDir[i];
          //exts[i] = circlePt;

          Vec2 extNormalized[2] = {
            normalize(exts[1]),
            normalize(exts[0])
          };

          Penumbra::UpdateSightRange(extNormalized, circleArcs[numArcs].m_MidVec, circleArcs[numArcs].m_LowLimit);

          ++numArcs;
        }
      }
    }

    if (newObstacle.m_Segs[0].m_Ext1 == newObstacle.m_Segs[0].m_Ext2
      && newObstacle.m_Segs[1].m_Ext1 == newObstacle.m_Segs[1].m_Ext2)
    {
      return;
    }

    //if (newObstacle.m_Segs[0].m_Ext1 == newObstacle.m_Segs[0].m_Ext2)
    //{
    //  newObstacle.m_Segs[0].m_Ext1 = newObstacle.m_Segs[0].m_Ext2 = newObstacle.m_Segs[1].m_Ext1;
    //}
    //if (newObstacle.m_Segs[1].m_Ext1 == newObstacle.m_Segs[1].m_Ext2)
    //{
    //  newObstacle.m_Segs[1].m_Ext1 = newObstacle.m_Segs[1].m_Ext2 = newObstacle.m_Segs[0].m_Ext2;
    //}

    if (velInside)
    {
      Vec2 vecs[2] = { 
        normalize(newObstacle.m_Segs[0].m_Ext1), 
        normalize(newObstacle.m_Segs[1].m_Ext2) 
      };
      Penumbra::UpdateSightRange(vecs, circleArcs[0].m_MidVec, circleArcs[0].m_LowLimit);


      //if(m_DrawDebug)
      //{
      //  auto* drawer = DebugTool::GetDrawer();
      //
      //  drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(m_CasterPos + circleArcs[numArcs].m_MidVec * m_MaxVelocity), Vec4(0.0, 0.0, /1.0, /1.0));
      //}

      ++numArcs;
    }

    if (numArcs == 0)
    {
      //Check if whole circle is cut.
      Penumbra::UpdateSightRange(vecDir, circleArcs[0].m_MidVec, circleArcs[0].m_LowLimit);
      if (Penumbra::IsInSightRange(iOrig / otherVel, circleArcs[0].m_MidVec, circleArcs[0].m_LowLimit))
      {
        m_WholeCircleCut = true;
        m_CircleArcs.clear();
        //return;
      }
    }

    float const arcTolerance = (1.0f - 0.02);

    // Combine arcs
    if (m_CircleArcs.empty())
    {
      if (!m_CircleCut)
      {
        for (unsigned int i = 0; i < numArcs; ++i)
        {
          auto& arc = circleArcs[i];
          if (velInside)
          {
            arc.Complement();
          }
          if (arc.m_LowLimit < arcTolerance)
          {
            m_CircleArcs.push_back(arc);
          }
        }
        if (m_CircleArcs.empty())
        {
          m_CircleCut = true;
        }
      }
    }
    else
    {
      for (unsigned int i = 0; i < numArcs; ++i)
      {
        auto& cutArc = circleArcs[i];
        if (!velInside)
        {
          cutArc.Complement();
        }
        Vec2 cutVec1;
        Vec2 cutVec2;
        {
          Vec2 perpDir = MathTools::GetPerp(cutArc.m_MidVec);
          float verticalFactor = Mathf::Sqrt(1.0 - cutArc.m_LowLimit * cutArc.m_LowLimit);

          cutVec1 = cutArc.m_MidVec * cutArc.m_LowLimit - verticalFactor * perpDir;
          cutVec2 = cutArc.m_MidVec * cutArc.m_LowLimit + verticalFactor * perpDir;
        }

        for (int j = m_CircleArcs.size() - 1; j >= 0; --j)
        {
          auto& arc = m_CircleArcs[j];
          if (Penumbra::IsInSightRange(cutVec1, arc.m_MidVec, arc.m_LowLimit))
          {
            if (Penumbra::IsInSightRange(cutVec2, arc.m_MidVec, arc.m_LowLimit))
            {
              Vec2 perpDir = MathTools::GetPerp(arc.m_MidVec);
              float verticalFactor = Mathf::Sqrt(1.0 - arc.m_LowLimit * arc.m_LowLimit);
              //Cut arc in two.
              Vec2 lowVec[2] = { arc.m_MidVec * arc.m_LowLimit - verticalFactor * perpDir, cutVec1 };
              Vec2 highVec[2] = { cutVec2, arc.m_MidVec * arc.m_LowLimit + verticalFactor * perpDir };

              bool replaced = false;

              Arc newArc;
              Penumbra::UpdateSightRange(lowVec, newArc.m_MidVec, newArc.m_LowLimit);
              if (newArc.m_LowLimit < arcTolerance)
              {
                arc = newArc;
                replaced = true;
              }

              Penumbra::UpdateSightRange(highVec, newArc.m_MidVec, newArc.m_LowLimit);
              if (newArc.m_LowLimit < arcTolerance)
              {
                if (replaced)
                {
                  m_CircleArcs.push_back(newArc);
                }
                else
                {
                  arc = newArc;
                  replaced = true;
                }
              }

              if (!replaced)
              {
                m_CircleArcs[j] = m_CircleArcs.back();
                m_CircleArcs.pop_back();
              }
            }
            else
            {
              Vec2 perpDir = MathTools::GetPerp(arc.m_MidVec);
              float verticalFactor = Mathf::Sqrt(1.0 - arc.m_LowLimit * arc.m_LowLimit);
              Vec2 lowVec[2] = { arc.m_MidVec * arc.m_LowLimit - verticalFactor * perpDir, cutVec1 };

              Penumbra::UpdateSightRange(lowVec, arc.m_MidVec, arc.m_LowLimit);
              if (arc.m_LowLimit >= arcTolerance)
              {
                m_CircleArcs[j] = m_CircleArcs.back();
                m_CircleArcs.pop_back();
              }
            }
          }
          else if (Penumbra::IsInSightRange(cutVec2, arc.m_MidVec, arc.m_LowLimit))
          {
            Vec2 perpDir = MathTools::GetPerp(arc.m_MidVec);
            float verticalFactor = Mathf::Sqrt(1.0 - arc.m_LowLimit * arc.m_LowLimit);
            Vec2 highVec[2] = { cutVec2, arc.m_MidVec * arc.m_LowLimit + verticalFactor * perpDir };

            Penumbra::UpdateSightRange(highVec, arc.m_MidVec, arc.m_LowLimit);
            if (arc.m_LowLimit >= arcTolerance)
            {
              m_CircleArcs[j] = m_CircleArcs.back();
              m_CircleArcs.pop_back();
            }
          }
        }
      }
      if (m_CircleArcs.empty())
      {
        m_CircleCut = true;
      }
    }

    if (m_Obstacles.empty())
    {
      m_ObstaclesBox = AABB2Df::FromMinAndSize(iOrig, Zero<Vec2>());
    }
    else
    {
      m_ObstaclesBox.Absorb(iOrig);
    }
    m_ObstaclesBox.Absorb(newObstacle.m_Segs[0].m_Ext1);
    m_ObstaclesBox.Absorb(newObstacle.m_Segs[1].m_Ext2);

    newObstacle.m_Origin = iOrig;
    newObstacle.m_Velocity = otherVel;
    newObstacle.m_Distance = iDistance;
    m_Obstacles.push_back(newObstacle);
  }

  void VelocityObstacle::AddPoint(Vec2 const& iOrigin, float iRadius, Vec2 const& iVelocity)
  {
    //if(m_CircleCut)
    //{
    //  return;
    //}
    if ((iRadius + m_CasterRadius) > 0)
    {
      Vec2 relPos = iOrigin - m_CasterPos;

      float totRadius = (iRadius + m_CasterRadius);
      float distance = NormalizeAndGetLength(relPos);
      if (distance > Mathf::ZeroTolerance())
      {
        Vec2 vecDir[2];
        {
          float verticalFactor = Mathf::Min(totRadius / distance, 1.0);
          float horizontalFactor = Mathf::Sqrt(1.0 - verticalFactor * verticalFactor);

          vecDir[0] = horizontalFactor * relPos - verticalFactor * MathTools::GetPerp(relPos);
          vecDir[1] = horizontalFactor * relPos + verticalFactor * MathTools::GetPerp(relPos);
        }

        AddObstacle(iVelocity, vecDir, distance > totRadius ? distance - totRadius : 0);

      }
    }
  }

  void VelocityObstacle::AddSegment(Segmentf const& iSeg, float iRadius)
  {
    Segmentf orderedSeg = iSeg;

    if (Segmentf::IsLeft(m_CasterPos, iSeg.m_Ext1, iSeg.m_Ext2) < 0)
    {
      std::swap(orderedSeg.m_Ext1, orderedSeg.m_Ext2);
    }

    Vec2 vecDir[2];
    Vec2 segDir = orderedSeg.m_Ext2 - orderedSeg.m_Ext1;
    float segLen = NormalizeAndGetLength(segDir);

    vecDir[0] = (orderedSeg.m_Ext1 - segDir * iRadius) - m_CasterPos;
    vecDir[1] = (orderedSeg.m_Ext2 + segDir * iRadius) - m_CasterPos;

    float disExt0 = NormalizeAndGetLength(vecDir[0]);
    float disExt1 = NormalizeAndGetLength(vecDir[1]);

    auto perpDir = MathTools::GetPerp(segDir);

    float distance;

    if (Segmentf::IsLeft(Zero<Vec2>(), perpDir, vecDir[0]) * Segmentf::IsLeft(Zero<Vec2>(), perpDir, vecDir[1]) < 0)
    {
      distance = Mathf::Abs(disExt0 * dot(vecDir[0], perpDir));
    }
    else
    {
      distance = Mathf::Min(disExt0, disExt1);
    }

    AddObstacle(Zero<Vec2>(), vecDir, distance);
  }

  VelocityObstacle::BestVelocity::BestVelocity(float iDirMult)
    : dirMult(iDirMult)
  {
    std::fill(curScore, curScore + 4, -Mathf::MaxReal());
  }

  float VelocityObstacle::BestVelocity::ComputeScore(Vec2 const& iOptimalDir, Vec2 const& iCandidateDir, float iCandidateSpeed)
  {
    if (iCandidateSpeed > Mathf::ZeroTolerance() && iCandidateSpeed < (1.0 + Mathf::ZeroTolerance()))
    {
      float score = dot(iCandidateDir, iOptimalDir) * dirMult + iCandidateSpeed;
      return score;
    }
    return -Mathf::MaxReal();
  }

  void VelocityObstacle::BestVelocity::Update(Vec2 const& iCandidateDir, float iCandidateSpeed)
  {
    Vec2 canonicalDirs[] = 
    {
      UnitX<Vec2>(),
      UnitY<Vec2>(),
      -UnitY<Vec2>(),
      -UnitX<Vec2>()
    };

    for (uint32_t i = 0; i < /*4*/1; ++i)
    {
      float score = ComputeScore(canonicalDirs[i], iCandidateDir, iCandidateSpeed);
      if (score > curScore[i])
      {
        curDir[i] = iCandidateDir;
        curSpeed[i] = iCandidateSpeed;
        curScore[i] = score;
      }
    }
  }

}

#define PRECISE_VO
#define OPTIMISTIC_PREAMBLE

#include <fstream>
#include <core/stream/jsonstreamer.hpp>

namespace eXl
{
  void SaveSegments(Vector<Segmenti> const& iSegments)
  {
    std::ofstream outputStream;
    outputStream.open("D:\\DebugIntersector.json");

    JSONStreamer streamer(&outputStream);
    streamer.Begin();
    
    streamer.Write(&iSegments);

    streamer.End();

    outputStream.flush();
  }

  Vec2 VelocityObstacle::FindBestVelocity(Vec2 const& prevDir)
  {
    if(m_WholeCircleCut)
    {
      return Zero<Vec2>();
    }

    if(m_Obstacles.empty())
    {
      return m_PureAvoidance ? Zero<Vec2>() : m_CasterDesiredDir * m_MaxVelocity;
    }

    BestVelocity velocities(m_PureAvoidance ? 0.0 : 0.5);

    Vector<Vec2> const& pts = GetSamples();

#if !defined(PRECISE_VO) || defined(OPTIMISTIC_PREAMBLE) 
    int32_t const numPts = pts.size();
#else
    int32_t const numPts = 0;
#endif

    for (int32_t i = -1; i<numPts; ++i)
    {
      bool valid = true;
      Vec2 point = (i >= 0 
        ? pts[i] 
        : (m_PureAvoidance ? Zero<Vec2>() : UnitX<Vec2>() )) * m_MaxVelocity;
      for (uint32_t obsIdx = 0; obsIdx < m_Obstacles.size(); ++obsIdx)
      {
        auto const& obstacle = m_Obstacles[obsIdx];
        //Vec2 const& tip = obstacle.m_Segs[0].m_Ext2;
        Vec2 ext1_0 = obstacle.m_Segs[0].m_Ext2;
        Vec2 ext2_0 = obstacle.m_Segs[0].m_Ext1;

        Vec2 ext1_1 = obstacle.m_Segs[1].m_Ext1;
        Vec2 ext2_1 = obstacle.m_Segs[1].m_Ext2;

        if ((ext1_0 == ext2_0 && Segmentf::IsLeft(ext1_1, ext2_1, point) < 0)
          || (ext1_1 == ext2_1 && Segmentf::IsLeft(ext1_0, ext2_0, point) > 0))
        {
          valid = false;
          break;
        }
        else
        {
          //if (Segmentf::IsLeft(ext1_0, ext2_0, ext2_1) < 0)
          //{
          //  std::swap(ext1_0, ext1_1);
          //  std::swap(ext2_0, ext2_1);
          //}
          if (Segmentf::IsLeft(ext1_0, ext2_0, point) > 0 && Segmentf::IsLeft(ext1_1, ext2_1, point) < 0)
          {
            valid = false;
            break;
          }
        }
      }

      if (i == -1 && m_PureAvoidance && valid)
      {
        return Zero<Vec2>();
      }

      if (valid)
      {
        Vec2 dir = point;
        float speed = NormalizeAndGetLength(dir);
        velocities.Update(dir, speed / m_MaxVelocity);
      }

      if (m_DrawDebug)
      {
        auto drawer = DebugTool::GetDrawer();

        Vec2 center = m_CasterPos + m_CasterDesiredDir * point.x + m_PerpDir * point.y;
        Vec4 color = valid ? Vec4(0.0, 1.0, 0.0, 1.0) : Vec4(1.0, 0.0, 0.0, 1.0);

        drawer->DrawLine(Vec3(center, 0) + Vec3(-1.0, -1.0, 0.0) * 0.25, Vec3(center, 0) + Vec3(1.0, 1.0, 0.0) * 0.25, color);
        drawer->DrawLine(Vec3(center, 0) + Vec3(1.0, -1.0, 0.0) * 0.25, Vec3(center, 0) + Vec3(-1.0, 1.0, 0.0) * 0.25, color);
      }
    }

    float* bestScore = std::max_element(velocities.curScore, velocities.curScore + 4);
    uint32_t priority = bestScore - velocities.curScore;
    if (*bestScore > -Mathf::Epsilon())
      //for (uint32_t priority = 0; priority < 4; ++priority)
    {
      //if (velocities.curScore[priority] > )
      {
        return (m_CasterDesiredDir * velocities.curDir[priority].x
          + m_PerpDir * velocities.curDir[priority].y)
          * m_MaxVelocity * velocities.curSpeed[priority];
      }
    }

    if (!m_CircleCut && m_CircleArcs.empty())
    {
      return m_CasterDesiredDir * m_MaxVelocity;
    }
    else
    {
      if (!m_CircleCut)
      {
        for (auto const& arc : m_CircleArcs)
        {
          if (m_DrawDebug)
          {
            auto drawer = DebugTool::GetDrawer();

            Vec2 perpDir = MathTools::GetPerp(arc.m_MidVec);

            float xInc = (1.0 - arc.m_LowLimit) / 8.0;
            Vec4 color = Vec4(0.0, 1.0, 0.0, 1.0);
            for (uint32_t arcSeg = 0; arcSeg < 8; ++arcSeg)
            {
              float horizontalFactor = arc.m_LowLimit + arcSeg * xInc;
              float verticalFactor = Mathf::Sqrt(1.0 - horizontalFactor * horizontalFactor);

              Vec2 pts[4];
              pts[0] = horizontalFactor * arc.m_MidVec - verticalFactor * perpDir;
              pts[2] = horizontalFactor * arc.m_MidVec + verticalFactor * perpDir;

              horizontalFactor = arc.m_LowLimit + (arcSeg + 1) * xInc;
              verticalFactor = Mathf::Sqrt(1.0 - horizontalFactor * horizontalFactor);

              pts[1] = horizontalFactor * arc.m_MidVec - verticalFactor * perpDir;
              pts[3] = horizontalFactor * arc.m_MidVec + verticalFactor * perpDir;

              for (auto& pt : pts)
              {
                pt = m_CasterPos + (m_CasterDesiredDir * pt.x + m_PerpDir * pt.y) * m_MaxVelocity;

              }

              drawer->DrawLine(Vec3(pts[0], 0), Vec3(pts[1], 0), color);
              drawer->DrawLine(Vec3(pts[2], 0), Vec3(pts[3], 0), color);

            }
          }

          if (Penumbra::IsInSightRange(UnitX<Vec2>(), arc.m_MidVec, arc.m_LowLimit))
          {
            return m_CasterDesiredDir * m_MaxVelocity;
          }
          else
          {
            Vec2 perpDir = MathTools::GetPerp(arc.m_MidVec);
            float verticalFactor = Mathf::Sqrt(1.0 - arc.m_LowLimit * arc.m_LowLimit);
            Vec2 dirs[3];
            dirs[0] = arc.m_MidVec;
            dirs[1] = arc.m_MidVec * arc.m_LowLimit - verticalFactor * perpDir;
            dirs[2] = arc.m_MidVec * arc.m_LowLimit + verticalFactor * perpDir;
            
            for (auto& dir : dirs)
            {
              velocities.Update(dir, 1.0);
            }
          }
        }
      }
    }

#ifdef PRECISE_VO
    m_HalfSeg.clear();
    m_ObstacleSegs.clear();

    const float boxRange = m_MaxVelocity;
    const int32_t c_GridSize = 1000;
    AABB2Di intBox = AABB2Di::FromMinAndSize(One<Vec2i>() * -c_GridSize, 2 * One<Vec2i>() * (c_GridSize + 1));

    float const obsBoxDiag = length(m_ObstaclesBox.GetSize());

    AABB2Df boxVel = AABB2Df::FromMinAndSize(One<Vec2>() * -m_MaxVelocity, One<Vec2>() * 2 *m_MaxVelocity);

    for(int32_t obsIdx = 0; obsIdx < m_Obstacles.size(); ++obsIdx)
    {
      auto& obstacle = m_Obstacles[obsIdx];

      for (auto& seg : obstacle.m_Segs)
      {
        MathTools::ClampToBox(seg.m_Ext1, m_ObstaclesBox);
        MathTools::ClampToBox(seg.m_Ext2, m_ObstaclesBox);

        Vec2i ext1 = MathTools::ToIVec(seg.m_Ext1 * c_GridSize / boxRange);
        Vec2i ext2 = MathTools::ToIVec(seg.m_Ext2 * c_GridSize / boxRange);

        MathTools::ClampToBox(ext1, intBox);
        MathTools::ClampToBox(ext2, intBox);

        m_ObstacleSegs.push_back(Segmenti({ ext1, ext2 }));
      }

      //if ((Mathi::Abs(ext1.x()) > c_GridSize || Mathi::Abs(ext1.y()) > c_GridSize))
      //{
      //  m_ObstacleSegs.push_back(Segmenti({ tip, tip }));
      //}
      //else
      //{
      //  m_ObstacleSegs.push_back(Segmenti({ ext1, tip }));
      //}
      //
      //if ((Mathi::Abs(ext2.x()) > c_GridSize || Mathi::Abs(ext2.y()) > c_GridSize))
      //{
      //  m_ObstacleSegs.push_back(Segmenti({ tip, tip }));
      //}
      //else
      //{
      //  m_ObstacleSegs.push_back(Segmenti({ tip, ext2 }));
      //}
    }

    //m_Checked.resize(m_ObstacleSegs.size() * 2, false);

    m_ObstacleSegs.push_back(Segmenti({Vec2i(-c_GridSize, -c_GridSize), Vec2i(-c_GridSize,  c_GridSize)}));
    m_ObstacleSegs.push_back(Segmenti({Vec2i(-c_GridSize,  c_GridSize), Vec2i( c_GridSize,  c_GridSize)}));
    m_ObstacleSegs.push_back(Segmenti({Vec2i( c_GridSize,  c_GridSize), Vec2i( c_GridSize, -c_GridSize)}));
    m_ObstacleSegs.push_back(Segmenti({Vec2i( c_GridSize, -c_GridSize), Vec2i(-c_GridSize, -c_GridSize)}));

    //SaveSegments(m_ObstacleSegs);

    if(!m_Inter.IntersectSegments(m_ObstacleSegs, m_HalfSeg, m_InterParams))
    {
      return Zero<Vec2>();
    }

    for(int32_t i = 0; i<(int)m_HalfSeg.size(); ++i)
    {
      bool validSeg = true;
      auto& curSeg = m_HalfSeg[i].second;
      //uint32_t obsSrc = m_HalfSeg[i].first / 2;
      Segmentf fltSeg = {MathTools::ToFVec(curSeg.m_Ext1) / c_GridSize * boxRange, MathTools::ToFVec(curSeg.m_Ext2) / c_GridSize * boxRange};

      m_PolyMesh.InsertEdge(curSeg, fltSeg, m_HalfSeg[i].first);
    }

    m_VisitedSeg.resize(m_PolyMesh.edges.size(), -1);
    m_FacesSeg.reserve(m_PolyMesh.edges.size());
    int32_t outerFace = -1;

    for (int32_t i = -1; i < (int)m_PolyMesh.edges.size(); ++i)
    {
      bool culledFace = false;
      uint32_t segToInspect;
      if(i == -1)
      {
        uint32_t leftmostPoint = m_PolyMesh.smallerPoint->second;
        // Outerseg will have an exterior direction along .x
        PolyVertex const& leftmostVertex = m_PolyMesh.vertices[leftmostPoint];

        uint32_t firstIncomingEdge = leftmostVertex.firstEdge;
        if(m_PolyMesh.edges[firstIncomingEdge].dstVtx != leftmostPoint)
        {
          firstIncomingEdge = m_PolyMesh.edges[firstIncomingEdge].sibling;
        }

        PolyHalfEdge const* firstIncoming = &m_PolyMesh.edges[firstIncomingEdge];
        PolyHalfEdge const* curIncoming = firstIncoming;
        do
        {
          PolyHalfEdge const& nextEdge = m_PolyMesh.edges[curIncoming->nextEdge];

          Vec2 edgeDirs[] = {-curIncoming->normDir, nextEdge.normDir};
          Vec2 midSeg = MathTools::ConeGetMidSegment(edgeDirs);

          if(dot(midSeg, UnitX<Vec2>()) < 0)
          {
            segToInspect = curIncoming - m_PolyMesh.edges.data();
            outerFace = 0;
            break;
          }
        }
        while((curIncoming = m_PolyMesh.GetNextIncomingEdge(leftmostPoint, curIncoming)) != firstIncoming);

        eXl_ASSERT(outerFace != -1);
        culledFace = true;
      }
      else
      {
        segToInspect = i;
      }

      if (m_VisitedSeg[segToInspect] != -1)
      {
        continue;
      }

      uint32_t faceIdx = m_FacesAlloc.size();
      m_FacesAlloc.push_back(std::pair<uint32_t, uint32_t>(m_FacesSeg.size(), 0));
      std::pair<uint32_t, uint32_t>& curFace = m_FacesAlloc.back();

      uint32_t checkCounter = m_PolyMesh.edges.size();

      uint32_t firstEdge = segToInspect;
      uint32_t curEdge = firstEdge;

      uint32_t testVtxId = m_PolyMesh.edges[firstEdge].dstVtx;
      Vec2 testVtxPos = m_PolyMesh.vertices[testVtxId].positionf;

      float minDist = Mathf::MaxReal();

      while (checkCounter != 0 && (curFace.second == 0 || curEdge != firstEdge))
      {
        if(testVtxId != m_PolyMesh.edges[curEdge].srcVtx && testVtxId != m_PolyMesh.edges[curEdge].dstVtx)
        {
          Vec2 pt1 = m_PolyMesh.vertices[m_PolyMesh.edges[curEdge].srcVtx].positionf;
          Vec2 pt2 = m_PolyMesh.vertices[m_PolyMesh.edges[curEdge].dstVtx].positionf;

          Vec2 dummy;
          float curDist = Segmentf::NearestPointSeg(pt1, pt2, testVtxPos, dummy);
          
          if(curDist > Mathf::Epsilon() && curDist < minDist)
          {
            minDist = curDist;
          }
        }

        m_VisitedSeg[curEdge] = faceIdx;
        curFace.second++;
        m_FacesSeg.push_back(curEdge);
        curEdge = m_PolyMesh.edges[curEdge].nextEdge;

        checkCounter--;
      }

      if(minDist < Mathf::MaxReal() && !culledFace)
      {
        PolyHalfEdge const& curPolyEdge = m_PolyMesh.edges[firstEdge];
        uint32_t nextEdge = curPolyEdge.nextEdge;
        PolyHalfEdge const& nextPolyEdge = m_PolyMesh.edges[nextEdge];

        Vec2 edgeDirs[] = {-curPolyEdge.normDir, nextPolyEdge.normDir};
        Vec2 midSeg = MathTools::ConeGetMidSegment(edgeDirs);

        Vec2 point = testVtxPos + midSeg * minDist * 0.5;

        for(uint32_t obsIdx = 0; obsIdx < m_Obstacles.size(); ++obsIdx)
        {
          auto const& obstacle = m_Obstacles[obsIdx];
          //Vec2 const& tip = obstacle.m_Segs[0].m_Ext2;
          Vec2 ext1_0 = obstacle.m_Segs[0].m_Ext2;
          Vec2 ext2_0 = obstacle.m_Segs[0].m_Ext1;
          
          Vec2 ext1_1 = obstacle.m_Segs[1].m_Ext1;
          Vec2 ext2_1 = obstacle.m_Segs[1].m_Ext2;

          if ((ext1_0 == ext2_0 && Segmentf::IsLeft(ext1_1, ext2_1, point) < 0)
           || (ext1_1 == ext2_1 && Segmentf::IsLeft(ext1_0, ext2_0, point) > 0))
          {
            culledFace = true;
          }
          else
          {
            //if (Segmentf::IsLeft(ext1_0, ext2_0, ext2_1) < 0)
            //{
            //  std::swap(ext1_0, ext1_1);
            //  std::swap(ext2_0, ext2_1);
            //}
            if (Segmentf::IsLeft(ext1_0, ext2_0, point) > 0 && Segmentf::IsLeft(ext1_1, ext2_1, point) < 0)
            {
              culledFace = true;
            }
          }
        }
      }
      else
      {
        culledFace = true;
      }

      if(culledFace)
      {
        m_FacesAlloc[faceIdx].second = 0;
      }
    }

    eXl_ASSERT(outerFace >= 0);

    if(m_DrawDebug)
    {
      Vector<bool> drawn(m_PolyMesh.edges.size(), false);

      for(int32_t i = 0; i<m_FacesAlloc.size(); ++i)
      {
        
        for(uint32_t faceIdx = m_FacesAlloc[i].first; faceIdx != m_FacesAlloc[i].first + m_FacesAlloc[i].second; ++faceIdx)
        {
          uint32_t seg = m_FacesSeg[faceIdx];
          drawn[seg] = true;
          PolyHalfEdge const& curPolyEdge = m_PolyMesh.edges[seg];
          drawn[curPolyEdge.sibling] = true;

          if (i == outerFace)
          {
            continue;
          }

          if (m_VisitedSeg[curPolyEdge.sibling] == outerFace)
          {
            continue;
          }
          //auto const& curSeg = m_HalfSeg[i].second;

          //Segmentf fltSeg = {MathTools::ToFVec(curSeg.m_Ext1) / c_GridSize * boxRange, MathTools::ToFVec(curSeg.m_Ext2) / c_GridSize * boxRange};
          Segmentf fltSeg = {m_PolyMesh.vertices[curPolyEdge.srcVtx].positionf, m_PolyMesh.vertices[curPolyEdge.dstVtx].positionf};

          auto drawer = DebugTool::GetDrawer();
    
          auto pt1 = m_CasterPos + m_CasterDesiredDir * fltSeg.m_Ext1.x + m_PerpDir * fltSeg.m_Ext1.y;
          auto pt2 = m_CasterPos + m_CasterDesiredDir * fltSeg.m_Ext2.x + m_PerpDir * fltSeg.m_Ext2.y;

          drawer->DrawLine(Vec3(pt1, 0), Vec3(pt2, 0), Vec4(0.0, 1.0, 0.0, 1.0));
        }
      }

      for(auto const& curPolyEdge : m_PolyMesh.edges)
      {
        if(!drawn[(&curPolyEdge - m_PolyMesh.edges.data())])
        {
          Segmentf fltSeg = {m_PolyMesh.vertices[curPolyEdge.srcVtx].positionf, m_PolyMesh.vertices[curPolyEdge.dstVtx].positionf};

          auto drawer = DebugTool::GetDrawer();

          auto pt1 = m_CasterPos + m_CasterDesiredDir * fltSeg.m_Ext1.x + m_PerpDir * fltSeg.m_Ext1.y;
          auto pt2 = m_CasterPos + m_CasterDesiredDir * fltSeg.m_Ext2.x + m_PerpDir * fltSeg.m_Ext2.y;

          drawer->DrawLine(Vec3(pt1, 0), Vec3(pt2, 0), Vec4(1.0, 0.0, 0.0, 1.0));
        }
      }
    }

    for (int32_t i = 0; i < m_FacesAlloc.size(); ++i)
    {
      auto const& face = m_FacesAlloc[i];
      if (i == outerFace)
      {
        continue;
      }

      for(uint32_t faceIdx = face.first; faceIdx != face.first + face.second; ++faceIdx)
      {
        uint32_t segIdx = m_FacesSeg[faceIdx];
        auto const& curSeg = m_PolyMesh.edges[segIdx];
        Segmentf fltSeg = {m_PolyMesh.vertices[curSeg.srcVtx].positionf, m_PolyMesh.vertices[curSeg.dstVtx].positionf};
        if (m_VisitedSeg[curSeg.sibling] == outerFace)
        {
          continue;
        }

        Vec2 validDir[3];
        validDir[0] = (fltSeg.m_Ext1 + fltSeg.m_Ext2) * 0.5;
        validDir[1] = fltSeg.m_Ext1;
        validDir[2] = fltSeg.m_Ext2;

        Vec2 point;
        auto interRes = Segmentf::Intersect(Zero<Vec2>(), UnitX<Vec2>() * m_MaxVelocity, validDir[0], validDir[1], point);
        if((interRes & Segmentf::PointOnSegments) == Segmentf::PointOnSegments)
        {
          float curVel = NormalizeAndGetLength(point);
          velocities.Update(point, curVel / m_MaxVelocity);
        }
        else
        {
          for(auto& dir : validDir)
          {
            float speed = NormalizeAndGetLength(dir);
            velocities.Update(dir, speed / m_MaxVelocity);
          }
        }
      }
    }

    bestScore = std::max_element(velocities.curScore, velocities.curScore + 4);
    priority = bestScore - velocities.curScore;
    if(*bestScore > -1.0/Mathf::MaxReal())
    //for (uint32_t priority = 0; priority < 4; ++priority)
    {
      //if (velocities.curScore[priority] > )
      {
        return (m_CasterDesiredDir * velocities.curDir[priority].x 
          + m_PerpDir * velocities.curDir[priority].y) 
          * m_MaxVelocity * velocities.curSpeed[priority];
      }
    }
#endif

    return Zero<Vec2>();

    //if(m_DrawDebug)
    //{
    //  auto drawer = DebugTool::GetDrawer();
    //  
    //  drawer->DrawLine(MathTools::To3DVec(m_CasterPos), MathTools::To3DVec(m_CasterPos + bestDir * selectedVelocity), Vec4(0.0, 0.0, 1.0, 1.0));
    //}
    //
    //return bestDir * selectedVelocity;
    
  }

  void VelocityObstacle::DrawDebug()
  {
    m_DrawDebug = true;
  }
}