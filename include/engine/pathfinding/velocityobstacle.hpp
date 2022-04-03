/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/containers.hpp>
#include <math/segment.hpp>
#include <math/seginter.hpp>
#include <math/halfedge.hpp>

namespace eXl
{
  class Random;

  class EXL_ENGINE_API VelocityObstacle
  {
  public:

    VelocityObstacle(Random& iRand);

    void Start(void* iActor, Vec2 const& iOrigin, float iRadius, Vec2 const& iDesiredDir, float iMaxVelocity);

    void AddPoint(Vec2 const& iOrigin, float iRadius, Vec2 const& iVelocity);

    void AddSegment(Segmentf const& iSeg, float iRadius);

    Vec2 FindBestVelocity(Vec2 const& iPrevDir);

    void DrawDebug();

  protected:

    struct BestVelocity
    {
      BestVelocity(float iDirMult);

      float ComputeScore(Vec2 const& iOptimalDir, Vec2 const& iCandidateDir, float iCandidateSpeed);
      void Update(Vec2 const& iCandidateDir, float iCandidateSpeed);

      float curScore[4];
      Vec2 curDir[4];
      float curSpeed[4];
      float dirMult;
    };

    void AddObstacle(Vec2 const& iOrig, Vec2 const (&iDirs)[2], float iDistance);

    struct Obstacle
    {
      Vec2 m_Origin;

      float m_Velocity;
      float m_Distance;
      
      Segmentf m_Segs[2];
    };

    Random& m_Rand;
    Vec2 m_CasterPos;
    Vec2 m_CasterDesiredDir;
    Vec2 m_PerpDir;
    float m_CasterRadius;
    float m_MaxVelocity;
    float m_MaxVelocitySq;
    bool m_DrawDebug;
    bool m_PureAvoidance;

    struct Arc
    {
      void Complement();

      Vec2 m_MidVec;
      float m_LowLimit;
    };

    Vector<Arc> m_CircleArcs;
    bool m_CircleCut;
    bool m_WholeCircleCut;

    Vector<Obstacle> m_Obstacles;
    Vector<Segmenti> m_ObstacleSegs;
    Vector<bool> m_Checked;
    Vector<Vec2> m_RandPts;
    Vector<float> m_RandPtsScore;
    Vector<std::pair<uint32_t, Segmenti>> m_HalfSeg;
    Vector<std::pair<uint32_t, Segmenti>> m_HalfSegRev;
    Vector<uint32_t> m_OrderedPoints;
    Intersector m_Inter;
    Intersector::Parameters m_InterParams;
    PolyMesh m_PolyMesh;
    Vector<int> m_VisitedSeg;
    Vector<uint32_t> m_FacesSeg;
    Vector<std::pair<uint32_t, uint32_t>> m_FacesAlloc;
    AABB2Df m_ObstaclesBox;

    void* m_Actor;
  };
}