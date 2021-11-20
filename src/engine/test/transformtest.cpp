
#include <gtest/gtest.h>

#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <math/mathtools.hpp>

using namespace eXl;

void PrintPos(Matrix4f const& iMat, ObjectHandle iHandle)
{
  Vector3f pos  = MathTools::GetPosition(iMat);
  printf("%i->(%f, %f, %f)\n", iHandle.GetId(), pos.X(), pos.Y(), pos.Z());
}

void CheckExpected(UnorderedMap<ObjectHandle, Vector3f>& iExpected, Matrix4f const& iMat, ObjectHandle iHandle)
{
  //PrintPos(iMat, iHandle);
  auto iter = iExpected.find(iHandle);
  ASSERT_TRUE(iter != iExpected.end());
  ASSERT_TRUE(iter->second == MathTools::GetPosition(iMat));
  iExpected.erase(iter);
}

void DoCheck(UnorderedMap<ObjectHandle, Vector3f>& iExpected, Transforms& transforms)
{
  //printf("-------------------\n\n");
  auto checkFunctor = [&iExpected](Matrix4f const& iMat, ObjectHandle iHandle)
  {
    CheckExpected(iExpected, iMat, iHandle);
  };

  transforms.IterateOverDirtyTransforms(checkFunctor);
  ASSERT_TRUE(iExpected.empty());
}

TEST(DunAtk, TransformsTest)
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

  Matrix4f testTrans;
  for (uint32_t i = 0; i < 5; ++i)
  {
    MathTools::GetPosition(testTrans) = Vector3f::UNIT_X * pos[i];
    transforms->AddTransform(objs[i], nullptr);
    transforms->UpdateTransform(objs[i], testTrans);
  }

  UnorderedMap<ObjectHandle, Vector3f> expectedMap;
  expectedMap = 
  {
    {objs[0], Vector3f::UNIT_X * pos[0]},
    {objs[1], Vector3f::UNIT_X * pos[1]},
    {objs[2], Vector3f::UNIT_X * pos[2]},
    {objs[3], Vector3f::UNIT_X * pos[3]},
    {objs[4], Vector3f::UNIT_X * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  for (uint32_t i = 0; i < 6; ++i)
  {
    transforms->NextFrame();
  }

  transforms->Attach(objs[0], objs[1]);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[0] + Vector3f::UNIT_X * pos[1]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Detach(objs[0]);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[0]},
  };

  DoCheck(expectedMap, *transforms);

  for (uint32_t i = 0; i < 6; ++i)
  {
    transforms->NextFrame();
  }

  transforms->Attach(objs[0], objs[2]);
  MathTools::GetPosition(testTrans) = Vector3f::UNIT_X * pos[2];
  transforms->UpdateTransform(objs[2], testTrans);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[0] + Vector3f::UNIT_X * pos[2]},
    {objs[2], Vector3f::UNIT_X * pos[2]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Attach(objs[4], objs[2]);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[2] + Vector3f::UNIT_X * pos[0]},
    {objs[2], Vector3f::UNIT_X * pos[2]},
    {objs[4], Vector3f::UNIT_X * pos[2] + Vector3f::UNIT_X * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->Attach(objs[1], objs[0]);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[2] + Vector3f::UNIT_X * pos[0]},
    {objs[1], Vector3f::UNIT_X * pos[2] + Vector3f::UNIT_X * pos[0] + Vector3f::UNIT_X * pos[1]},
    {objs[2], Vector3f::UNIT_X * pos[2]},
    {objs[4], Vector3f::UNIT_X * pos[2] + Vector3f::UNIT_X * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  MathTools::GetPosition(testTrans) = Vector3f::UNIT_X * pos[3];
  transforms->UpdateTransform(objs[2], testTrans);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[3] + Vector3f::UNIT_X * pos[0]},
    {objs[1], Vector3f::UNIT_X * pos[3] + Vector3f::UNIT_X * pos[0] + Vector3f::UNIT_X * pos[1]},
    {objs[2], Vector3f::UNIT_X * pos[3]},
    {objs[4], Vector3f::UNIT_X * pos[3] + Vector3f::UNIT_X * pos[4]},
  };

  DoCheck(expectedMap, *transforms);

  transforms->DeleteComponent(objs[2]);

  expectedMap =
  {
    {objs[0], Vector3f::UNIT_X * pos[0]},
    {objs[1], Vector3f::UNIT_X * pos[0] + Vector3f::UNIT_X * pos[1]},
    {objs[4], Vector3f::UNIT_X * pos[4]},
  };

  DoCheck(expectedMap, *transforms);
}