#include "abilityroom.hpp"

#include "maptest.hpp"

#include <core/base/input.hpp>

#include <math/mathtools.hpp>

#include <dunatk/common/animation.hpp>
#include <dunatk/common/world.hpp>

#include <dunatk/gfx/gfxcomponent.hpp>
#include <dunatk/gfx/gfxsystem.hpp>

#include <dunatk/map/dungeongraph_res.hpp>
#include <dunatk/map/dungeonlayout.hpp>
#include <dunatk/map/maptiler.hpp>

#include <dunatk/physics/physicsys.hpp>
#include <dunatk/physics/physiccomponent.hpp>
#include <dunatk/pathfinding/navmesh.hpp>
#include <dunatk/pathfinding/navigator.hpp>

#include <dunatk/game/character.hpp>
#include <dunatk/game/characteranimation.hpp>
#include <dunatk/game/ability.hpp>
#include <dunatk/game/projectile.hpp>
#include <dunatk/game/grabability.hpp>
#include <dunatk/game/pickability.hpp>
#include <dunatk/game/walkability.hpp>
#include <dunatk/game/throwability.hpp>
#include <dunatk/game/swordability.hpp>

#include <dunatk/script/luascriptsystem.hpp>

#include <core/base/corelib.hpp>

#include <dunatk/net/network.hpp>
#include <imgui.h>

#include "navigatorbench.hpp"

#define FULLSCALE_TEST

namespace eXl
{
  static CharacterAnimation s_DefaultAnim;

  class NetworkPanel : public MenuManager::Panel
  {
  public:
    NetworkPanel(World& iWorld, AbilityRoom& iScenario)
      : m_World(iWorld)
      , m_Scenario(iScenario)
    {

    }

  protected:
    void Display() override
    {
      char buff[256] = { 0 };
      strcpy(buff, m_URL.c_str());
      if (ImGui::InputText("URL", buff, sizeof(buff)))
      {
        m_URL = buff;
      }

      if (ImGui::Button("StartServer"))
      {
        m_Scenario.StartServer(m_World);
      }

      if (ImGui::Button("StartClient"))
      {
        m_Scenario.StartClient(m_World, m_URL);
      }
    }
    String m_URL = String("127.0.0.1");
    World& m_World;
    AbilityRoom& m_Scenario;
  };
  
  //AbilityName contactDamageAbilityName("ContactDamage");
  //EffectName contactDamageEffectName("ContactDamage");
  ////PropertyName healthPropertyName("Health");
  //GameTagName invulnerableTagName("Invulnerable");
  //GameTagName fireballTag("Fireball");

  //class ContactDamageAbility : public AbilityDesc
  //{
  //  DECLARE_RTTI(ContactDamageAbility, AbilityDesc);
  //public:
  //  ContactDamageAbility()
  //    : AbilityDesc(contactDamageAbilityName)
  //  {}
  //  
  //  AbilityUseState Use(AbilityStateHandle, ObjectHandle iTarget) override;
  //
  //  AbilityState m_StateBuffer;
  //};
  //IMPLEMENT_RTTI(ContactDamageAbility);
  //
  //struct ContactDamageEffectState : public EffectState
  //{
  //  float damageAmount;
  //};
  //
  //class ContactDamageEffect : public EffectDescT<ContactDamageEffectState>
  //{
  //  DECLARE_RTTI(ContactDamageEffect, EffectDesc)
  //public:
  //  ContactDamageEffect()
  //    : EffectDescT(contactDamageEffectName)
  //  {}
  //  virtual void Apply(EffectHandle) override;
  //};
  //
  //IMPLEMENT_RTTI(ContactDamageEffect);
  //
  //AbilityUseState ContactDamageAbility::Use(AbilityStateHandle iAbility, ObjectHandle iTarget)
  //{
  //  m_System->CreateEffect<ContactDamageEffect>(m_StateBuffer.m_Target, contactDamageEffectName, [](ObjectHandle iTarget, ContactDamageEffect& iEffect)
  //  {
  //    EffectHandle handle = iEffect.Create(iTarget);
  //    auto& state = iEffect.Get(handle);
  //    state.m_Target = iTarget;
  //    state.damageAmount = 10;
  //
  //    return handle;
  //  });
  //  return AbilityUseState::Using;
  //}
  //
  //void ContactDamageEffect::Apply(EffectHandle iToApply)
  //{
  //  ContactDamageEffectState& state = Get(iToApply);
  //  if (!m_System->HasTag(state.m_Target, invulnerableTagName))
  //  {
  //    //boost::optional<float> health = m_System->GetProperty(state.m_Target, healthPropertyName);
  //    //if (health)
  //    //{
  //    //  m_System->SetProperty(state.m_Target, healthPropertyName, *health - state.damageAmount);
  //    //}
  //  }
  //  m_System->RemoveEffect(state.m_Target, m_Name, iToApply);
  //}
  
