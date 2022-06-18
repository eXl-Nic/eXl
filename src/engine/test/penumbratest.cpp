#include <gtest/gtest.h>

#include <core/random.hpp>
#include <engine/pathfinding/velocityobstacle.hpp>
#include <math/polygon_def.hpp>
#include <math/mathtools.hpp>
#include <math/halfedge.hpp>
#include <math/seginter.hpp>

#include <fstream>
//#include <core/utils/filetextreader.hpp>

using namespace eXl;

TEST(Pathfinding, VelocityObstacle)
{
  Random* randGen = Random::CreateDefaultRNG(0);
  VelocityObstacle obs(*randGen);

  obs.Start(nullptr, Zero<Vec2>(), 1.0, UnitX<Vec2>(), 1.0);
  obs.AddPoint(UnitX<Vec2>() * 2.0f, 1.0, Zero<Vec2>());

  Vec2 bestVel = obs.FindBestVelocity(Zero<Vec2>());
  ASSERT_GT(Mathf::Abs(dot(bestVel, UnitY<Vec2>())), 0.8);

  obs.Start(nullptr, Zero<Vec2>(), 1.0, UnitX<Vec2>(), 1.0);
  obs.AddPoint(UnitX<Vec2>() * 2.0f, 1.0, UnitX<Vec2>());

  bestVel = obs.FindBestVelocity(Zero<Vec2>());
  ASSERT_EQ(Mathf::Abs(dot(bestVel,UnitX<Vec2>())), 1.0);

  obs.Start(nullptr, Zero<Vec2>(), 1.0, UnitX<Vec2>(), 1.0);
  obs.AddPoint(UnitX<Vec2>() * 4.0f, 1.0, Zero<Vec2>());

  bestVel = obs.FindBestVelocity(Zero<Vec2>());
  ASSERT_GT(Mathf::Abs(dot(bestVel, UnitX<Vec2>())), 0.75);

  obs.Start(nullptr, Zero<Vec2>(), 1.0, UnitX<Vec2>(), 1.0);
  obs.AddPoint(UnitX<Vec2>() * 4.0f, 1.0, -UnitX<Vec2>());
  obs.AddPoint(UnitX<Vec2>() * 4.0f + UnitY<Vec2>() * 2.0f, 1.0, -UnitX<Vec2>());

  Vec2 bestVel1 = obs.FindBestVelocity(Zero<Vec2>());

  ASSERT_LT(Mathf::Abs(dot(bestVel1, UnitX<Vec2>())), 0.3);
  ASSERT_LT(dot(bestVel1, UnitY<Vec2>()), -0.5);

  obs.Start(nullptr, UnitX<Vec2>() * 4.0f, 1.0, -UnitX<Vec2>(), 1.0);
  obs.AddPoint(Zero<Vec2>(), 1.0, UnitX<Vec2>());
  obs.AddPoint(UnitX<Vec2>() * 4.0f, 1.0, -UnitX<Vec2>());

  Vec2 bestVel2 = obs.FindBestVelocity(Zero<Vec2>());

  eXl_DELETE randGen;
}

TEST(Pathfinding, PooledList)
{
  return;

  PooledList<uint32_t>::Pool nodePool;
  
  PooledList<uint32_t> blah2(nodePool);

  {
    PooledList<uint32_t> blah(nodePool);

    blah.PushBack(0);
    blah.PushBack(1);
    blah.PushBack(2);
    blah.PushBack(3);

    for(auto iter = blah.Begin(); iter != blah.End() ;++iter)
    {
      printf("%i\n", *iter);
    }

    blah.Insert((blah.Begin()++)++, 10);

    for(auto iter = blah.Begin(); iter != blah.End() ;++iter)
    {
      printf("%i\n", *iter);
    }

    blah.Erase((blah.Begin()++)++);
    for(auto iter = blah.Begin(); iter != blah.End() ;++iter)
    {
      printf("%i\n", *iter);
    }

  
    blah2.PushBack(4);
    blah2.PushBack(5);
    blah2.PushBack(6);
    blah2.PushBack(7);

    for(auto iter = blah2.Begin(); iter != blah2.End() ;++iter)
    {
      printf("%i\n", *iter);
    }
  }

  for(auto iter = blah2.Begin(); iter != blah2.End() ;++iter)
  {
    printf("%i\n", *iter);
  }


}

