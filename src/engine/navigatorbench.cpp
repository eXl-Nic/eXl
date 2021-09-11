#include "navigatorbench.hpp"

#include "maptest.hpp"

#include <math/mathtools.hpp>

#include <engine/common/world.hpp>

#include <engine/game/characteranimation.hpp>
#include <engine/game/projectile.hpp>
#include <core/input.hpp>

#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>

#include <engine/map/dungeongraph_res.hpp>
#include <engine/map/dungeongraph_z.hpp>
#include <engine/map/dungeonlayout.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/pathfinding/navigator.hpp>

#include <engine/game/character.hpp>

//#include "mapset.hpp"
#include <engine/map/maptiler.hpp>
#include <engine/map/tilinggroup.hpp>

#define FULLSCALE_TEST
//#define COMPLEX_ROOM

namespace eXl
{

  Vector2f const (&NavigatorBench::GetMovingObstaclesPos())[4]
  {
    static Vector2f const s_MovingObstaclesPos[4] =
    {
      (Vector2f::ONE * 50) + (Vector2f::UNIT_X * 10 + (-Vector2f::UNIT_X - Vector2f::UNIT_Y) * 1) * 4,
      (Vector2f::ONE * 50) + (Vector2f::UNIT_X * 10 + (-Vector2f::UNIT_X + Vector2f::UNIT_Y) * 1) * 4,
      (Vector2f::ONE * 50) + (Vector2f::UNIT_X * 10 + (Vector2f::UNIT_X + Vector2f::UNIT_Y) * 1) * 4,
      (Vector2f::ONE * 50) + (Vector2f::UNIT_X * 10 + (Vector2f::UNIT_X - Vector2f::UNIT_Y) * 1) * 4,
    };

    return s_MovingObstaclesPos;
  }

  Vector2f PickRandomPosInBox(AABB2Df const& iBox, Random& iRand)
  {
    AABB2Df trimmedFace = iBox;
    trimmedFace.m_Data[0] += Vector2f::ONE * 2.0;
    trimmedFace.m_Data[1] -= Vector2f::ONE * 2.0;
    Vector2f size = trimmedFace.GetSize();

    return trimmedFace.m_Data[0] + Vector2f(
      size.X() * (iRand.Generate() % 1000) / 1000.0, 
      size.Y() * (iRand.Generate() % 1000) / 1000.0);
  };

  Vector3f PickRandomPos(NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand)
  {
    uint32_t faceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
    AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
    Vector2f randPos = PickRandomPosInBox(curFace, iRand);
    return Vector3f(randPos.X(), randPos.Y(), 0.0);
  };

  Vector3f PickRandomDest(Vector3f const& iCurPos, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, NavigatorBench::ProbaTable const& iProbaTable)
  {
    auto faceIdx = iNavMesh.FindFace(Vector2f(iCurPos.X(), iCurPos.Y()));
    uint32_t destFaceIdx = faceIdx->m_Face;
    if (iNavMesh.GetFaces(iComponent).size() > 1)
    {
      do
      {
        //destFaceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
        float sample = float(iRand.Generate() % 10000) / 10000;
        auto iter = std::lower_bound(iProbaTable.begin(), iProbaTable.end(), std::make_pair(sample, 0u));
        destFaceIdx = iter == iProbaTable.end() ? iProbaTable.back().second : iter->second;
      } while (destFaceIdx == faceIdx->m_Face);
    }

    AABB2Df destFace = iNavMesh.GetFaces(iComponent)[destFaceIdx].m_Box;

    Vector2f randPos = PickRandomPosInBox(destFace, iRand);
    return Vector3f(randPos.X(), randPos.Y(), 0.0);
  };

  Vector3f NavigatorBench::GetNextDestination(Vector3f const& iCurPos)
  {
    Vector2f const& pos2D = MathTools::As2DVec(iCurPos);

    uint32_t destIdx;
    float minDist = FLT_MAX;

    for (uint32_t posIdx = 0; posIdx < 4; ++posIdx)
    {
      float dist = (GetMovingObstaclesPos()[posIdx] - pos2D).Length();
      if (dist < minDist)
      {
        destIdx = posIdx;
        minDist = dist;
      }
    }
    if (minDist != FLT_MAX)
    {
      return MathTools::To3DVec(GetMovingObstaclesPos()[(destIdx + 1) % 4]);
    }
    return iCurPos;
  };

  NavigatorBench::NavigatorBench()
  {
    m_RandGen = Random::CreateDefaultRNG(Application::GetAppl().GetSeed());
  }

