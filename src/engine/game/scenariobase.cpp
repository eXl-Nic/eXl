/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/scenariobase.hpp>

#include <engine/game/character.hpp>
#include <engine/game/characteranimation.hpp>
#include <engine/game/ability.hpp>
#include <engine/game/grabability.hpp>
#include <engine/game/pickability.hpp>
#include <engine/game/walkability.hpp>
#include <engine/game/throwability.hpp>
#include <engine/game/swordability.hpp>

#include <engine/common/transforms.hpp>
#include <engine/game/character.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <core/input.hpp>

#include <math/mathtools.hpp>

namespace eXl
{
  void Scenario_Base::Init(World& iWorld)
  {
    Engine_Application& appl = static_cast<Engine_Application&>(Application::GetAppl());

    auto& transforms = *iWorld.GetSystem<Transforms>();
    //auto& phSys = *iWorld.GetSystem<PhysicsSystem>();
    auto& charSys = *iWorld.GetSystem<CharacterSystem>();
    auto& navigator = *iWorld.GetSystem<NavigatorSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();

    abilitySys.RegisterAbility(new GrabAbility);
    abilitySys.RegisterEffect(new GrabbedEffect);
    abilitySys.RegisterAbility(new WalkAbility);
    abilitySys.RegisterAbility(new PickAbility);
    abilitySys.RegisterAbility(new ThrowAbility);
    abilitySys.RegisterAbility(new SwordAbility);

    auto* gfxSys = iWorld.GetSystem<GfxSystem>();

    if (MapResource const* map = m_Map.GetOrLoad())
    {
      m_InstatiatedMap = map->Instantiate(iWorld);
    }

    if (m_InstatiatedMap.navMesh)
    {
      auto components = m_InstatiatedMap.navMesh->GetComponents();
      if (components.size() > 0)
      {
        eXl_ASSERT_REPAIR_RET(components[0].m_Faces.size() > 0, );
        m_SpawnPos = components[0].m_Faces[0].m_Box.GetCenter();
      }
    }

    iWorld.AddTick(World::FrameStart, [this](World& iWorld, float)
    {
      ProcessInputs(iWorld);
    });

    StartLocal(iWorld);

  }

  ObjectHandle Scenario_Base::SpawnCharacter(World& iWorld, Network::NetRole iRole)
  {
    auto& charSys = *iWorld.GetSystem<CharacterSystem>();
    auto& abilitySys = *iWorld.GetSystem<AbilitySystem>();
    static CharacterAnimation s_DefaultAnim;
    s_DefaultAnim.Register(iWorld);

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

    ObjectHandle newChar = iWorld.CreateObject();
    CharacterSystem::Build(iWorld, newChar, MathTools::To3DVec(m_SpawnPos), defaultDesc);
    {
      if (auto* gfxSys = iWorld.GetSystem<GfxSystem>())
      {
        Tileset const* characterTileset = []
        {
          ResourceHandle<Tileset> tilesetHandle;
          Resource::UUID id({ 2191849209, 2286785320, 240566463, 373114736 });
          tilesetHandle.SetUUID(id);
          tilesetHandle.Load();
          return tilesetHandle.Get();
        }();

        GfxSpriteComponent& gfxComp = gfxSys->CreateSpriteComponent(newChar);
        gfxComp.SetTileset(characterTileset);
      }
      //gfxComp.SetTint(Vector4f(1.0, 0.0, 0.0, 1.0));

      charSys.AddCharacter(newChar, defaultDesc);

      abilitySys.CreateComponent(newChar);
      abilitySys.AddAbility(newChar, WalkAbility::Name());
      abilitySys.AddAbility(newChar, GrabAbility::Name());
      abilitySys.AddAbility(newChar, PickAbility::Name());
      abilitySys.AddAbility(newChar, ThrowAbility::Name());
      //abilitySys.AddAbility(newChar, SwordAbility::Name());
    }

    return newChar;
  }

  void Scenario_Base::StartServer(World& iWorld)
  {
#if 0

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
#endif
  }

  void Scenario_Base::StartClient(World& iWorld, String const& iURL)
  {
#if 0
    m_CurMode = Network::NetRole::Client;
    Network::OnNewObjectReceived = [this, &iWorld](Network::ClientData const&)
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
#endif
  }

  void Scenario_Base::StartLocal(World& iWorld)
  {
    m_CurMode = Network::NetRole::None;
    m_MainChar = SpawnCharacter(iWorld, Network::NetRole::None);

    auto& transforms = *iWorld.GetSystem<Transforms>();
    transforms.Attach(GetCamera().cameraObj, m_MainChar, Transforms::Position);
  }

  void Scenario_Base::ProcessInputs(World& iWorld)
  {
    Engine_Application& app = Engine_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();

    GfxSystem* gfxSys = iWorld.GetSystem<GfxSystem>();
    Vector2i vptSize = gfxSys ? gfxSys->GetViewportSize() : Vector2i::ONE;

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
#if 0
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
#endif
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
#if 0
        Network::ClientInputData inputData;
        inputData.m_Moving = (dir != Vector3f::ZERO);
        inputData.m_Dir = dir;
        inputData.m_Dir.Normalize();

        Network::SetClientInput(m_MainChar, inputData);
#endif
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