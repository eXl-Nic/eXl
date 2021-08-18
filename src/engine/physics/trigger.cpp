/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/physics/trigger.hpp>

#include <engine/physics/physicsys_impl.hpp>
#include <engine/physics/physiccomponent_impl.hpp>


btTriggerGhostObject::btTriggerGhostObject()
{

}

void btTriggerGhostObject::SetChangeCB(std::function<void()> iChangeCallback)
{
  m_ChangeCallback = std::move(iChangeCallback);
}

btTriggerGhostObject::~btTriggerGhostObject()
{

}

void btTriggerGhostObject::addOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btBroadphaseProxy* thisProxy)
{
  btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
  btAssert(otherObject);

  eXl::PhysicComponent_Impl* phComp = reinterpret_cast<eXl::PhysicComponent_Impl*>(otherObject->getUserPointer());
  if (phComp == nullptr)
  {
    return;
  }
  eXl::ObjectHandle obj = phComp->m_ObjectId;
    
  if (m_Inside.count(obj) == 0)
  {
    m_Inside.insert(obj);
    m_Added.push_back(obj);
    if (m_Removed.size() == 0 && m_Added.size() == 1)
    {
      m_ChangeCallback();
    }
  }
}

void btTriggerGhostObject::removeOverlappingObjectInternal(btBroadphaseProxy* otherProxy, btDispatcher* dispatcher, btBroadphaseProxy* thisProxy)
{
  btCollisionObject* otherObject = (btCollisionObject*)otherProxy->m_clientObject;
  btAssert(otherObject);

  eXl::PhysicComponent_Impl* phComp = reinterpret_cast<eXl::PhysicComponent_Impl*>(otherObject->getUserPointer());
  if (phComp == nullptr)
  {
    return;
  }
  eXl::ObjectHandle obj = phComp->m_ObjectId;

  if (m_Inside.count(obj) > 0)
  {
    m_Inside.erase(obj);
    m_Removed.push_back(obj);
    if (m_Removed.size() == 1 && m_Added.size() == 0)
    {
      m_ChangeCallback();
    }
  }
}

eXl::SmallVector<eXl::ObjectHandle, 1> btTriggerGhostObject::StealAdded()
{
  eXl::SmallVector<eXl::ObjectHandle, 1> retVec;
  retVec.swap(m_Added);
  return retVec;
}
eXl::SmallVector<eXl::ObjectHandle, 1> btTriggerGhostObject::StealRemoved()
{
  eXl::SmallVector<eXl::ObjectHandle, 1> retVec;
  retVec.swap(m_Removed);
  return retVec;
}


namespace eXl
{
  TriggerManager::TriggerManager(PhysicsSystem_Impl& iImpl)
    : m_Impl(iImpl)
  {

  }

  void TriggerManager::AddTrigger(ObjectHandle iObj, TriggerDef const& iGeom, TriggerCallbackHandle iCallback, ContactFilterCallback iFilter)
  {
    ObjectTable<CallbackEntry>::Handle cbHandle(iCallback);
    eXl_ASSERT_REPAIR_RET(m_Callbacks.IsValid(cbHandle), );

    CallbackEntry& cbEntry = m_Callbacks.Get(cbHandle);

    auto triggerHandle = cbEntry.m_Entries.Alloc();
    TriggerEntry& triggerEntry = cbEntry.m_Entries.Get(triggerHandle);
    if (iFilter)
    {
      m_Impl.AddContactCb(iObj, std::move(iFilter));
    }

    TriggerKey key = std::make_pair(cbHandle, triggerHandle);

    triggerEntry.object = iObj;
    triggerEntry.contactCb = iFilter;
    triggerEntry.triggerObj.setCollisionShape(m_Impl.m_ShapesCache.MakeGeom(iGeom.m_Geom, nullptr));
    triggerEntry.triggerObj.setCollisionFlags(triggerEntry.triggerObj.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    triggerEntry.triggerObj.SetChangeCB([this, key]
    {
      m_Dirty.insert(key);
    });

    m_Impl.m_dynamicsWorld->addCollisionObject(&triggerEntry.triggerObj, iGeom.m_Category, iGeom.m_Filter);

    m_ObjectToEntry.insert(std::make_pair(iObj, key));
    m_ObjectToGhost.insert(std::make_pair(iObj, &triggerEntry.triggerObj));
  }

  void TriggerManager::DeleteComponent(ObjectHandle iObj)
  {
    auto iter = m_ObjectToEntry.find(iObj);
    if (iter != m_ObjectToEntry.end())
    {
      TriggerKey key = iter->second;
      CallbackEntry& entry = m_Callbacks.Get(key.first);

      TriggerEntry& trigger = entry.m_Entries.Get(key.second);
      if (trigger.contactCb)
      {
        m_Impl.RemoveContactCb(iObj);
      }
      m_Impl.m_dynamicsWorld->removeCollisionObject(&trigger.triggerObj);
      entry.m_Entries.Release(key.second);

      m_ObjectToEntry.erase(iter);
      m_ObjectToGhost.erase(iObj);
    }
  }

  void TriggerManager::Tick(World& iWorld, float)
  {
    if (m_Dirty.empty())
    {
      return;
    }

    TriggerCallbackHandle curHandle;
    CallbackEntry* curEntry = nullptr;
    for (auto const& dirtyTrigger : m_Dirty)
    {
      if (curHandle != dirtyTrigger.first)
      {
        curEntry = m_Callbacks.TryGet(dirtyTrigger.first);
        curHandle = dirtyTrigger.first;
      }
      eXl_ASSERT_REPAIR_BEGIN(curEntry != nullptr) { continue; }

      TriggerEntry* trigger = curEntry->m_Entries.TryGet(dirtyTrigger.second);
      eXl_ASSERT_REPAIR_BEGIN(trigger != nullptr) { continue; }

      auto newEnter = trigger->triggerObj.StealAdded();
      auto newLeave = trigger->triggerObj.StealRemoved();

      for (auto obj : newEnter)
      {
        if (iWorld.IsObjectValid(obj) && obj != trigger->object)
        {
          curEntry->m_NewEnter.push_back(std::make_pair(trigger->object, obj));
        }
      }

      for (auto obj : newLeave)
      {
        if (iWorld.IsObjectValid(obj) && obj != trigger->object)
        {
          curEntry->m_NewLeave.push_back(std::make_pair(trigger->object, obj));
        }
      }
    }

    m_Callbacks.Iterate([this](CallbackEntry& iEntry, ObjectTable<CallbackEntry>::Handle)
    {
      iEntry.m_Callback->OnLeave(iEntry.m_NewLeave);
      iEntry.m_Callback->OnEnter(iEntry.m_NewEnter);
      iEntry.m_NewLeave.clear();
      iEntry.m_NewEnter.clear();
    });

    m_Dirty.clear();
  }

  TriggerCallbackHandle TriggerManager::AddCallback(std::unique_ptr<TriggerCallback> iCallback)
  {
    auto handle = m_Callbacks.Alloc();
    m_Callbacks.Get(handle).m_Callback = std::move(iCallback);

    return handle;
  }
}