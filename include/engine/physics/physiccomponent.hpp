#pragma once

#include <engine/enginelib.hpp>
#include <engine/common/object.hpp>
#include "physicsdef.hpp"

namespace eXl
{
  class PhysicsSystem;
  struct PhysicComponent_Impl;

  //class EXL_ENGINE_API PhysicComponent : public HeapObject
  //{
  //  ADD_WEAKREF(PhysicComponent, EXL_ENGINE_API);
  //public:
  //
  //  PhysicComponent(PhysicComponent_Impl*);
  //  ~PhysicComponent();
  //
  //  PhysicComponent_Impl* GetImpl() const { return m_Impl.get(); };
  //
  //private:
  //
  //  PhysicComponent(PhysicComponent const&);
  //  PhysicComponent& operator=(PhysicComponent const&);
  //  IntrusivePtr<PhysicComponent_Impl> m_Impl;
  //};

  // Motion controlled outside of the physic engine.
  class EXL_ENGINE_API KinematicController : public HeapObject
  {
    friend class PhysicInternalActionInterface;
  public:
    
    ~KinematicController();

  protected:
    KinematicController();

    friend class PhysicsSystem;

    virtual void Step(PhysicsSystem* iSys, float iTime)=0;

    bool SweepTest(PhysicComponent_Impl& iComp, Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore = std::function<bool(PhysicComponent_Impl*)>(), uint16_t iMask = -1);

    Vector3f GetPosition(PhysicComponent_Impl& iComp) const;

    bool IsControlled(PhysicComponent_Impl& iComp);

    //void SetTransform(PhysicComponent_Impl& iComp, Quaternionf const& iOrient, Vector3f const& iPos);

    void ApplyLinearVelocity(PhysicComponent_Impl& iComp, Vector3f const& iLinVel, float iTimeStep);

    bool RecoverFromPenetration(PhysicComponent_Impl& iComp, Vector3f& OutNewPos);

  private:
    KinematicController(KinematicController const&);
    KinematicController& operator=(KinematicController const&);
  };

}