  AbilityRoom::AbilityRoom(Random* iRand)
  {
    m_RandGen = iRand;
  }

  void AbilityRoom::StartServer(World& iWorld)
  {
    m_CurMode = Network::NetRole::Server;
    Network::OnNewPlayer = [this, &iWorld]()
    {
      return SpawnCharacter(iWorld, Network::NetRole::Server);
    };

    Network::OnClientCommand = [this, &iWorld](ObjectHandle iObject, Network::ClientInputData const& iData)
    {
      Network::ClientData update;

      auto& transforms = *iWorld.GetSystem<Transforms>();
      auto& charSys = *iWorld.GetSystem<CharacterSystem>();

      Matrix4f const& trans = transforms.GetWorldTransform(iObject);

      if (iData.m_Moving)
      {
        charSys.SetCurDir(iObject, iData.m_Dir);
        charSys.SetSpeed(iObject, 10.0);
      }
      else
      {
        charSys.SetSpeed(iObject, 0.0);
      }

      update.m_Moving = iData.m_Moving;
      update.m_Pos = MathTools::GetPosition(trans);
      update.m_Dir = *reinterpret_cast<Vector3f const*>(trans.m_Data);

      return update;
    };

    if (Network::Connect(Network::NetRole::Server, "127.0.0.1", 7777))
    {
      iWorld.AddTick(World::FrameStart, [](World& iWorld, float iDelta)
      {
        Network::Tick(iDelta);
      });

      iWorld.AddTick(World::PostPhysics, [](World& iWorld, float iDelta)
      {
        Vector<Network::MovedObject> objectsUpdate;
        Transforms& trans = *iWorld.GetSystem<Transforms>();
        auto& charSys = *iWorld.GetSystem<CharacterSystem>();
        trans.IterateOverDirtyTransforms([&charSys, &objectsUpdate](Matrix4f const& iTrans, ObjectHandle iObject)
        {
          Network::MovedObject update;
          update.object = iObject;
          update.data.m_Moving = (charSys.GetCurrentState(iObject) & (uint32_t)CharacterSystem::CharStateFlags::Walking) != 0;
          update.data.m_Dir = *reinterpret_cast<Vector3f const*>(iTrans.m_Data);
          update.data.m_Pos = *reinterpret_cast<Vector3f const*>(iTrans.m_Data + 12);
          objectsUpdate.push_back(update);

        });

        Network::UpdateObjects(objectsUpdate);
      });
    }
  }

  void AbilityRoom::StartClient(World& iWorld, String const& iURL)
  {
    m_CurMode = Network::NetRole::Client;
    Network::OnNewObjectReceived = [this, &iWorld](Network::ClientData const& )
    {
      return SpawnCharacter(iWorld, Network::NetRole::Client);
    };

    Network::OnPlayerAssigned = [this, &iWorld](ObjectHandle iPlayer)
    {
      m_MainChar = iPlayer;

      auto& transforms = *iWorld.GetSystem<Transforms>();
      transforms.Attach(GetCamera().cameraObj, m_MainChar, Transforms::Position);
    };

    if (Network::Connect(Network::NetRole::Client, iURL, 7777))
    {
      iWorld.AddTick(World::FrameStart, [](World& iWorld, float iDelta)
      {
        Network::Tick(iDelta);
      });

      iWorld.AddTick(World::PostPhysics, [](World& iWorld, float iDelta)
      {
        auto& transforms = *iWorld.GetSystem<Transforms>();
        auto& charSys = *iWorld.GetSystem<CharacterSystem>();

        Vector<Network::MovedObject> const& updates = Network::GetMovedObjects();
        for (auto const& update : updates)
        {
          Matrix4f newTrans;
          newTrans.MakeIdentity();
          MathTools::GetPosition(newTrans) = update.data.m_Pos;
          Vector3f& dirX = *reinterpret_cast<Vector3f*>(newTrans.m_Data + 0);
          Vector3f& dirY = *reinterpret_cast<Vector3f*>(newTrans.m_Data + 4);
          Vector3f& dirZ = *reinterpret_cast<Vector3f*>(newTrans.m_Data + 8);

          dirX = update.data.m_Dir;
          dirZ = Vector3f::UNIT_Z;
          dirY = dirZ.Cross(dirX);
          dirY.Normalize();

          transforms.UpdateTransform(update.object, newTrans);
          charSys.SetState(update.object, CharacterSystem::GetStateFromDir(update.data.m_Dir, update.data.m_Moving));
        }
      });
    }
  }

  void AbilityRoom::StartLocal(World& iWorld)
  {
    m_CurMode = Network::NetRole::None;
    m_MainChar = SpawnCharacter(iWorld, Network::NetRole::None);

    auto& transforms = *iWorld.GetSystem<Transforms>();
    transforms.Attach(GetCamera().cameraObj, m_MainChar, Transforms::Position);
  }

