
#include <gtest/gtest.h>

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>

using namespace eXl;

void PrintPos(Mat4 const& iMat, ObjectHandle iHandle)
{
  Vec3 pos = iMat[3];
  printf("%i->(%f, %f, %f)\n", iHandle.GetId(), pos.x, pos.y, pos.z);
}

void CheckExpected(UnorderedMap<ObjectHandle, Vec3>& iExpected, Mat4 const& iMat, ObjectHandle iHandle)
{
  //PrintPos(iMat, iHandle);
  auto iter = iExpected.find(iHandle);
  ASSERT_TRUE(iter != iExpected.end());
  ASSERT_TRUE(iter->second == Vec3(iMat[3]));
  iExpected.erase(iter);
}

void DoCheck(UnorderedMap<ObjectHandle, Vec3>& iExpected, Transforms& transforms)
{
  //printf("-------------------\n\n");
  auto checkFunctor = [&iExpected](Mat4 const& iMat, ObjectHandle iHandle)
  {
    CheckExpected(iExpected, iMat, iHandle);
  };

  transforms.IterateOverDirtyTransforms(checkFunctor);
  ASSERT_TRUE(iExpected.empty());
}

TEST(Engine, TransformsTest)
{
  eXl::ComponentManifest dummy;
  World world(dummy);

  Transforms* transforms;

  transforms = world.AddSystem(std::make_unique<Transforms>());

  ObjectHandle objs[]{ world.CreateObject(),
                        world.CreateObject(),
                        world.CreateObject(),
                        world.CreateObject(),
                        world.CreateObject() };

  float pos[] = { 1, 10, 100, 1000, 10000 };

  Mat4 testTrans = Identity<Mat4>();
  for (uint32_t i = 0; i < 5; ++i)
  {
    testTrans[3] = Vec4(UnitX<Vec3>() * pos[i], 1);
    transforms->AddTransform(objs[i]);
    transforms->UpdateTransform(objs[i], testTrans);
  }

  UnorderedMap<ObjectHandle, Vec3> expectedMap;
  expectedMap = 
  {
    {objs[0], UnitX<Vec3>() * pos[0]},
    {objs[1], UnitX<Vec3>() * pos[1]},
    {objs[2], UnitX<Vec3>() * pos[2]},
    {objs[3], UnitX<Vec3>() * pos[3]},
    {objs[4], UnitX<Vec3>() * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  for (uint32_t i = 0; i < 6; ++i)
  {
    transforms->NextFrame();
  }

  transforms->Attach(objs[0], objs[1]);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[0] + UnitX<Vec3>() * pos[1]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Detach(objs[0]);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[0]},
  };

  DoCheck(expectedMap, *transforms);

  for (uint32_t i = 0; i < 6; ++i)
  {
    transforms->NextFrame();
  }

  transforms->Attach(objs[0], objs[2]);
  testTrans[3] = Vec4(UnitX<Vec3>() * pos[2], 1);
  transforms->UpdateTransform(objs[2], testTrans);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[0] + UnitX<Vec3>() * pos[2]},
    {objs[2], UnitX<Vec3>() * pos[2]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Attach(objs[4], objs[2]);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[2] + UnitX<Vec3>() * pos[0]},
    {objs[2], UnitX<Vec3>() * pos[2]},
    {objs[4], UnitX<Vec3>() * pos[2] + UnitX<Vec3>() * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Attach(objs[1], objs[0]);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[2] + UnitX<Vec3>() * pos[0]},
    {objs[1], UnitX<Vec3>() * pos[2] + UnitX<Vec3>() * pos[0] + UnitX<Vec3>() * pos[1]},
    {objs[2], UnitX<Vec3>() * pos[2]},
    {objs[4], UnitX<Vec3>() * pos[2] + UnitX<Vec3>() * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  testTrans[3] = Vec4(UnitX<Vec3>() * pos[3], 1);
  transforms->UpdateTransform(objs[2], testTrans);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[3] + UnitX<Vec3>() * pos[0]},
    {objs[1], UnitX<Vec3>() * pos[3] + UnitX<Vec3>() * pos[0] + UnitX<Vec3>() * pos[1]},
    {objs[2], UnitX<Vec3>() * pos[3]},
    {objs[4], UnitX<Vec3>() * pos[3] + UnitX<Vec3>() * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->DeleteComponent(objs[2]);

  expectedMap =
  {
    {objs[0], UnitX<Vec3>() * pos[0]},
    {objs[1], UnitX<Vec3>() * pos[0] + UnitX<Vec3>() * pos[1]},
    {objs[4], UnitX<Vec3>() * pos[4]},
  };

  DoCheck(expectedMap, *transforms);
}