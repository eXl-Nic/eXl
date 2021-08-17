#pragma once

#include <engine/enginelib.hpp>
#include <core/containers.hpp>
#include <math/vector2.hpp>
#include <math/segment.hpp>

namespace eXl
{
  class EXL_ENGINE_API Penumbra
  {
  public:

    static Vector2f GetMidSegment(Vector2f const (&iRange)[2]);

    static void UpdateSightRange(Vector2f const (&iRange)[2], Vector2f& oMid, float& oLowLimit);

    static bool IsInSightRange(Vector2f const& iDir, Vector2f const& iMidSeg, float iLowLimit, float iEpsilon = 0.01);

    void Start(Vector2f const& iOrigin, float iRadius, Vector2f const& iDesiredDir, float iLookAhead);

    void AddPoint(Vector2f const& iOrigin, float iRadius);

    void AddSegment(Segmentf const& iSeg, float iRadius);

    void AddBox(Vector2f const& iDimensions, Vector2f const& iOrigin, Vector2f const& iDirection, float iRadius);

    Vector2f FindBestDir(Vector2f const& prevDir);

    Vector<Segmentf> GetAllOpenings();

    void DrawDebug();

  protected:

    void GenTangentsAndSortPoints();

    Vector2f GetWorldDirection(Vector2f const&);

    Vector2f GetWorldPosition(Vector2f const&);

    struct Point
    {
      Vector2f m_RelDir;
      float m_Distance;
      float m_Radius;

      void ComputeTangents();

      Segmentf m_BlockingSegment;

      bool m_Blocking = false;
    };

    Vector2f m_CasterPos;
    Vector2f m_CasterDesiredDir;
    float m_CasterRadius;
    float m_LookAheadDist;
    bool m_DesiredDirValid;
    bool m_DrawDebug;
    bool m_Sorted;

    Vector<Point> m_Obstacles;
    Vector<Vector2f> m_ValidDirs;
    Vector<uint32_t> m_OrderedPoints;

  };
}