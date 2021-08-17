
#include <gtest/gtest.h>

#include <gen/floodfill.hpp>

using namespace eXl;

TEST(DunAtk, FloodFillTest)
{

  AABB2Di box(Vector2i::ZERO, Vector2i::ONE * 8);
  {
    Vector<char> testVec =
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
    uint32_t numComps = FloodFill::ExtractComponents(testVec, box, out_comps);

    ASSERT_EQ(numComps, 2);

    Vector<AABB2DPolygoni> polys;
    FloodFill::MakePolygons(testVec, box, [](char iVal) { return iVal == 0; }, polys);

    ASSERT_EQ(polys.size(), 2);
  }

  Vector<bool> testVec =
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 0, 1, 1, 1,
    0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 1, 1,
    0, 1, 1, 1, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 0,
  };

  Vector<AABB2DPolygoni> polys;
  FloodFill::MakePolygons(testVec, box, FloodFill::ValidOperator<bool>(), polys);

  ASSERT_EQ(polys.size(), 2);
}