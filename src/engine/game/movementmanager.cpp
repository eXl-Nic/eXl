/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/movementmanager.hpp>

#include <math/mathtools.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent_impl.hpp>

namespace eXl
{
  MovementManagerBase::KinematicEntry::KinematicEntry() = default;
  MovementManagerBase::KinematicEntry::~KinematicEntry() = default;
  MovementManagerBase::KinematicEntry::KinematicEntry(KinematicEntry const&) = default;
  MovementManagerBase::KinematicEntry& MovementManagerBase::KinematicEntry::operator =(KinematicEntry const&) = default;
  MovementManagerBase::KinematicEntry::KinematicEntry(KinematicEntry&&) = default;
  MovementManagerBase::KinematicEntry& MovementManagerBase::KinematicEntry::operator =(KinematicEntry&&) = default;
  void MovementManagerBase::KinematicEntry::SetPhComp(PhysicComponent_Impl* iComp)
  {
    m_PhComp = iComp;
  }

  ObjectTable<MovementManagerBase::KinematicEntry>::Handle MovementManagerBase::CreateKinematic(PhysicComponent_Impl& iComp, KinematicEntry* iEntry)
  {
    eXl_ASSERT(m_ObjectToKinematicEntry.count(iComp.m_ObjectId) == 0);

    auto handle = m_KinematicEntries.Alloc();
    if (iEntry)
    {
      m_KinematicEntries.Get(handle) = *iEntry;
    }
    m_KinematicEntries.Get(handle).SetPhComp(&iComp);
    m_ObjectToKinematicEntry.insert(std::make_pair(iComp.m_ObjectId, handle));
    iComp.PushController(this);

    return handle;
  }

  MovementManagerBase::KinematicEntry& MovementManagerBase::GetKinematic(KinematicEntryHandle iHandle)
  {
    return m_KinematicEntries.Get(iHandle);
  }

  void MovementManagerBase::RemoveKinematic(KinematicEntryHandle iHandle)
  {
    KinematicEntry* entry = m_KinematicEntries.TryGet(iHandle);
    if (!entry)
    {
      return;
    }

    if (entry->m_ParentEntry.IsAssigned())
    {
      if (KinematicEntry* parentEntry = m_KinematicEntries.TryGet(KinematicEntryHandle(entry->m_ParentEntry)))
      {
        parentEntry->m_AttachedObjects.erase(parentEntry->GetPhComp());
      }
    }

    for (auto child : entry->m_AttachedObjects)
    {
      if (KinematicEntry* childEntry = m_KinematicEntries.TryGet(KinematicEntryHandle(child.second)))
      {
        childEntry->m_ParentEntry = KinematicEntryHandle();
      }
      child.first->Release();
    }
    entry->m_AttachedObjects.clear();
    entry->GetPhComp()->PopController(this);
    m_ObjectToKinematicEntry.erase(entry->GetPhComp()->m_ObjectId);
    m_KinematicEntries.Release(iHandle);
  }

  bool MovementManagerBase::Attach(KinematicEntryHandle iParent, PhysicComponent_Impl& iChild)
  {
    KinematicEntry* parentEntry = m_KinematicEntries.TryGet(iParent);

    if (!parentEntry)
    {
      return false;
    }

    if (parentEntry->m_AttachedObjects.count(&iChild) > 0)
    {
      return true;
    }

    KinematicEntryHandle childEntry;
    auto existingEntry = m_ObjectToKinematicEntry.find(iChild.m_ObjectId);
    if (existingEntry != m_ObjectToKinematicEntry.end())
    {
      childEntry = existingEntry->second;
      KinematicEntry& child = m_KinematicEntries.Get(childEntry);
      child.m_ParentEntry = iParent;
    }
    else
    {
      iChild.PushController(this);
    }
    iChild.AddRef();

    parentEntry->m_AttachedObjects.insert(std::make_pair(&iChild, childEntry));

    return true;
  }

  void MovementManagerBase::Detach(KinematicEntryHandle iParent, PhysicComponent_Impl& iChild)
  {
    KinematicEntry* parentEntry = m_KinematicEntries.TryGet(iParent);

    if (!parentEntry)
    {
      return;
    }

    auto childEntry = parentEntry->m_AttachedObjects.find(&iChild);
    if(childEntry == parentEntry->m_AttachedObjects.end())
    {
      return;
    }

    if (childEntry->second.IsAssigned())
    {
      if (KinematicEntry* child = m_KinematicEntries.TryGet(KinematicEntryHandle(childEntry->second)))
      {
        child->m_ParentEntry = ObjectTableHandle_Base();
      }
    }
    else
    {
      iChild.PopController(this);
    }
    parentEntry->m_AttachedObjects.erase(childEntry);
    iChild.Release();
  }

