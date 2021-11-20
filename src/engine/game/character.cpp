/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/character.hpp>
#include <engine/common/object.hpp>

#include <math/mathtools.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/world.hpp>
#include <engine/game/characteranimation.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/game/commondef.hpp>

#include <engine/pathfinding/navigator.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(CharacterSystem);

  void CharacterSystem::Build(World& iWorld, ObjectHandle iObject, Vector3f const& iPosition, Desc const& iDesc)
  {
    eXl_ASSERT_REPAIR_RET(iWorld.IsObjectValid(iObject), void());

    Transforms* transforms = iWorld.GetSystem<Transforms>();
    PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
    NavigatorSystem* navSys = iWorld.GetSystem<NavigatorSystem>();

    Matrix4f navTrans = Matrix4f::IDENTITY;
    MathTools::GetPosition2D(navTrans) = MathTools::As2DVec(iPosition);
    transforms->AddTransform(iObject, &navTrans);

    PhysicInitData desc;
    uint32_t flags = PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation | PhysicFlags::AlignRotToVelocity | PhysicFlags::AddSensor;

    if(iDesc.controlKind == ControlKind::Remote)
    {
      flags |= (PhysicFlags::IsGhost | PhysicFlags::Kinematic);
    }
    else
    {
      if (iDesc.kind == PhysicKind::Ghost
        || iDesc.kind == PhysicKind::GhostAbsolute)
      {
        flags |= PhysicFlags::IsGhost;
      }
      if (iDesc.kind == PhysicKind::Kinematic
        || iDesc.kind == PhysicKind::KinematicAbsolute)
      {
        flags |= PhysicFlags::Kinematic;
      }
    }
    
    desc.SetFlags(flags);
    desc.AddSphere(iDesc.size);
    desc.SetCategory(EngineCommon::s_CharacterCategory, EngineCommon::s_CharacterMask);

    phSys->CreateComponent(iObject, desc);
    
    if (navSys)
    {
      phSys->GetNeighborhoodExtraction().AddObject(iObject, iDesc.size, true);
      if (iDesc.controlKind == ControlKind::Navigation)
      {
        navSys->AddNavigator(iObject, iDesc.size, iDesc.maxSpeed);
      }
      else
      {
        navSys->AddObstacle(iObject, iDesc.size);
      }
    }
  }

  uint32_t CharacterSystem::GetStateFromDir(Vector3f const& iDir, bool iMoving)
  {
    float dotProd[] =
    {
      iDir.Dot(Vector3f::UNIT_X),
      iDir.Dot(Vector3f::UNIT_Y),
    };

    uint32_t dirIdx = Mathf::Abs(dotProd[0]) > Mathf::Abs(dotProd[1]) ? 0 : 1;
    uint32_t sign = dotProd[dirIdx] > 0 ? 1 : 0;

    uint32_t curState = (dirIdx << 1) | sign;
    curState |= uint32_t(iMoving ? CharStateFlags::Walking : CharStateFlags::Idle);

    return curState;
  }

  void CharacterSystem::Tick(float iDelta)
  {
    Transforms* transforms = GetWorld().GetSystem<Transforms>();
    NavigatorSystem* navSys = GetWorld().GetSystem<NavigatorSystem>();
    GameDataView<Vector3f>* velocities = EngineCommon::GetVelocities(GetWorld());
    m_Entries.Iterate([this, transforms, navSys, velocities](Entry& entry, ObjectTable<Entry>::Handle)
    {
      Vector3f linVel;
      Vector3f dir;
      bool moving = false;

      //if (entry.m_Desc.controlKind == ControlKind::Navigation)
      //{
      //  NavigatorSystem::Obstacle const* obstacle = navSys->GetObstacle(entry.m_Handle);
      //  //linVel = obstacle->m_LinVel;
      //  
      //  moving = obstacle->m_LinVel.SquaredLength() > Mathf::ZERO_TOLERANCE;
      //
      //  Matrix4f const& mat = transforms->GetWorldTransform(entry.m_Handle);
      //  dir = *reinterpret_cast<Vector3f const*>(mat.m_Data + 0);
      //}
      //else 
      if (entry.m_Desc.kind == PhysicKind::Kinematic)
      {
        KinematicEntry& kEntry = GetKinematic(entry.m_KinematicEntry);
        moving = kEntry.m_Speed > Mathf::ZERO_TOLERANCE;
        dir = kEntry.m_Dir;
        linVel = dir * kEntry.m_Speed;
        velocities->GetOrCreate(entry.m_Handle) = linVel;
      }
      else
      {
        linVel = velocities->GetOrCreate(entry.m_Handle);
        dir = linVel;
        float vel = linVel.Normalize();
        moving = vel > Mathf::ZERO_TOLERANCE;
      }

      if (entry.m_Desc.controlKind != ControlKind::Remote)
      {
        uint32_t curState = GetStateFromDir(dir, moving);

        //eXl_ASSERT(curState < sizeof(s_StateAnimTable) / sizeof(TileName));

        if (entry.m_CurState != curState)
        {
          if (entry.m_Animation)
          {
            entry.m_Animation->OnWalkingStateChange(entry.m_Handle, curState);
          }
          entry.m_CurState = curState;
        }
      }
      //if (navSys)
      //{
      //  if (entry.m_Desc.controlKind != ControlKind::Navigation)
      //  {
      //    navSys->SetObstacleSpeed(entry.m_Handle, linVel);
      //  }
      //}
    });
  }

	void CharacterSystem::AddCharacter(ObjectHandle iObj, Desc const& iDesc)
	{
		if (GetWorld().IsObjectValid(iObj) )
		{
			if (m_Characters.count(iObj) == 0)
			{
        auto entryHandle = m_Entries.Alloc();
        Entry& entry = m_Entries.Get(entryHandle);
        entry.m_CurState = uint32_t(CharStateFlags::Idle);
        entry.m_Desc = iDesc;
        entry.m_Handle = iObj;
        entry.m_Animation = iDesc.animation;
        if (entry.m_Animation)
        {
          entry.m_Animation->AddCharacter(iObj);
        }

        if (iDesc.kind != PhysicKind::Simulated)
        {
          auto* phSys = GetWorld().GetSystem<PhysicsSystem>();
          if (auto phComp = phSys->GetCompImpl(iObj))
          {
            KinematicEntry kEntry;
            //kEntry.m_PhComp = phComp;
            kEntry.m_NeedDepenetration = true;
            kEntry.m_Speed = 0;
            if (iDesc.controlKind == ControlKind::Remote)
            {
              if (iDesc.kind == PhysicKind::Ghost
                || iDesc.kind == PhysicKind::GhostAbsolute)
              {
                kEntry.m_Kind = PhysicKind::GhostAbsolute;
              }
              if (iDesc.kind == PhysicKind::Kinematic
                || iDesc.kind == PhysicKind::KinematicAbsolute)
              {
                kEntry.m_Kind = PhysicKind::KinematicAbsolute;
              }
            }
            else
            {
              kEntry.m_Kind = iDesc.kind;
            }
            entry.m_KinematicEntry = CreateKinematic(*phComp, &kEntry);
          }
        }
        m_Characters.emplace(std::make_pair(iObj, entryHandle));
        ComponentManager::CreateComponent(iObj);
			}
		}
	}

  void CharacterSystem::DeleteComponent(ObjectHandle iObj)
  {
    auto iter = m_Characters.find(iObj);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_KinematicEntry.IsAssigned())
      {
        RemoveKinematic(entry.m_KinematicEntry);
      }
      if (entry.m_Animation)
      {
        entry.m_Animation->RemoveCharacter(iObj);
      }
      m_Entries.Release(iter->second);
      m_Characters.erase(iter);
      ComponentManager::DeleteComponent(iObj);
    }
  }

	void CharacterSystem::SetSpeed(ObjectHandle iObj, float iSpeed)
	{
		auto iter = m_Characters.find(iObj);
		if (iter != m_Characters.end())
		{
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_KinematicEntry.IsAssigned())
      {
        KinematicEntry& kEntry = GetKinematic(entry.m_KinematicEntry);
        kEntry.m_Speed = Mathf::Clamp(iSpeed, 0.0, entry.m_Desc.maxSpeed);
      }
		}
	}

  void CharacterSystem::SetState(ObjectHandle iObj, uint32_t iState)
  {
    auto iter = m_Characters.find(iObj);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_Desc.controlKind == ControlKind::Remote)
      {
        if (entry.m_CurState != iState)
        {
          if (entry.m_Animation)
          {
            entry.m_Animation->OnWalkingStateChange(entry.m_Handle, iState);
          }
          entry.m_CurState = iState;
        }
      }
    }
  }

  uint32_t CharacterSystem::GetCurrentState(ObjectHandle iObj)
  {
    auto iter = m_Characters.find(iObj);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      return entry.m_CurState;
    }
    return 0;
  }

  Vector3f CharacterSystem::GetCurrentFacingDirection(ObjectHandle iObj)
  {
    Vector3f dir;
    auto iter = m_Characters.find(iObj);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      switch (entry.m_CurState & (uint32_t)CharacterSystem::StateFlags::DirMask)
      {
      case (uint32_t)CharacterSystem::StateFlags::DirLeft:
        dir = Vector3f::UNIT_X * -1;
        break;
      case (uint32_t)CharacterSystem::StateFlags::DirRight:
        dir = Vector3f::UNIT_X;
        break;
      case (uint32_t)CharacterSystem::StateFlags::DirDown:
        dir = Vector3f::UNIT_Y * -1;
        break;
      case (uint32_t)CharacterSystem::StateFlags::DirUp:
        dir = Vector3f::UNIT_Y;
        break;
      }
    }

    return dir;
  }

	void CharacterSystem::SetCurDir(ObjectHandle iObj, Vector3f const& iDir)
	{
		auto iter = m_Characters.find(iObj);
		if (iter != m_Characters.end())
		{
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_KinematicEntry.IsAssigned())
      {
        KinematicEntry& kEntry = GetKinematic(entry.m_KinematicEntry);
        kEntry.m_Dir = iDir;
        kEntry.m_Dir.Normalize();
      }
		}
	}

  bool CharacterSystem::GrabObject(ObjectHandle iGrabber, ObjectHandle iGrabbed)
  {
    auto iter = m_Characters.find(iGrabber);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_KinematicEntry.IsAssigned())
      {
        auto& phSys = *GetWorld().GetSystem<PhysicsSystem>();
        
        PhysicComponent_Impl* grabbedPh = phSys.GetCompImpl(iGrabbed);

        if (!grabbedPh)
        {
          return false;
        }
        
        return Attach(entry.m_KinematicEntry, *grabbedPh);
      }
    }
    return false;
  }

  bool CharacterSystem::ReleaseObject(ObjectHandle iGrabber, ObjectHandle iGrabbed)
  {
    auto iter = m_Characters.find(iGrabber);
    if (iter != m_Characters.end())
    {
      Entry& entry = m_Entries.Get(iter->second);
      if (entry.m_KinematicEntry.IsAssigned())
      {
        auto& phSys = *GetWorld().GetSystem<PhysicsSystem>();
        
        PhysicComponent_Impl* grabbedComp = phSys.GetCompImpl(iGrabbed);
        Detach(entry.m_KinematicEntry, *grabbedComp);

        return true;
      }
    }
    return false;
  }
}