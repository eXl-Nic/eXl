/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <engine/physics/physiccomponent.hpp>

namespace eXl
{
  class World;

  class EXL_ENGINE_API MovementManagerBase
    : public ComponentManager
    , public KinematicController
  {
  public:

    enum class PhysicKind
    {
      Ghost,
      GhostAbsolute,
      Kinematic,
      KinematicAbsolute,
      Simulated
    };

    enum class StateFlags
    {
      DirLeft = 0,
      DirRight = 1,
      DirDown = 2,
      DirUp = 3,
      DirMask = 3
    };

    //struct Desc
    //{
    //  float size = 1.0;
    //  PhysicKind kind = PhysicKind::Ghost;
    //  bool IsSimulated() const { return kind == PhysicKind::Simulated; }
    //  bool IsKinematic() const { return !IsSimulated(); }
    //  bool IsGhost() const { return kind == PhysicKind::Ghost || kind == PhysicKind::GhostAbsolute; }
    //  bool IsSolid() const { return !IsGhost(); }
    //};

  protected:

    struct KinematicEntry
    {
      KinematicEntry();
      ~KinematicEntry();
      KinematicEntry(KinematicEntry const&);
      KinematicEntry& operator =(KinematicEntry const&);
      KinematicEntry(KinematicEntry&&);
      KinematicEntry& operator =(KinematicEntry&&);

      ContactFilterCallback contactCb;

      Vector3f m_Dir;
      float m_Speed;
      bool m_NeedDepenetration = false;
      bool m_CanBounce = false;
      bool m_Asleep = false;

      PhysicKind m_Kind;

      ObjectTableHandle_Base m_ParentEntry;
      UnorderedMap<PhysicComponent_Impl*, ObjectTableHandle_Base> m_AttachedObjects;
      bool UseTransform() const { return m_Kind == PhysicKind::GhostAbsolute || m_Kind == PhysicKind::KinematicAbsolute; }
      bool UseSpeed() const { return !UseTransform(); }
      bool IsGhost() const { return m_Kind == PhysicKind::Ghost || m_Kind == PhysicKind::GhostAbsolute; }
      bool IsSolid() const { return !IsGhost(); }

      PhysicComponent_Impl* GetPhComp() const { return m_PhComp.get(); }
      void SetPhComp(PhysicComponent_Impl*);

    private:
      IntrusivePtr<PhysicComponent_Impl> m_PhComp;
    };

    typedef ObjectTable<KinematicEntry>::Handle KinematicEntryHandle;

  private:
    ObjectTable<KinematicEntry> m_KinematicEntries;
    UnorderedMap<ObjectHandle, KinematicEntryHandle> m_ObjectToKinematicEntry;
  protected:

    ObjectTable<KinematicEntry>::Handle CreateKinematic(PhysicComponent_Impl& iComp, KinematicEntry* Entry);
    KinematicEntry& GetKinematic(KinematicEntryHandle);
    void RemoveKinematic(KinematicEntryHandle);

    bool Attach(KinematicEntryHandle iParent, PhysicComponent_Impl& iChild);
    void Detach(KinematicEntryHandle iParent, PhysicComponent_Impl& iChild);

    bool AttemptMove(PhysicComponent_Impl& iPhComp, 
      Vector3f const& iCurPos, 
      bool iCanBounce, 
      Vector3f& ioNextPos, 
      std::function<bool(PhysicComponent_Impl*)>& iIgnore,
      Optional<Vector3f>& oCollideNormal);
    void Step(PhysicsSystem* iSys, float iTime) override;
  };
}