  Vector3f GetCorrectedDir(Vector3f const& iTouchingNormal, Vector3f iOldMove, float iDotDirNormal, float iCollisionDepth, bool iCanBounce)
  {
    Vector3f newMove = iOldMove;
    float oldMoveLen = newMove.Normalize();

    iDotDirNormal /= oldMoveLen;
    
    Vector3f perpDir = newMove - iTouchingNormal * iDotDirNormal;
    if (!iCanBounce && perpDir.Length() > Mathf::ZERO_TOLERANCE)
    {
      perpDir.Normalize();
      newMove = perpDir * iOldMove.Dot(perpDir) + iTouchingNormal * iOldMove.Dot(iTouchingNormal) * iCollisionDepth;
    }
    else
    {
      newMove = iTouchingNormal * iOldMove.Dot(iTouchingNormal) * iCollisionDepth;
    }

    return newMove;
  }

  bool MovementManagerBase::AttemptMove(PhysicComponent_Impl& iPhComp, 
    Vector3f const& iCurPos, 
    bool iCanBounce,
    Vector3f& ioNextPos, 
    std::function<bool(PhysicComponent_Impl*)>& iIgnore,
    Optional<Vector3f>& oCollideNormal)
  {
    bool originalMoveSucceeded = true;
    CollisionData collision;
    const uint32_t maxDeprojAttempts = 1;
    uint32_t numAttempts = maxDeprojAttempts;
    while (numAttempts > 0
      && iCurPos != ioNextPos
      && SweepTest(iPhComp, iCurPos, ioNextPos, collision, iIgnore, EngineCommon::s_MovementSensorMask))
    {
      if (collision.normal1To2 != Vector3f::ZERO)
      {
        oCollideNormal = collision.normal1To2;
        Vector3f const& touchingNormal = collision.normal1To2;
        Vector3f oldDir = ioNextPos - iCurPos;
        float dot = touchingNormal.Dot(oldDir);
        if (dot < 0.0)
        {
          originalMoveSucceeded = false;
          ioNextPos = iCurPos + GetCorrectedDir(touchingNormal, oldDir, dot, collision.depth, iCanBounce);
          ioNextPos.Z() = iCurPos.Z();
        }
        --numAttempts;
      }
      else
      {
        numAttempts = 0;
      }
    }

    return originalMoveSucceeded;
  }

