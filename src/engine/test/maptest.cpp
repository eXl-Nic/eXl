
#include <gtest/gtest.h>

#include <gen/floodfill.hpp>

using namespace eXl;

TEST(Engine, FloodFillTest)
{

  AABB2Di box = AABB2Di::FromMinAndSize(Zero<Vec2i>(), One<Vec2i>() * 8);
  if(0)
  {
    Vector<char> testVecComp =
    { -1, -1, -1, -1, -1, -1, -1, -1,
      -1,  0,  0, -1, -1,  0,  0, -1,
      -1,  0,  0,  0, -1, -1,  0,  0,
      -1, -1, -1,  0, -1, -1, -1,  0,
      -1,  0,  0,  0, -1, -1, -1,  0,
      -1, -1, -1,  0,  0,  0,  0,  0,
       0,  0, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1,
    };


    Vector<uint32_t> out_comps;
    uint32_t numComps = FloodFill::ExtractComponents(testVecComp, box, out_comps);

    ASSERT_EQ(numComps, 2);

    Vector<AABB2DPolygoni> polys;
    FloodFill::MakePolygons(testVecComp, box, [](char iVal) { return iVal == 0; }, polys);

    ASSERT_EQ(polys.size(), 2);
  }

  Vector<bool> testVec0 =
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
  };
  Vector<bool> testVec =
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
  };

  Vector<AABB2DPolygoni> polys;
  FloodFill::MakePolygons(testVec, box, FloodFill::ValidOperator<bool>(), polys);

  ASSERT_EQ(polys.size(), 2);
}