/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/projectile.hpp>
#include <engine/common/object.hpp>

#include <math/mathtools.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/world.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent_impl.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/pathfinding/navigator.hpp>

#include <engine/game/commondef.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ProjectileSystem);

  ObjectHandle ProjectileSystem::Build(World& iWorld, Vector3f const& iPosition, Desc const& iDesc)
  {
    Transforms* transforms = iWorld.GetSystem<Transforms>();
    PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
    NavigatorSystem* navSys = iWorld.GetSystem<NavigatorSystem>();

    ObjectHandle newObject = iWorld.CreateObject();

    transforms->AddTransform(newObject, Matrix4f::FromPosition(iPosition));

    PhysicInitData desc;
    uint32_t flags = PhysicFlags::NeedContactNotify | PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation | PhysicFlags::AlignRotToVelocity;
    if (iDesc.kind == PhysicKind::Ghost)
    {
      flags |= PhysicFlags::IsGhost;
    }
    if (iDesc.kind == PhysicKind::Ghost || iDesc.kind == PhysicKind::Kinematic)
    {
      flags |= PhysicFlags::Kinematic;
    }

    desc.SetFlags(flags);
    desc.AddSphere(iDesc.size);
    desc.SetCategory(EngineCommon::s_ProjectileCategory, EngineCommon::s_ProjectileMask);

    phSys->CreateComponent(newObject, desc);

    return newObject;
  }

  void ProjectileSystem::Register(World& iWorld)
  {
    MovementManagerBase::Register(iWorld);
    m_Entries.emplace(iWorld);
  }

  void ProjectileSystem::Tick(float iDelta)
  {
    Transforms* transforms = GetWorld().GetSystem<Transforms>();
    NavigatorSystem* navSys = GetWorld().GetSystem<NavigatorSystem>();
    GameDataView<Vector3f>* velocities = EngineCommon::GetVelocities(GetWorld());
    if (velocities == nullptr)
    {
      return;
    }
    m_Entries->Iterate([this, transforms, navSys, velocities](ObjectHandle iObject, Entry& entry)
    {
      Vector3f dir;
      float speed;
      if (entry.m_Desc.kind == PhysicKind::Kinematic)
      {
        KinematicEntry& kEntry = GetKinematic(entry.m_KinematicEntry);
        dir = kEntry.m_Dir;
        speed = kEntry.m_Speed;
        velocities->GetOrCreate(iObject) = dir * speed;
      }
      if (entry.m_Desc.kind == PhysicKind::KinematicAbsolute
        || entry.m_Desc.kind == PhysicKind::GhostAbsolute)
      {
        Matrix4f const& trans = transforms->GetWorldTransform(iObject);
        dir = *reinterpret_cast<Vector3f const*>(trans.m_Data);
        speed = 0.0;
      }
      
      if (!entry.m_Desc.rotateSprite)
      {
        float dotProd[] =
        {
          dir.Dot(Vector3f::UNIT_X),
          dir.Dot(Vector3f::UNIT_Y),
        };

        uint32_t dirIdx = Mathf::Abs(dotProd[0]) > Mathf::Abs(dotProd[1]) ? 0 : 1;
        uint32_t sign = dotProd[dirIdx] > 0 ? 1 : 0;

        uint32_t curState = (dirIdx << 1) | sign;

        if (entry.m_CurState != curState)
        {
          if (entry.m_GfxComp)
          {
            TileName animState("INVALID");
            switch (curState)
            {
            case 0:
              animState = TileName("Left");
              break;
            case 1:
              animState = TileName("Right");
              break;
            case 2:
              animState = TileName("Down");
              break;
            case 3:
              animState = TileName("Up");
              break;
            }
            entry.m_GfxComp->SetTileName(animState);
          }
          entry.m_CurState = curState;
        }
      }
    });
  }

  void ProjectileSystem::AddProjectile(ObjectHandle iObj, Desc const& iDesc, Vector3f const& iInitialSpeed)
  {
    if (GetWorld().IsObjectValid(iObj))
    {
      if (Entry const* entry = m_Entries->GetDataForDeletion(iObj))
      {
        return;
      }
      Entry& entry = m_Entries->GetOrCreate(iObj);
      entry.m_CurState = -1;
      entry.m_Desc = iDesc;

      auto* phSys = GetWorld().GetSystem<PhysicsSystem>();
      if (auto phComp = phSys->GetCompImpl(iObj))
      {
        if (entry.m_Desc.kind != PhysicKind::Simulated)
        {
          KinematicEntry kEntry;
          kEntry.m_Dir = iInitialSpeed;
          kEntry.m_Speed = kEntry.m_Dir.Normalize();
          kEntry.m_CanBounce = true;
          kEntry.m_Kind = entry.m_Desc.kind;
          kEntry.contactCb = iDesc.contactCb;
          entry.m_KinematicEntry = CreateKinematic(*phComp, &kEntry);
        }
        else
        {
          phSys->AddContactCb(iObj, iDesc.contactCb);
          ApplyLinearVelocity(*phComp, iInitialSpeed, 1.0);
        }
      }

      if (iDesc.registerNav)
      {
        NavigatorSystem* navSys = GetWorld().GetSystem<NavigatorSystem>();
        navSys->AddObstacle(iObj, iDesc.size);
        phSys->GetNeighborhoodExtraction().AddObject(iObj, iDesc.size, false);
      }

      ComponentManager::CreateComponent(iObj);
    }
  }

  void ProjectileSystem::DeleteComponent(ObjectHandle iObj)
  {
    if(Entry const* entry = m_Entries->GetDataForDeletion(iObj))
    {
      if (entry->m_KinematicEntry.IsAssigned())
      {
        RemoveKinematic(entry->m_KinematicEntry);
      }
      ComponentManager::DeleteComponent(iObj);
    }
  }
}