  void MovementManagerBase::Step(PhysicsSystem* iSys, float iTime)
  {
    Transforms& transforms = *GetWorld().GetSystem<Transforms>();

    m_KinematicEntries.Iterate([this, iTime, &transforms](KinematicEntry& entry, ObjectTable<KinematicEntry>::Handle)
    {
      if (entry.m_ParentEntry.IsAssigned())
      {
        return;
      }

      if (!IsControlled(*entry.GetPhComp()))
      {
        return;
      }

      if (entry.UseTransform())
      {
        Matrix4f const& worldTrans = transforms.GetWorldTransform(entry.GetPhComp()->m_ObjectId);

        Vector3f curPhPos = GetPosition(*entry.GetPhComp());
        Vector3f const& curPos = MathTools::GetPosition(worldTrans);
        if (curPhPos == curPos)
        {
          if (!entry.m_Asleep)
          {
            ApplyLinearVelocity(*entry.GetPhComp(), Vector3f::ZERO, iTime);
            entry.m_Asleep = true;

            for (auto child : entry.m_AttachedObjects)
            {
              if (child.second.IsAssigned())
              {
                KinematicEntry& childEntry = m_KinematicEntries.Get(KinematicEntryHandle(child.second));
                childEntry.m_Asleep = true;
              }
              ApplyLinearVelocity(*child.first, Vector3f::ZERO, iTime);
            }
          }
        }
        else
        {
          Vector3f linVel = (curPos - curPhPos) / iTime;
          ApplyLinearVelocity(*entry.GetPhComp(), linVel, iTime);
          entry.m_Asleep = false;
          for (auto child : entry.m_AttachedObjects)
          {
            if (child.second.IsAssigned())
            {
              KinematicEntry& childEntry = m_KinematicEntries.Get(KinematicEntryHandle(child.second));
              childEntry.m_Asleep = true;
            }
            ApplyLinearVelocity(*child.first, linVel, iTime);
          }
        }
      }
      else
      {
        Vector3f curPos = GetPosition(*entry.GetPhComp());
        Vector3f correctedPos = curPos;

        if (entry.m_NeedDepenetration)
        {
          const uint32_t maxDepenAttempts = 4;
          uint32_t numAttempts = maxDepenAttempts;
          bool penetration = false;
          do
          {
            penetration = RecoverFromPenetration(*entry.GetPhComp(), correctedPos);
            --numAttempts;
          } while (penetration && numAttempts > 0);
          correctedPos.Z() = curPos.Z();
        }

        bool posWasCorrected = curPos != correctedPos;

        Vector3f candidateMove;
        Vector3f moveDir = candidateMove = entry.m_Dir * entry.m_Speed * iTime;
        if (posWasCorrected)
        {
          candidateMove = correctedPos - curPos;
          Vector3f touchingNormal = candidateMove;
          float depth = touchingNormal.Normalize();
          float dot = moveDir.Dot(touchingNormal);
          if (dot < 0.0)
          {
            moveDir = GetCorrectedDir(touchingNormal, moveDir, dot, depth, false);
          }
          candidateMove = correctedPos + moveDir - curPos;
        }
        else
        {
          candidateMove = moveDir;
        }

        if (moveDir.Length() > Mathf::ZERO_TOLERANCE)
        {
          std::function<bool(PhysicComponent_Impl*)> attachedFilter = [&entry](PhysicComponent_Impl* iComp)
          {
            if (iComp == entry.GetPhComp() 
              || entry.m_AttachedObjects.count(iComp) != 0
              || entry.m_ParentEntry == iComp->m_ObjectId)
            {
              return true;
            }

            if (entry.contactCb)
            {
              return !entry.contactCb(entry.GetPhComp()->m_ObjectId, iComp->m_ObjectId);
            }

            return false;
          };

          Optional<Vector3f> collideNormal;
          if (!entry.IsGhost())
          {
            Vector3f nextPos = curPos + candidateMove;
            bool moveSucceeded = AttemptMove(*entry.GetPhComp(),
              curPos,
              entry.m_CanBounce,
              nextPos,
              attachedFilter,
              collideNormal);
            if (collideNormal && entry.m_CanBounce)
            {
              auto newDir = MathTools::Reflect(entry.m_Dir, -(*collideNormal));
              newDir.Z() = entry.m_Dir.Z();
              entry.m_Dir = newDir;

            }
            if (!moveSucceeded)
            {
              candidateMove = (nextPos - curPos);
            }

          }
          if (!entry.m_CanBounce)
          {
            for (auto child : entry.m_AttachedObjects)
            {
              Vector3f curPos = GetPosition(*child.first);
              Vector3f nextPos = curPos + candidateMove;
              bool moveSucceeded = AttemptMove(*child.first, curPos, entry.m_CanBounce, nextPos, attachedFilter, collideNormal);
              if (!moveSucceeded)
              {
                Vector3f childCandidateMove = (nextPos - curPos);
                if (childCandidateMove.SquaredLength() < candidateMove.SquaredLength())
                {
                  candidateMove = childCandidateMove;
                }
              }
            }
          }
        }

        if (candidateMove.SquaredLength() > Mathf::ZERO_TOLERANCE)
        {
          ApplyLinearVelocity(*entry.GetPhComp(), candidateMove / iTime, iTime);
          entry.m_Asleep = false;
          for (auto child : entry.m_AttachedObjects)
          {
            if (child.second.IsAssigned())
            {
              KinematicEntry& childEntry = m_KinematicEntries.Get(KinematicEntryHandle(child.second));
              childEntry.m_Asleep = false;
            }
            ApplyLinearVelocity(*child.first, candidateMove / iTime, iTime);
          }
        }
        else if (!entry.m_Asleep)
        {
          ApplyLinearVelocity(*entry.GetPhComp(), Vector3f::ZERO, iTime);
          entry.m_Asleep = true;
          for (auto child : entry.m_AttachedObjects)
          {
            if (child.second.IsAssigned())
            {
              KinematicEntry& childEntry = m_KinematicEntries.Get(KinematicEntryHandle(child.second));
              childEntry.m_Asleep = true;
            }
            ApplyLinearVelocity(*child.first, Vector3f::ZERO, iTime);
          }
        }
      }
    });
  }
}