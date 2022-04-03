/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

    bool SweepTest(PhysicComponent_Impl& iComp, Vec3 const& iFrom, Vec3 const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore = std::function<bool(PhysicComponent_Impl*)>(), uint16_t iMask = -1);

    Vec3 GetPosition(PhysicComponent_Impl& iComp) const;

    bool IsControlled(PhysicComponent_Impl& iComp);

    //void SetTransform(PhysicComponent_Impl& iComp, Quaternion const& iOrient, Vec3 const& iPos);

    void ApplyLinearVelocity(PhysicComponent_Impl& iComp, Vec3 const& iLinVel, float iTimeStep);

    bool RecoverFromPenetration(PhysicComponent_Impl& iComp, Vec3& OutNewPos);

  private:
    KinematicController(KinematicController const&);
    KinematicController& operator=(KinematicController const&);
  };

}