  NavMesh AbilityRoom::BuildDefaultMap(World& iWorld, ObjectHandle iMapHandle)
  {
    DungeonGraph_Res dung;
    dung.MakeSingleRoom(8);

    eXl::LayoutCollection rooms;

    rooms.push_back(eXl::Layout());
    rooms.back().push_back(eXl::Room());

    rooms.back().back().m_Box = AABB2Di(0, 0, 20, 20);
    rooms.back().back().m_Node = *dung.GetGraph().m_vertices.begin();

    return CreateMapFromRooms(iWorld, iMapHandle, dung, rooms);
  }

  struct AbilityRoom::PlateTrigger : TriggerCallback
  {
    PlateTrigger(AbilityRoom& iRoom, World& iWorld)
      : m_Room(iRoom)
      , m_World(iWorld)
      , m_Transforms(*iWorld.GetSystem<Transforms>())
      , m_Gfx(*iWorld.GetSystem<GfxSystem>())
    {
    }

    void OnEnter(const Vector<ObjectPair>& iNewPairs) 
    {
      for (auto const& pair : iNewPairs)
      {
        auto iter = m_Entries.find(pair.first);
        if (iter != m_Entries.end())
        {
          TriggerEntry& entry = iter->second;
          ++entry.inside;
          if (entry.inside == 1)
          {
            GfxSpriteComponent* sprite = m_Gfx.GetSpriteComponent(pair.first);
            sprite->SetTileName(TileName("Switch_On"));
            Vector3f emitterPos = MathTools::GetPosition(m_Transforms.GetWorldTransform(entry.emitter));
            if (!entry.aiming)
            {
              m_Room.CreateFireball(m_World, emitterPos + entry.dir, entry.dir);
            }
            else
            {
              Vector3f targetPos = MathTools::GetPosition(m_Transforms.GetWorldTransform(pair.second));
              Vector3f dir = targetPos - emitterPos;
              dir.Normalize();

              m_Room.CreateFireball(m_World, emitterPos + entry.dir, dir);
            }
          }
        }
      }
    }

    void OnLeave(const Vector<ObjectPair>& iNewPairs) 
    {
      for (auto const& pair : iNewPairs)
      {
        auto iter = m_Entries.find(pair.first);
        if (iter != m_Entries.end())
        {
          TriggerEntry& entry = iter->second;
          --entry.inside;
          if (entry.inside == 0)
          {
            GfxSpriteComponent* sprite = m_Gfx.GetSpriteComponent(pair.first);
            sprite->SetTileName(TileName("Switch_Off"));
          }
        }
      }
    }

    struct TriggerEntry
    {
      ObjectHandle emitter;
      Vector3f dir;
      uint32_t inside = 0;
      bool aiming = false;
    };

    World& m_World;
    AbilityRoom& m_Room;
    Transforms& m_Transforms;
    GfxSystem& m_Gfx;
    UnorderedMap<ObjectHandle, TriggerEntry> m_Entries;
  };