  NavMesh NavigatorBench::BuildMap(World& iWorld, ObjectHandle iMapHandle, Random& iRand)
  {

#if defined(COMPLEX_ROOM)
    DungeonGraph_Z dung;
    dung.Init();
    printf("\nBefore Expand\n");
    dung.PrintGraph();
    for (uint32_t i = 0; i < 20; ++i)
    {
      dung.ApplyRoomRule(iRand);
    }
    printf("\nAfter Expand\n");
    dung.PrintGraph();
#else
    DungeonGraph_Res dung;
    dung.MakeSingleRoom(8);
#endif

#if 0
    DungeonGraph_Res dung;
#if defined(COMPLEX_ROOM)
    dung.InitRes();
    printf("\nBefore Expand\n");
    dung.PrintGraph();
    dung.ExpandResources(iRand);
    printf("\nAfter Expand\n");
    dung.PrintGraph();
#else
    dung.MakeSingleRoom(8);
#endif
#endif

    eXl::LayoutCollection rooms;

#if !defined(COMPLEX_ROOM)
    rooms.push_back(eXl::Layout());
    rooms.back().push_back(eXl::Room());

    rooms.back().back().m_Box = AABB2Di(0, 0, 20, 20);
    rooms.back().back().m_Node = *dung.GetGraph().m_vertices.begin();
#else
    rooms = LayoutGraph(dung, iRand);
#endif

    return CreateMapFromRooms(iWorld, iMapHandle, dung, rooms);
  }

