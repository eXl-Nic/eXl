#pragma once

#include <engine/common/world.hpp>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <boost/container/flat_set.hpp>

#include <math/mathtools.hpp>
#include "physicsdef.hpp"

class	btTriggerGhostObject : public btGhostObject
{
public:

  btTriggerGhostObject();

  virtual ~btTriggerGhostObject();

  void SetChangeCB(std::function<void()> iChangeCallback);

  void addOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy = 0);
  void removeOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btDispatcher* dispatcher, btBroadphaseProxy* thisProxy = 0);

  eXl::SmallVector<eXl::ObjectHandle, 1> StealAdded();
  eXl::SmallVector<eXl::ObjectHandle, 1> StealRemoved();

protected:
  std::function<void()> m_ChangeCallback;
  eXl::UnorderedSet<eXl::ObjectHandle> m_Inside;
  eXl::SmallVector<eXl::ObjectHandle, 1> m_Added;
  eXl::SmallVector<eXl::ObjectHandle, 1> m_Removed;
};

namespace eXl
{
  struct PhysicsSystem_Impl;

  class TriggerManager
  {

  public:

    TriggerManager(PhysicsSystem_Impl& iImpl);

    void AddTrigger(ObjectHandle iObj, TriggerDef const& iDef, TriggerCallbackHandle iCallback, ContactFilterCallback iFilter = ContactFilterCallback());
    void DeleteComponent(ObjectHandle iObj);

    inline void UpdateTransform(ObjectHandle iObj, Matrix4f const& iTrans)
    {
      auto iter = m_ObjectToGhost.find(iObj);
      if (iter != m_ObjectToGhost.end())
      {
        Vector3f const& pos = MathTools::GetPosition(iTrans);
        btTransform newTransform;
        newTransform.getBasis().setValue(
          iTrans.m_Data[0], iTrans.m_Data[1], iTrans.m_Data[2],
          iTrans.m_Data[4], iTrans.m_Data[5], iTrans.m_Data[6],
          iTrans.m_Data[8], iTrans.m_Data[9], iTrans.m_Data[10]);
        newTransform.setOrigin(btVector3(pos.X(), pos.Y(), pos.Z()));
        iter->second->setWorldTransform(newTransform);
      }
    }

    void Tick(World& iWorld, float);

    TriggerCallbackHandle AddCallback(std::unique_ptr<TriggerCallback> iCallback);

  protected:

    struct TriggerEntry
    {
      btTriggerGhostObject triggerObj;
      ContactFilterCallback contactCb;
      ObjectHandle object;
    };

    struct CallbackEntry
    {
      std::unique_ptr<TriggerCallback> m_Callback;
      ObjectTable<TriggerEntry> m_Entries;
      Vector<TriggerCallback::ObjectPair> m_NewEnter;
      Vector<TriggerCallback::ObjectPair> m_NewLeave;
      
    };

    using TriggerKey = std::pair<ObjectTable<CallbackEntry>::Handle, ObjectTable<TriggerEntry>::Handle>;
    boost::container::flat_set<TriggerKey, std::less<TriggerKey>, Allocator<TriggerKey>> m_Dirty;

    ObjectTable<CallbackEntry> m_Callbacks;
    UnorderedMap<ObjectHandle, TriggerKey> m_ObjectToEntry;
    UnorderedMap<ObjectHandle, btTriggerGhostObject*> m_ObjectToGhost;

    PhysicsSystem_Impl& m_Impl;
  };
}