  void AbilityRoom::Init(World& world)
  {
    DunAtk_Application& appl = static_cast<DunAtk_Application&>(Application::GetAppl());

    appl.GetMenuManager().AddMenu("Network")
      .AddOpenPanelCommand("Connect", [this, &world] { return new NetworkPanel(world, *this); })
      .EndMenu();


    auto& transforms = *world.GetSystem<Transforms>();
    auto& gfxSys = *world.GetSystem<GfxSystem>();
    auto& phSys = *world.GetSystem<PhysicsSystem>();
    auto& charSys = *world.GetSystem<CharacterSystem>();
    auto& navigator = *world.GetSystem<NavigatorSystem>();
    auto& abilitySys = *world.GetSystem<AbilitySystem>();

    //abilitySys.RegisterAbility(new ContactDamageAbility);
    //abilitySys.RegisterEffect(new ContactDamageEffect);
    abilitySys.RegisterAbility(new GrabAbility);
    abilitySys.RegisterEffect(new GrabbedEffect);
    abilitySys.RegisterAbility(new WalkAbility);
    abilitySys.RegisterAbility(new PickAbility);
    abilitySys.RegisterAbility(new ThrowAbility);
    abilitySys.RegisterAbility(new SwordAbility);

    auto triggerCb = std::make_unique<PlateTrigger>(*this, world);
    m_PlateBehaviour = triggerCb.get();
    m_PlateCallback = phSys.AddTriggerCallback(std::move(triggerCb));


    unsigned int const spriteSize = 16;

    ResourceHandle<Tileset> tilesetHandle;
    {
      Resource::UUID id({ 2191849209, 2286785320, 240566463, 373114736 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_CharacterTileset = tilesetHandle.Get();
    }
    
    {
      Resource::UUID id({ 3495140576, 2538432508, 3618018991, 904438486 });
      
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_FireballTileset = tilesetHandle.Get();
    }
    {
      Resource::UUID id({ 1077917011, 3024235796, 2526589356, 1370983528 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_CrateTileset = tilesetHandle.Get();
    }
    {
      Resource::UUID id({ 86315995, 1229174732, 712222382, 1381354038 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_VaseTileset = tilesetHandle.Get();
    }
    {
      Resource::UUID id({ 2187413379, 893556734, 3293346461, 2634847951 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
      m_DungeonTileset = tilesetHandle.Get();
    }

    s_DefaultAnim.Register(world);
    //"Crate";

    m_MapHandle = world.CreateObject();
    m_NavMesh = BuildDefaultMap(world, m_MapHandle);

    navigator.SetNavMesh(m_NavMesh);

    uint32_t componentIdx = m_RandGen->Generate() % m_NavMesh.GetComponents().size();
    uint32_t mainRoomFaceIdx = m_RandGen->Generate() % m_NavMesh.GetFaces(componentIdx).size();
    AABB2Df mainRoom = m_NavMesh.GetFaces(componentIdx)[mainRoomFaceIdx].m_Box;

    m_RoomCenter = mainRoom.GetCenter() - Vector2f::UNIT_X * 10;

    Vector<ObjectHandle> autonomousAgents;

    world.AddTick(World::FrameStart, [this](World& iWorld, float)
    {
      ProcessInputs(iWorld);
    });

    world.AddTick(World::PostPhysics, [this](World& iWorld, float)
    {
      auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
      auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();

      Vector<CollisionData> const& lastCollisions = phSys.GetLastCollisions();
      for (auto const& collision : lastCollisions)
      {
        //if (abilitySys.HasTag(collision.obj1, fireballTag))
        //{
        //  //abilitySystem.UseAbility<ContactDamageAbility>(m_Fireball, contactDamageAbilityName, [&collision](ObjectHandle iUser, ContactDamageAbility& iAbility)
        //  //{
        //  //  iAbility.m_StateBuffer.m_User = iUser;
        //  //  iAbility.m_StateBuffer.m_Target = collision.obj1;
        //  //});
        //  abilitySys.CreateEffect<ContactDamageEffect>(collision.obj2, contactDamageEffectName, [](ObjectHandle iTarget, ContactDamageEffect& iEffect)
        //  {
        //    EffectHandle handle = iEffect.Create(iTarget);
        //    auto& state = iEffect.Get(handle);
        //    state.m_Target = iTarget;
        //    state.damageAmount = 10;
        //
        //    return handle;
        //  });
        //  //iWorld.DeleteObject(collision.obj1);
        //}
      }
    });

    world.AddTick(World::PostAbilites, [](World& iWorld, float iDelta)
    {
      auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
      //for (auto const& obj : abilitySys.GetPropertiesModified())
      //{
      //  boost::optional<float> prop = abilitySys.GetProperty(obj.first, obj.second);
      //  LOG_INFO << *prop << "\n";
      //}
    });

    world.AddTimer(0.25, true, [this](World& iWorld)
    {
      return;

      Quaternionf rot45;
      rot45.FromAxisAngle(Vector3f::UNIT_Z, Mathf::PI / 4);
      
      Quaternionf rot[8];
      rot[0] = m_RotatingBase;
      for (uint32_t i = 1; i < 8; ++i)
      {
        rot[i] = rot[i - 1] * rot45;
      }

      Vector3f orig = MathTools::To3DVec(m_RoomCenter - Vector2f::UNIT_X * 12);
      for (uint32_t i = 0; i < 8; ++i)
      {
        Vector3f dir = rot[i].Rotate(Vector3f::UNIT_X);
        CreateFireball(iWorld, orig + dir * 4 * 2, dir);
      }

      Quaternionf rot10;
      rot10.FromAxisAngle(Vector3f::UNIT_Z, Mathf::PI / 36);

      m_RotatingBase = m_RotatingBase * rot10;
    });

    //CreateCrate(world, MathTools::To3DVec(m_RoomCenter + Vector2f::UNIT_Y * - 4.0));
    CreateVase(world, MathTools::To3DVec(m_RoomCenter + Vector2f::UNIT_Y * -8.0));

    StartLocal(world);

#if 1

    CharacterSystem::Desc baseDesc;
    baseDesc.animation = &s_DefaultAnim;
    Vector<ObjectHandle> agents = NavigatorBench::BuildCrossingTest(world, baseDesc, m_NavMesh, componentIdx);
    for (auto agent : agents)
    {
      GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(agent);
      gfxComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
      gfxComp.SetTileset(m_CharacterTileset);
      gfxComp.SetOffset(Vector2f::UNIT_Y / DunAtk::s_WorldToPixel);
    
      charSys.AddCharacter(agent, baseDesc);
    }
#endif
    //world.AddTick(World::PostPhysics, [this, componentIdx](World& iWorld, float iDelta) 
    //{ 
    //  NavigatorBench::StepFullScaleTest(iWorld, iDelta, m_NavMesh, componentIdx, *m_RandGen);
    //});

    CreateTrigger(world,
      MathTools::To3DVec(m_RoomCenter + Vector2f(-4, 4)),
      MathTools::To3DVec(m_RoomCenter + Vector2f(4, 4)),
      Vector3f(-1, 0, 0));
  }

  void AbilityRoom::CreateTrigger(World& iWorld, Vector3f const& iPos, Vector3f const& iEmitterPos, Vector3f const& iDir)
  {
    auto& transSys = *iWorld.GetSystem<Transforms>();
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& archSys = *iWorld.GetSystem<GameDatabase>();

    Matrix4f trans;
    trans.MakeIdentity();

    ObjectHandle pressurePlate = iWorld.CreateObject();
    MathTools::GetPosition(trans) = iPos;
    transSys.AddTransform(pressurePlate, &trans);

    ObjectHandle emitter = iWorld.CreateObject();
    MathTools::GetPosition(trans) = iEmitterPos;
    transSys.AddTransform(emitter, &trans);

    TileName emitterTile("Beast2_L");
    if (iDir.Dot(Vector3f::UNIT_X))
    {
      emitterTile = TileName("Beast2_R");
    }

    GfxSpriteComponent& emitterComp = gfxSys.CreateSpriteComponent(emitter);
    emitterComp.SetTileset(m_DungeonTileset);
    emitterComp.SetTileName(emitterTile);

    PhysicInitData emitterPh;
    emitterPh.SetFlags(DunAtk::s_BasePhFlags | PhysicFlags::Static);
    emitterPh.AddBox(Vector3f::ONE);
    phSys.CreateComponent(emitter, emitterPh);

    TriggerDef def;
    def.m_Geom = GeomDef::MakeBox(Vector3f::ONE);

    phSys.AddTrigger(pressurePlate, def, m_PlateCallback);

    GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(pressurePlate);
    gfxComp.SetTileset(m_DungeonTileset);
    gfxComp.SetTileName(TileName("Switch_Off"));
    gfxComp.SetFlat(true);

    PlateTrigger::TriggerEntry desc;
    desc.dir = iDir;
    desc.emitter = emitter;

    m_PlateBehaviour->m_Entries.insert(std::make_pair(pressurePlate, desc));

#ifdef EXL_RSC_HAS_FILESYSTEM

    LuaScriptSystem* scripts = iWorld.GetSystem<LuaScriptSystem>();

    m_TriggerB = LuaScriptBehaviour::Create(Path("D:/Test"), "TestScript");
    m_TriggerB->m_BehaviourName = "Trigger";
    m_TriggerB->m_Script = R"-(

local module TriggerScriptXJKPQJDI = {}

function TriggerScriptXJKPQJDI.Init(object)
  return { inside = 0, emitter = eXl.ObjectHandle() }
end

function TriggerScriptXJKPQJDI.Enter(scriptObj, trigger, object) 
  scriptObj.inside = scriptObj.inside + 1
  print("EnterTriggered")
  
  local world = eXl.GetWorld()
  local trans = world:GetTransforms()

  local archSys = world:GetArchetypeSys();

  local turretProp = archSys:GetProperty(trigger, eXl.PropertySheetName("Turret"))
  local posToTest = turretProp.m_FireDir * 12.0;
  turretProp.m_FireRate = turretProp.m_FireRate * 2
  local newPos = eXl.Matrix4f.FromPosition(eXl.Vector3f(60.0, 60.0, 0.0))
  trans:UpdateTransform(trigger, newPos)

  print("Fire rate : ", turretProp.m_FireRate)

  local healthProp = archSys:GetProperty(trigger, eXl.PropertySheetName("Health"))
  healthProp.currentHealth = healthProp.currentHealth - 1

  print("Current health : ", healthProp.currentHealth)

end

function TriggerScriptXJKPQJDI.Leave(scriptObj, trigger, object) 
  scriptObj.inside = scriptObj.inside - 1
  print("LeaveTriggered")
end

return TriggerScriptXJKPQJDI
)-";

    Archetype* arch = Archetype::Create("D://TestData", "dummy");
    DunAtk::TurretData dummyData;
    dummyData.m_FireDir = Vector3f::ONE * 4;
    dummyData.m_FireRate = 1.0;
     
    arch->SetProperty(DunAtk::TurretData::PropertyName(), ConstDynObject(DunAtk::TurretData::GetType(), &dummyData), false);

    DunAtk::HealthData health;
    health.currentHealth = 10;
    health.maxHealth = 10;

    arch->SetProperty(DunAtk::HealthData::PropertyName(), ConstDynObject(DunAtk::HealthData::GetType(), &health), true);

    archSys.InstantiateArchetype(pressurePlate, arch, nullptr);

    scripts->LoadScript(*m_TriggerB);

    scripts->AddBehaviour(pressurePlate, *m_TriggerB);
    scripts->CallBehaviour<void>(pressurePlate, Name("Trigger"), Name("Enter"), pressurePlate, pressurePlate);
    scripts->CallBehaviour<void>(pressurePlate, Name("Trigger"), Name("Enter"), pressurePlate, pressurePlate);
    scripts->CallBehaviour<void>(pressurePlate, Name("Trigger"), Name("Enter"), pressurePlate, pressurePlate);
#endif
  }

  ObjectHandle AbilityRoom::CreateFireball(World& iWorld, Vector3f const& iPos, Vector3f const& iDir)
  {
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& navigator = *iWorld.GetSystem<NavigatorSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
    auto& projectiles = *iWorld.GetSystem<ProjectileSystem>();

    ProjectileSystem::Desc fireballDesc;
    fireballDesc.kind = ProjectileSystem::PhysicKind::Kinematic;
    fireballDesc.size = 1.0;
    fireballDesc.rotateSprite = true;
    fireballDesc.registerNav = true;

    ObjectHandle fireball = ProjectileSystem::Build(iWorld, iPos, fireballDesc);
    {
      GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(fireball);
      gfxComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
      gfxComp.SetOffset(((-Vector2f::UNIT_X) / DunAtk::s_WorldToPixel));
      gfxComp.SetTileset(m_FireballTileset);
      gfxComp.SetTileName(TileName("Right"));
      gfxComp.SetRotateSprite(true);

      projectiles.AddProjectile(fireball, fireballDesc, iDir * 20);

      abilitySys.CreateComponent(fireball);
      //abilitySys.AddTag(fireball, fireballTag);
    }

    iWorld.AddTimer(10.0, false, [fireball](World& iWorld)
    {
      iWorld.DeleteObject(fireball);
    });

    return fireball;
  }

  ObjectHandle AbilityRoom::CreateCrate(World& iWorld, Vector3f const& iPos)
  {
    auto& transforms = *iWorld.GetSystem<Transforms>();
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
    auto& navSys = *iWorld.GetSystem<NavigatorSystem>();

    ObjectHandle crateObject = iWorld.CreateObject();

    Matrix4f objTrans = Matrix4f::IDENTITY;
    MathTools::GetPosition2D(objTrans) = MathTools::As2DVec(iPos);
    transforms.AddTransform(crateObject, &objTrans);

    PhysicInitData desc;
    uint32_t flags = /*PhysicFlags::NeedContactNotify |*/ PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation;
    flags |= PhysicFlags::Kinematic;
    
    desc.SetFlags(flags);
    desc.AddBox(Vector3f::ONE * 2);

    phSys.CreateComponent(crateObject, desc);

    navSys.AddObstacle(crateObject, 2);
    phSys.GetNeighborhoodExtraction().AddObject(crateObject, 2, false);

    GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(crateObject);
    gfxComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
    gfxComp.SetTileset(m_CrateTileset);
    gfxComp.SetTileName(TileName("Crate"));
    gfxComp.SetRotateSprite(true);

    abilitySys.CreateComponent(crateObject);
    abilitySys.AddTag(crateObject, GrabAbility::GrabbableTagName());
    //abilitySys.AddTag(crateObject, PickAbility::PickableTagName());

    auto& transformAnimManager = *iWorld.GetSystem<TransformAnimManager>();

    static LinearPositionAnimation s_PosAnim;
    {
      s_PosAnim.Add(Vector3f(-10.0,   0.0, 0.0),  5.0);
      s_PosAnim.Add(Vector3f(-10.0, -10.0, 0.0), 10.0);
      s_PosAnim.Add(Vector3f(  0.0, -10.0, 0.0), 15.0);
      s_PosAnim.Add(Vector3f(  0.0,   0.0, 0.0), 20.0);
    }
    transformAnimManager.StartLooping(crateObject, s_PosAnim, 20.0, Matrix4f::IDENTITY, objTrans);

    auto& projectiles = *iWorld.GetSystem<ProjectileSystem>();
    ProjectileSystem::Desc projDesc;
    projDesc.kind = ProjectileSystem::PhysicKind::KinematicAbsolute;

    projectiles.AddProjectile(crateObject, projDesc, Vector3f::ZERO);

    return crateObject;
  }

  ObjectHandle AbilityRoom::CreateVase(World& iWorld, Vector3f const& iPos)
  {
    auto& transforms = *iWorld.GetSystem<Transforms>();
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
    auto& navSys = *iWorld.GetSystem<NavigatorSystem>();

    ObjectHandle vaseObject = iWorld.CreateObject();

    Matrix4f objTrans = Matrix4f::IDENTITY;
    MathTools::GetPosition2D(objTrans) = MathTools::As2DVec(iPos);
    transforms.AddTransform(vaseObject, &objTrans);

    PhysicInitData desc;
    uint32_t flags = /*PhysicFlags::NeedContactNotify |*/ PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation;
    flags |= PhysicFlags::Kinematic;

    desc.SetFlags(flags);
    desc.AddSphere(0.75);

    phSys.CreateComponent(vaseObject, desc);

    navSys.AddObstacle(vaseObject, 0.75);
    phSys.GetNeighborhoodExtraction().AddObject(vaseObject, 0.75, false);

    GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(vaseObject);
    gfxComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
    gfxComp.SetOffset((Vector2f::UNIT_Y * 0.25));
    gfxComp.SetTileset(m_VaseTileset);
    gfxComp.SetTileName(TileName("Vase"));
    gfxComp.SetRotateSprite(true);

    abilitySys.CreateComponent(vaseObject);
    abilitySys.AddTag(vaseObject, GrabAbility::GrabbableTagName());
    abilitySys.AddTag(vaseObject, PickAbility::PickableTagName());

    return vaseObject;
  }

  ObjectHandle AbilityRoom::SpawnCharacter(World& iWorld, Network::NetRole iRole)
  {
    auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
    auto& charSys = *iWorld.GetSystem<CharacterSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();

    CharacterSystem::Desc defaultDesc;
    defaultDesc.kind = CharacterSystem::PhysicKind::Kinematic;
    defaultDesc.animation = &s_DefaultAnim;
    switch (iRole)
    {
    case Network::NetRole::Client:
      defaultDesc.controlKind = CharacterSystem::ControlKind::Remote;
      break;
    case Network::NetRole::None:
    case Network::NetRole::Server:
      defaultDesc.controlKind = CharacterSystem::ControlKind::Predicted;
      break;
    }
    
    defaultDesc.size = 1.0;
    defaultDesc.maxSpeed = 10.0;

    ObjectHandle newChar = CharacterSystem::Build(iWorld, MathTools::To3DVec(m_RoomCenter), defaultDesc);
    {
      GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(newChar);
      gfxComp.SetSize(Vector2f::ONE / DunAtk::s_WorldToPixel);
      gfxComp.SetTileset(m_CharacterTileset);
      //gfxComp.SetTint(Vector4f(1.0, 0.0, 0.0, 1.0));

      charSys.AddCharacter(newChar, defaultDesc);

      abilitySys.CreateComponent(newChar);
      abilitySys.AddAbility(newChar, WalkAbility::Name());
      abilitySys.AddAbility(newChar, GrabAbility::Name());
      abilitySys.AddAbility(newChar, PickAbility::Name());
      abilitySys.AddAbility(newChar, ThrowAbility::Name());
      abilitySys.AddAbility(newChar, SwordAbility::Name());
    }

    return newChar;
  }

  void AbilityRoom::ProcessInputs(World& iWorld)
  {
    DunAtk_Application& app = DunAtk_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();

    GfxSystem& gfxSys = *iWorld.GetSystem<GfxSystem>();
    Vector2i vptSize = gfxSys.GetViewportSize();

    AbilitySystem& abilities = *iWorld.GetSystem<AbilitySystem>();

    AbilityName actionAbilities[] =
    {
      GrabAbility::Name(),
      PickAbility::Name(),
      ThrowAbility::Name(),
    };

    boost::optional<AbilityName> currentAbilityUsed;
    for (auto const& name : actionAbilities)
    {
      if (abilities.GetAbilityUseState(m_MainChar, name) == AbilityUseState::Using)
      {
        currentAbilityUsed = name;
        break;
      }
    }

    boost::optional<bool> actionKeyUsed;
    bool swordUsed = false;

    if (m_MainChar.IsAssigned())
    {
      for (int i = 0; i < (int)iInputs.m_KeyEvts.size(); ++i)
      {
        KeyboardEvent& evt = iInputs.m_KeyEvts[i];
        if (!evt.pressed)
        {
          if (evt.key == K_UP)
          {
            dirMask &= ~(1 << 2);
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask &= ~(1 << 3);
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask &= ~(1 << 1);
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask &= ~(1 << 0);
            keyChanged = true;
          }
          if (evt.key == K_SPACE)
          {
            actionKeyUsed = false;
          }
          if (evt.key == K_E)
          {
            swordUsed = true;
          }
        }
        else
        {
          if (evt.key == K_UP)
          {
            dirMask |= 1 << 2;
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask |= 1 << 3;
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask |= 1 << 1;
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask |= 1 << 0;
            keyChanged = true;
          }
          if (evt.key == K_SPACE)
          {
            actionKeyUsed = true;
          }
        }

        if (keyChanged || actionKeyUsed || swordUsed)
        {
          iInputs.m_KeyEvts.erase(iInputs.m_KeyEvts.begin() + i);
          --i;
        }
      }
    }

    for (auto const& evt : iInputs.m_MouseMoveEvts)
    {
      if (!evt.wheel)
      {
        m_MousePos = Vector2i(evt.absX, evt.absY);
      }
    }

    for (auto const& evt : iInputs.m_MouseEvts)
    {
      if (!evt.pressed)
      {
        dirMask = 0;
        keyChanged = true;
      }
      else
      {
        Vector2i halfSize = vptSize / 2;
        Vector2i clipSpacePos = m_MousePos;
        clipSpacePos.Y() = vptSize.Y() - m_MousePos.Y();
        Vector2i relPos = clipSpacePos - halfSize;
        Vector2i otherCorner(halfSize.X(), -halfSize.Y());

        uint32_t quadrant;
        if (Segmenti::IsLeft(-halfSize, halfSize, relPos) > 0)
        {
          if (Segmenti::IsLeft(-otherCorner, otherCorner, relPos) > 0)
          {
            quadrant = 3;
          }
          else
          {
            quadrant = 0;
          }
        }
        else
        {
          if (Segmenti::IsLeft(-otherCorner, otherCorner, relPos) < 0)
          {
            quadrant = 2;
          }
          else
          {
            quadrant = 1;
          }
        }

        switch (quadrant)
        {
        case 0:
          dirMask |= 1 << 1;
          keyChanged = true;
          break;
        case 1:
          dirMask |= 1 << 0;
          keyChanged = true;
          break;
        case 2:
          dirMask |= 1 << 3;
          keyChanged = true;
          break;
        case 3:
          dirMask |= 1 << 2;
          keyChanged = true;
          break;
        }
      }

    }

    if (keyChanged)
    {
      static const Vector3f dirs[] =
      {
        Vector3f::UNIT_X *  1.0,
        Vector3f::UNIT_X * -1.0,
        Vector3f::UNIT_Y *  1.0,
        Vector3f::UNIT_Y * -1.0,
      };
      Vector3f dir;
      for (unsigned int i = 0; i < 4; ++i)
      {
        if (dirMask & (1 << i))
        {
          dir += dirs[i];
        }
      }

      if (m_CurMode == Network::NetRole::Client)
      {
        Network::ClientInputData inputData;
        inputData.m_Moving = (dir != Vector3f::ZERO);
        inputData.m_Dir = dir;
        inputData.m_Dir.Normalize();

        Network::SetClientInput(m_MainChar, inputData);
      }
      else
      {
        if (dirMask == 0)
        {
          abilities.StopUsingAbility(m_MainChar, WalkAbility::Name());
        }
        else
        {
          ObjectHandle grabbedObject = GrabAbility::GetGrabbedObject(&abilities, m_MainChar);
          if (grabbedObject.IsAssigned())
          {
            Vector2f grabDir = GrabAbility::GetGrabDirection(&abilities, m_MainChar);
            grabDir.Normalize();
            Vector2f walkDir = MathTools::As2DVec(dir);
            walkDir.Normalize();
            if (Mathf::Abs(grabDir.Dot(MathTools::As2DVec(dir)) - -1.0) < Mathf::ZERO_TOLERANCE)
            {
              abilities.UseAbility(m_MainChar, PickAbility::Name(), grabbedObject);
            }
          }
          
          {
            WalkAbility::SetWalkDirection(&abilities, m_MainChar, MathTools::As2DVec(dir));
            abilities.UseAbility(m_MainChar, WalkAbility::Name());
          }
        }
      }
      keyChanged = false;
    }

    if (actionKeyUsed)
    {
      if (currentAbilityUsed)
      {
        if (*currentAbilityUsed == GrabAbility::Name())
        {
          if (!(*actionKeyUsed))
          {
            abilities.StopUsingAbility(m_MainChar, GrabAbility::Name());
          }
        }
        if (*currentAbilityUsed == PickAbility::Name())
        {
          if (*actionKeyUsed)
          {
            abilities.UseAbility(m_MainChar, ThrowAbility::Name());
          }
        }
      }
      else
      {
        if (*actionKeyUsed)
        {
          CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();
          Vector3f seekDir = controller.GetCurrentFacingDirection(m_MainChar);
          GrabAbility::SetGrabDirection(&abilities, m_MainChar, MathTools::As2DVec(seekDir));
          abilities.UseAbility(m_MainChar, GrabAbility::Name());
        }
      }
    }

    if (swordUsed)
    {
      CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();
      Vector3f swingDir = controller.GetCurrentFacingDirection(m_MainChar);
      SwordAbility::SetSwingDirection(&abilities, m_MainChar, MathTools::As2DVec(swingDir));
      abilities.UseAbility(m_MainChar, SwordAbility::Name());
    }

    GetCamera().ProcessInputs(iWorld, iInputs);
  }
  
}