  Vector<ObjectHandle> NavigatorBench::BuildCrossingTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent)
  {
    auto const& faces = iNavMesh.GetFaces(iComponent);
    float curScore = -FLT_MAX;
    uint32_t biggestFace = 0;
    for (uint32_t i = 0; i < faces.size(); ++i)
    {
      auto const& face = faces[i];
      Vector2f size = face.m_Box.GetSize();
      float score = Mathf::Min(size.X(), size.Y()) - Mathf::Abs(size.X() - size.Y());
      if (score > curScore)
      {
        biggestFace = i;
        curScore = score;
      }
    }

    auto const& face = faces[biggestFace];
    Vector2f center = face.m_Box.GetCenter();
    Vector2f size = face.m_Box.GetSize() - (Vector2f::ONE * 4 * 4);
    float radius = Mathf::Min(size.X(), size.Y()) * 0.5;
    float perimeter = radius * 2 * Mathf::PI;

    uint32_t numActors = (perimeter / (1.2 * 4));
    float increment = Mathf::PI * 2 / numActors;

    Vector<ObjectHandle> autonomousAgents;
    auto& transforms = *iWorld.GetSystem<Transforms>();
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

    ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
    ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
    ioBaseDesc.size = 1.0;
    ioBaseDesc.maxSpeed = 10.0;

    for (unsigned int i = 0; i < numActors / 2; ++i)
    {
      Vector3f curPos(Mathf::Cos(i * increment) * radius + center.X(), Mathf::Sin(i * increment) * radius + center.Y(), 0.0);
      Vector3f destPos(Mathf::Cos(i * increment + Mathf::PI) * radius + center.X(), Mathf::Sin(i * increment + Mathf::PI) * radius + center.Y(), 0.0);

      ObjectHandle truc = CharacterSystem::Build(iWorld, curPos, ioBaseDesc);
      navigator.SetDestination(truc, destPos);

      ObjectHandle truc2 = CharacterSystem::Build(iWorld, destPos, ioBaseDesc);
      navigator.SetDestination(truc2, curPos);

      autonomousAgents.push_back(truc);
      autonomousAgents.push_back(truc2);
    }
    return autonomousAgents;
  }

  Vector<ObjectHandle> NavigatorBench::BuildFullScaleTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, ProbaTable& ioProbaTable)
  {
    Vector<ObjectHandle> autonomousAgents;
    auto& transforms = *iWorld.GetSystem<Transforms>();
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

    ioBaseDesc.kind = CharacterSystem::PhysicKind::Simulated;
    ioBaseDesc.controlKind = CharacterSystem::ControlKind::Navigation;
    ioBaseDesc.size = 1.0;
    ioBaseDesc.maxSpeed = 10.0;

    ioProbaTable.clear();
    float totalArea = 0;
    for (uint32_t i = 0; i< iNavMesh.GetFaces(iComponent).size(); ++i)
    {
      auto const& Face = iNavMesh.GetFaces(iComponent)[i];
      Vector2f size = Face.m_Box.GetSize();
      float area = size.X() * size.Y();
      totalArea += area;
      ioProbaTable.push_back(std::make_pair(area, i));
    }

    std::sort(ioProbaTable.begin(), ioProbaTable.end());

    float accum = 0;
    for (auto& entry : ioProbaTable)
    {
      if (entry.first < 36)
      {
        totalArea -= entry.first;
        entry.first = 0;
        continue;
      }
      float curProba = accum / totalArea;
      accum += entry.first;
      entry.first = curProba;
    }

    for (unsigned int i = 0; i < iNumNavAgents; ++i)
    {
      //auto curPos = PickRandomPos(iNavMesh, iComponent, iRand);

      uint32_t faceIdx = iRand.Generate() % iNavMesh.GetFaces(iComponent).size();
      AABB2Df curFace = iNavMesh.GetFaces(iComponent)[faceIdx].m_Box;
      Vector2f randPos = PickRandomPosInBox(curFace, iRand);
      auto curPos = Vector3f(randPos.X(), randPos.Y(), 0.0);

      auto destPos = PickRandomDest(curPos, iNavMesh, iComponent, iRand, ioProbaTable);
      
      ObjectHandle truc = CharacterSystem::Build(iWorld, curPos, ioBaseDesc);
      navigator.SetDestination(truc, destPos);

      //ObjectHandle sword = world.CreateObject();
      //{
      //  Matrix4f mat;
      //  mat.MakeIdentity();
      //  MathTools::GetPosition2D(mat) = Vector2f::UNIT_X * 2;
      //  transforms.AddTransform(sword, &mat);
      //  GfxSpriteComponent& spriteComp = gfxSys.CreateSpriteComponent(sword);
      //  spriteComp.SetSize((Vector2f::ONE * size16 * 0.5) / 24);
      //  spriteComp.SetTileset(swordSet);
      //  spriteComp.SetTileName(TileName("idle_down"));
      //}

      //transforms.Attach(sword, truc);
      autonomousAgents.push_back(truc);
    }
    return autonomousAgents;
  }

  void NavigatorBench::Init(World& world)
  {

    auto& transforms = *world.GetSystem<Transforms>();
    auto& gfxSys = *world.GetSystem<GfxSystem>();
    auto& phSys = *world.GetSystem<PhysicsSystem>();
    auto& charSys = *world.GetSystem<CharacterSystem>();
    auto& navigator = *world.GetSystem<NavigatorSystem>();

    
#ifdef EXL_IMAGESTREAMER_ENABLED
    unsigned int const spriteSize = 16;

    ResourceHandle<Tileset> tilesetHandle;
    {
      Resource::UUID id({ 2191849209, 2286785320, 240566463, 373114736 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
    }
    Tileset const* charSet = tilesetHandle.Get();
    {
      Resource::UUID id({ 3147825353,893968785, 3414459273, 2423460864 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
    }
    Tileset const* swordSet = tilesetHandle.Get();
    {
      Resource::UUID id({ 1077917011, 3024235796, 2526589356, 1370983528 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
    }
    Tileset const* crateTileset = tilesetHandle.Get();
    {
      Resource::UUID id({ 3495140576, 2538432508, 3618018991, 904438486 });

      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_FireballTileset = tilesetHandle.Get();
    }

    //String AppPath = GetAppPath();
    //String DataPath = AppPath.substr(0, AppPath.rfind("/"));
    //String SpritePath = DataPath + "/../../";

#else
    //Image* spriteGuy = eXl_NEW Image(DummySprites::BitmapToImage(DummySprites::dummyChar, Image::Size(8, 8)));
    //
    //Image::Size numTiles(1, 1);
    //Vector2f texStep = Vector2f::ONE;

    Tileset const* charSet = nullptr;
#endif


    m_MapHandle = world.CreateObject();
    m_NavMesh = BuildMap(world, m_MapHandle, *m_RandGen);
    if (m_NavMesh.GetComponents().empty())
    {
      return;
    }
    
    navigator.SetNavMesh(m_NavMesh);

    m_Component = m_RandGen->Generate() % m_NavMesh.GetComponents().size();
    auto& component = m_NavMesh.GetComponents()[m_Component];

    uint32_t mainRoomFaceIdx = m_RandGen->Generate() % m_NavMesh.GetFaces(m_Component).size();
    AABB2Df mainRoom = m_NavMesh.GetFaces(m_Component)[mainRoomFaceIdx].m_Box;

    Vector2f roomCenter = mainRoom.GetCenter();

    Vector<ObjectHandle> autonomousAgents;

#if !defined(FULLSCALE_TEST)

    Vector2f obstaclePos[] =
    {
      roomCenter,
      roomCenter + Vector2f::UNIT_Y * size16,
      roomCenter + Vector2f::UNIT_Y * 1.5 * size16,
      roomCenter + Vector2f::UNIT_Y * 2.5 * size16,
    };

    for (auto const& pos : obstaclePos)
    {
      ObjectHandle obstacleHandle = world.CreateObject();
      {
        Matrix4f obstacleTrans = Matrix4f::IDENTITY;
        MathTools::GetPosition2D(obstacleTrans) = pos;
        transforms.AddTransform(obstacleHandle, &obstacleTrans);

        PhysicInitData desc;
        desc.SetFlags(PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation | PhysicFlags::Static);
        desc.AddSphere(size16 * 0.25);

        phSys.CreateComponent(obstacleHandle, desc);

        GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(obstacleHandle);
        gfxComp.SetSize((Vector2f::ONE * size16 * 0.5) / 32);
        gfxComp.SetOffset(((Vector2f::UNIT_Y * size16 * 0.5) / DunAtk::s_WorldToPixel));
        gfxComp.SetTileset(crateTileset);
        gfxComp.SetTileName(TileName("Crate"));
        
        navigator.AddObstacle(obstacleHandle, size16 * 0.25);

        phSys.GetNeighborhoodExtraction().AddObject(obstacleHandle, size16 * 0.25, false);
      }
    }

    for (auto const& pos : GetMovingObstaclesPos())
    {
      ObjectHandle obstacleHandle = world.CreateObject();
      {
        Matrix4f obstacleTrans = Matrix4f::IDENTITY;
        MathTools::GetPosition2D(obstacleTrans) = pos;
        transforms.AddTransform(obstacleHandle, &obstacleTrans);

        PhysicInitData desc;
        desc.SetFlags(PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation);
        desc.AddSphere(size16 * 0.25);

        phSys.CreateComponent(obstacleHandle, desc);

        GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(obstacleHandle);
        gfxComp.SetSize((Vector2f::ONE * size16 * 0.5) / 32);
        gfxComp.SetOffset(((Vector2f::UNIT_Y * size16 * 0.5) / DunAtk::s_WorldToPixel));
        gfxComp.SetTileset(crateTileset);
        gfxComp.SetTileName(TileName("Crate"));

        navigator.AddNavigator(obstacleHandle, size16 * 0.25, 5.0, true);

        phSys.GetNeighborhoodExtraction().AddObject(obstacleHandle, size16 * 0.25, true);

        navigator.SetDestination(obstacleHandle, GetNextDestination(MathTools::GetPosition(obstacleTrans)));
      }
    }

    m_NavHandle = world.CreateObject();
    {
      Matrix4f navTrans = Matrix4f::IDENTITY;
      MathTools::GetPosition2D(navTrans) = roomCenter - Vector2f::UNIT_X * 10;
      transforms.AddTransform(m_NavHandle, &navTrans);

      PhysicInitData desc;
      desc.SetFlags(PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation);
      desc.AddSphere(size16 * 0.25);

      phSys.CreateComponent(m_NavHandle, desc);

      GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(m_NavHandle);
      gfxComp.SetSize((Vector2f::ONE * size16 * 0.825) / 24);
      gfxComp.SetTileset(charSet);
      gfxComp.SetOffset(Vector2f(0.0, size16 * 0.825 * 0.25));
      gfxComp.SetTileName(TileName("Idle_Left"));

      navigator.AddNavigator(m_NavHandle, size16 * 0.25, 10.0);

      phSys.GetNeighborhoodExtraction().AddObject(m_NavHandle, size16 * 0.25, true);
    }

#endif

#if defined(FULLSCALE_TEST)
#ifdef _DEBUG
    const uint32_t numNavAgents = 100;
#else
    const uint32_t numNavAgents = 1000 ;
#endif
    static CharacterAnimation s_DefaultAnim;
    s_DefaultAnim.Register(world);
    CharacterSystem::Desc charDesc;
    charDesc.animation = &s_DefaultAnim;
    autonomousAgents = BuildFullScaleTest(world, charDesc, numNavAgents, m_NavMesh, m_Component, *m_RandGen, m_ProbaTable);
    for(auto agent : autonomousAgents)
    {
      GfxSpriteComponent& spriteComp = gfxSys.CreateSpriteComponent(agent);
      spriteComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
      spriteComp.SetOffset(Vector2f::UNIT_Y * 0.5);
      spriteComp.SetTileset(charSet);  
      charSys.AddCharacter(agent, charDesc);
    }
    world.AddTick(World::PostPhysics, [this](World& iWorld, float iDelta) { Step(iWorld, iDelta); });
#else
    const uint32_t numNavAgents = 0;
#endif

    world.AddTick(World::FrameStart, [this](World& iWorld, float iDelta)
    {
      ProcessInputs(iWorld);
      Step(iWorld, iDelta);
    });
    
    world.AddTimer(0.75, true, [this](World& iWorld)
    {
      Vector3f orig(1.0, 1.0, 0.0);
      orig *= 130;
      Vector3f dir(-1.0, 0.0, 0.0);
      CreateProjectile(iWorld, orig, dir);
    });
  }

  ObjectHandle NavigatorBench::CreateProjectile(World& iWorld, Vector3f const& iPos, Vector3f const& iDir)
  {
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& navigator = *iWorld.GetSystem<NavigatorSystem>();
    auto& projectiles = *iWorld.GetSystem<ProjectileSystem>();

    ProjectileSystem::Desc fireballDesc;
    fireballDesc.kind = ProjectileSystem::PhysicKind::Kinematic;
    fireballDesc.size = 1.0;
    fireballDesc.rotateSprite = true;
    fireballDesc.registerNav = true;

    ObjectHandle fireball = ProjectileSystem::Build(iWorld, iPos, fireballDesc);
    {
      GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(fireball);
      gfxComp.SetSize((Vector2f::ONE) / DunAtk::s_WorldToPixel);
      gfxComp.SetOffset(((-Vector2f::UNIT_X) / DunAtk::s_WorldToPixel));
      gfxComp.SetTileset(m_FireballTileset);
      gfxComp.SetTileName(TileName("Right"));
      gfxComp.SetRotateSprite(true);

      projectiles.AddProjectile(fireball, fireballDesc, iDir * 20);
    }

    iWorld.AddTimer(10.0, false, [fireball](World& iWorld)
    {
      iWorld.DeleteObject(fireball);
    });

    return fireball;
  }

  void NavigatorBench::ProcessInputs(World& iWorld)
  {
    Engine_Application& app = Engine_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();

    for (auto move : iInputs.m_MouseMoveEvts)
    {
      m_MousePos = Vector2i(move.absX, move.absY);
    }

    for (auto evt : iInputs.m_MouseEvts)
    {
      if (evt.button == MouseButton::Left && evt.pressed == false)
      {
        GfxSystem& gfxSys = *iWorld.GetSystem<GfxSystem>();

        Vector3f worldPos;
        Vector3f viewDir;
        gfxSys.ScreenToWorld(m_MousePos, worldPos, viewDir);

        worldPos.Z() = 0;
        SetNavDest(iWorld, worldPos);
      }
    }

    GetCamera().ProcessInputs(iWorld, iInputs);
  }

  void NavigatorBench::SetNavDest(World& iWorld, Vector3f const& iDest)
  {
    if(m_NavHandle.IsAssigned())
    {
      auto& navigator = *iWorld.GetSystem<NavigatorSystem>();

      navigator.SetDestination(m_NavHandle, iDest);
    }
  }

  void NavigatorBench::StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, ProbaTable const& iProbaTable)
  {
    auto& transforms = *world.GetSystem<Transforms>();
    auto& navigator = *world.GetSystem<NavigatorSystem>();
    for (auto evt : navigator.DispatchEvents())
    {
      Matrix4f transform = transforms.GetWorldTransform(evt.m_Object);
      Vector3f const& curPos = *reinterpret_cast<Vector3f*>(transform.m_Data + 12);
      navigator.SetDestination(evt.m_Object, PickRandomDest(curPos, iNavMesh, iComponent, iRand, iProbaTable));
    }
  }

  void NavigatorBench::Step(World& world, float iDelta)
  {
    auto& transforms = *world.GetSystem<Transforms>();
    auto& navigator = *world.GetSystem<NavigatorSystem>();

#if defined(FULLSCALE_TEST)
    StepFullScaleTest(world, iDelta, m_NavMesh, m_Component, *m_RandGen, m_ProbaTable);
#else
    {
      for(auto evt : navigator.DispatchEvents())
      {
        if(evt.m_Object != m_NavHandle)
        {
          Matrix4f transform = transforms.GetWorldTransform(evt.m_Object);
          Vector3f const& curPos = MathTools::GetPosition(transform);
          navigator.SetDestination(evt.m_Object, GetNextDestination(curPos));
        }
      }
    }
#endif
  }
  
}