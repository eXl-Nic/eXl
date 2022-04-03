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

namespace eXl
{
  class EXL_ENGINE_API Penumbra
  {
  public:

    static Vec2 GetMidSegment(Vec2 const (&iRange)[2]);

    static void UpdateSightRange(Vec2 const (&iRange)[2], Vec2& oMid, float& oLowLimit);

    static bool IsInSightRange(Vec2 const& iDir, Vec2 const& iMidSeg, float iLowLimit, float iEpsilon = 0.01);

    void Start(Vec2 const& iOrigin, float iRadius, Vec2 const& iDesiredDir, float iLookAhead);

    void AddPoint(Vec2 const& iOrigin, float iRadius);

    void AddSegment(Segmentf const& iSeg, float iRadius);

    void AddBox(Vec2 const& iDimensions, Vec2 const& iOrigin, Vec2 const& iDirection, float iRadius);

    Vec2 FindBestDir(Vec2 const& prevDir);

    Vector<Segmentf> GetAllOpenings();

    void DrawDebug();

  protected:

    void GenTangentsAndSortPoints();

    Vec2 GetWorldDirection(Vec2 const&);

    Vec2 GetWorldPosition(Vec2 const&);

    struct Point
    {
      Vec2 m_RelDir;
      float m_Distance;
      float m_Radius;

      void ComputeTangents();

      Segmentf m_BlockingSegment;

      bool m_Blocking = false;
    };

    Vec2 m_CasterPos;
    Vec2 m_CasterDesiredDir;
    float m_CasterRadius;
    float m_LookAheadDist;
    bool m_DesiredDirValid;
    bool m_DrawDebug;
    bool m_Sorted;

    Vector<Point> m_Obstacles;
    Vector<Vec2> m_ValidDirs;
    Vector<uint32_t> m_OrderedPoints;

  };
}