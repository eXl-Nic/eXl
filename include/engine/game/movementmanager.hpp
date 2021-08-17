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
      boost::optional<Vector3f>& oCollideNormal);
    void Step(PhysicsSystem* iSys, float iTime) override;
  };
}