
#include <gtest/gtest.h>

#include <engine/gfx/tileset.hpp>
#include <core/corelib.hpp>
#include <core/plugin.hpp>

#include <core/resource/resourcemanager.hpp>

using namespace eXl;
#if 0
TEST(DunAtk, Tileset)
{
  Tileset* newSet = Tileset::Create("D:\\TestDir", "Cloud_Strife");
  boost::optional<ImageName> img = newSet->ImageNameFromImagePath("D:\\TestDir\\Cloud_Strife.png");

  ASSERT_TRUE(!(!img));

  Tile testTile;
  testTile.m_ImageName = *img;
  testTile.m_AnimType = AnimationType::Loop;
  testTile.m_FrameDuration = 0.2;
  testTile.m_Size = Vector2i::ONE * 16;
  testTile.m_Frames.push_back(Vector2i::ZERO);
  testTile.m_Frames.push_back(UnitX<Vector2i>() * 16);

  newSet->AddTile(TileName("Tile1"), testTile);

  testTile.m_Frames.clear();

  testTile.m_AnimType = AnimationType::None;
  testTile.m_FrameDuration = 0.4;
  testTile.m_Size = Vector2i::ONE * 32;
  testTile.m_Frames.push_back(Vector2i::ZERO);
  testTile.m_Frames.push_back(UnitY<Vector2i>() * 32);

  newSet->AddTile(TileName("Tile2"), testTile);

  //Path folder = "D:\\eXlProject\\data";
  //Path file = folder / "TestTile.eXlAsset";

  //ResourceManager::SaveTo(newSet, file);
  ResourceManager::Save(newSet);

  Path file = ResourceManager::GetPath(newSet->GetHeader().m_ResourceId);
  Path folder = file.parent_path();

  ResourceHandle<Tileset> handle;
  handle.Set(newSet);
  handle.ClearPtr();

  newSet = nullptr;

  ResourceManager::UnloadUnusedResources();
  ResourceManager::Reset();
  ResourceManager::BootstrapDirectory(folder, true);
  //newSet = ResourceManager::Load<Tileset>("D:\\TestTile.eXlAsset");

  handle.Load();

  eXl::Tileset const* loadedSet = handle.Get();
  ASSERT_NE(loadedSet, nullptr);

  Filesystem::remove(file);
}
#endif