struct SegmentComparator
{
  bool operator ()(Segmenti const& iSeg1, Segmenti const& iSeg2) const
  {
    if(iSeg1.m_Ext1 == iSeg2.m_Ext1)
    {
      return LexicographicCompare(iSeg1.m_Ext2, iSeg2.m_Ext2);
    }
    return LexicographicCompare(iSeg1.m_Ext1, iSeg2.m_Ext1);
  }
};

TEST(Pathfinding, IntersectorTest)
{
  
  Vector<std::pair<uint32_t,Segmenti>> outSegs;
  Set<Segmenti, SegmentComparator> expectedSegs;

  auto checkExpectedSegs = [&]()
  {
    for(auto seg : outSegs)
    {
      auto iter = expectedSegs.find(seg.second);
      ASSERT_TRUE(iter != expectedSegs.end());
      expectedSegs.erase(iter);
    }
    ASSERT_TRUE(expectedSegs.empty());
    outSegs.clear();
  };

  Intersector inter;
  
  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(10000, 0)});
    segs.push_back({Vec2i(0, 10000), Vec2i(10000, 10000)});
    segs.push_back({Vec2i(2000, -2000), Vec2i(8000, 12000)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(2000, -2000), Vec2i(2857, 0)},
      {Vec2i(0, 0), Vec2i(2857, 0)},
      {Vec2i(2857, 0), Vec2i(10000, 0)},
      {Vec2i(2857, 0), Vec2i(7142, 10000)},
      {Vec2i(0, 10000), Vec2i(7142, 10000)},
      {Vec2i(7142, 10000), Vec2i(10000, 10000)},
      {Vec2i(7142, 10000), Vec2i(8000, 12000)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(10000, 0)});
    segs.push_back({Vec2i(0, 0), Vec2i(10000, 10000)});
    segs.push_back({Vec2i(0, 0), Vec2i(8000, 12000)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(segs.begin(), segs.end());

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(-10000, 0), Vec2i(0, 0)});
    segs.push_back({Vec2i(-10000, 10000), Vec2i(0, 0)});
    segs.push_back({Vec2i(-8000, 12000), Vec2i(0, 0)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(segs.begin(), segs.end());

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 300), Vec2i(800, 0)});
    segs.push_back({Vec2i(100, 200), Vec2i(1300, 0)});
    segs.push_back({Vec2i(200, 100), Vec2i(1300, 300)});
    segs.push_back({Vec2i(300, 0), Vec2i(1000, 400)});
    

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(100, 200), Vec2i(400, 150)},
      {Vec2i(0, 300), Vec2i(400, 150)},
      {Vec2i(200, 100), Vec2i(424, 140)},
      {Vec2i(400, 150), Vec2i(424, 140)},

      {Vec2i(424, 140), Vec2i(439, 143)},
      {Vec2i(400, 150), Vec2i(439, 143)},
      {Vec2i(300, 0), Vec2i(498, 113)},
      {Vec2i(424, 140), Vec2i(498, 113)},

      {Vec2i(498, 113), Vec2i(525, 129)},
      {Vec2i(439, 143), Vec2i(525, 129)},
      {Vec2i(525, 129), Vec2i(603, 173)},
      {Vec2i(439, 143), Vec2i(603, 173)},

      {Vec2i(498, 113), Vec2i(800, 0)},
      {Vec2i(603, 173), Vec2i(1000, 400)},
      {Vec2i(525, 129), Vec2i(1300, 0)},
      {Vec2i(603, 173), Vec2i(1300, 300)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(1000, 0)});
    segs.push_back({Vec2i(0, 500), Vec2i(1000, -500)});
    segs.push_back({Vec2i(0, -500), Vec2i(1000, 500)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(0, 0), Vec2i(500, 0)},
      {Vec2i(0, 500), Vec2i(500, 0)},
      {Vec2i(0, -500), Vec2i(500, 0)},
      {Vec2i(500, 0), Vec2i(1000, 0)},
      {Vec2i(500, 0), Vec2i(1000, -500)},
      {Vec2i(500, 0), Vec2i(1000, 500)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(1000, 0)});
    segs.push_back({Vec2i(-250, 0), Vec2i(250, 0)});
    segs.push_back({Vec2i(0, 500), Vec2i(1000, -500)});
    segs.push_back({Vec2i(0, -500), Vec2i(1000, 500)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(-250, 0), Vec2i(250, 0)},
      {Vec2i(250, 0), Vec2i(500, 0)},
      {Vec2i(0, 500), Vec2i(500, 0)},
      {Vec2i(0, -500), Vec2i(500, 0)},
      {Vec2i(500, 0), Vec2i(1000, 0)},
      {Vec2i(500, 0), Vec2i(1000, -500)},
      {Vec2i(500, 0), Vec2i(1000, 500)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(1000, 0)});
    segs.push_back({Vec2i(-250, 0), Vec2i(500, 0)});
    segs.push_back({Vec2i(0, 500), Vec2i(1000, -500)});
    segs.push_back({Vec2i(0, -500), Vec2i(1000, 500)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(-250, 0), Vec2i(500, 0)},
      {Vec2i(0, 500), Vec2i(500, 0)},
      {Vec2i(0, -500), Vec2i(500, 0)},
      {Vec2i(500, 0), Vec2i(1000, 0)},
      {Vec2i(500, 0), Vec2i(1000, -500)},
      {Vec2i(500, 0), Vec2i(1000, 500)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(500, -500), Vec2i(500, 1000)});
    segs.push_back({Vec2i(-250, 0), Vec2i(500, 0)});
    segs.push_back({Vec2i(0, 500), Vec2i(1000, -500)});
    segs.push_back({Vec2i(0, -500), Vec2i(1000, 500)});

    inter.IntersectSegments(segs, outSegs);

    expectedSegs = Set<Segmenti, SegmentComparator>(
    {
      {Vec2i(-250, 0), Vec2i(500, 0)},
      {Vec2i(500, -500), Vec2i(500, 0)},
      {Vec2i(0, 500), Vec2i(500, 0)},
      {Vec2i(0, -500), Vec2i(500, 0)},
      {Vec2i(500, 0), Vec2i(500, 1000)},
      {Vec2i(500, 0), Vec2i(1000, -500)},
      {Vec2i(500, 0), Vec2i(1000, 500)},
    });

    checkExpectedSegs();
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(-9230, 3846), Vec2i(9779, -2088)});
    segs.push_back({Vec2i(-9898, 1418), Vec2i(3974, 9176)});
    segs.push_back({Vec2i(-9137, -4062), Vec2i(3974, 9176)});
    
    inter.IntersectSegments(segs, outSegs);
  }

  {
    Vector<Segmenti> segs;
    segs.push_back({Vec2i(0, 0), Vec2i(27, -999)});
    segs.push_back({Vec2i(0, 0), Vec2i(437, 898)});
    segs.push_back({Vec2i(0, 0), Vec2i(146, 989)});
    segs.push_back({Vec2i(0, 0), Vec2i(-999, -27)});
    segs.push_back({Vec2i(0, 0), Vec2i(-958, 284)});
    segs.push_back({Vec2i(0, 0), Vec2i(-820, -571)});
    segs.push_back({Vec2i(0, 0), Vec2i(-988, -149)});
    segs.push_back({Vec2i(0, 0), Vec2i(-988, -149)});
    segs.push_back({Vec2i(0, 0), Vec2i(-900, 435)});
    segs.push_back({Vec2i(0, 0), Vec2i(-988, -151)});

    inter.IntersectSegments(segs, outSegs);
  }
#if 0
  {
    Vector<Segmenti> segs;

    if (auto reader = FileTextReader::Create("D:\\DebugIntersector_Test.json"))
    {
      JSONUnstreamer unstreamer(reader);
      unstreamer.Begin();

      unstreamer.Read(&segs);

      unstreamer.End();
      eXl_DELETE reader;
    }

    inter.IntersectSegments(segs, outSegs);
  }
#endif
}