#include <engine/common/gamedata.hpp>

namespace eXl
{
  GameDataAllocatorBase::GameDataAllocatorBase(ObjectDataIndex& iIndex, Type const* iType)
    : m_IndexRef(iIndex)
    , m_Type(iType)
  {}

  void GameDataAllocatorBase::Clear()
  {
    m_IndexRef.m_WorldObjects.clear();
    m_IndexRef.m_ObjectToSlot.clear();
  }

  DenseGameDataAllocator::DenseGameDataAllocator(ObjectDataIndex& iIndex, Type const* iType)
    : GameDataAllocatorBase(iIndex, iType)
  {}

  uint32_t DenseGameDataAllocator::AllocateSlot(ObjectHandle iHandle)
  {
    return AllocateSlot_Inl(iHandle);
  }

  void DenseGameDataAllocator::EraseSlot(uint32_t iSlot)
  {
    if (iSlot < m_IndexRef.m_WorldObjects.size() && m_IndexRef.m_WorldObjects[iSlot].IsAssigned())
    {
      Release(GetDataFromSlot_Inl(iSlot));
      ObjectHandle object = m_IndexRef.m_WorldObjects[iSlot];
      m_IndexRef.m_WorldObjects[iSlot] = ObjectHandle();
      m_IndexRef.m_ObjectToSlot.erase(object);
    }
  }

  void DenseGameDataAllocator::GarbageCollect(World& iWorld)
  {
    for (int32_t slot = 0; slot < static_cast<int32_t>(m_IndexRef.m_WorldObjects.size()); ++slot)
    {
      ObjectHandle object = m_IndexRef.m_WorldObjects[slot];
      if (!object.IsAssigned() || iWorld.IsObjectValid(object))
      {
        continue;
      }

      EraseSlot(slot);
    }
  }

  ObjectTableHandle_Base DenseGameDataAllocator::GetDataFromSlot(uint32_t iSlot)
  {
    return GetDataFromSlot_Inl(iSlot);
  }

  ObjectTableHandle_Base DenseGameDataAllocator::GetDataFromSlot(uint32_t iSlot) const
  {
    return GetDataFromSlot_Inl(iSlot);
  }
  
  void DenseGameDataAllocator::Clear()
  {
    GameDataAllocatorBase::Clear();
  }

  SparseGameDataAllocator::SparseGameDataAllocator(ObjectDataIndex& iIndex, Type const* iType, ObjectTable_Data& iObjects)
    : GameDataAllocatorBase(iIndex, iType)
    , m_ObjectData(iObjects)
  {}

  void SparseGameDataAllocator::EraseSlot(uint32_t iSlot)
  {
    ObjectHandle object = m_IndexRef.m_WorldObjects[iSlot];

    if (m_ObjectHandles.size() > 1
      && iSlot != m_ObjectHandles.size() - 1)
    {
      ObjectHandle lastObject = m_IndexRef.m_WorldObjects.back();
      std::swap(m_ObjectHandles[iSlot], m_ObjectHandles.back());
      std::swap(m_ArchetypeHandle[iSlot], m_ArchetypeHandle.back());
      std::swap(m_IndexRef.m_WorldObjects[iSlot], m_IndexRef.m_WorldObjects.back());

      m_IndexRef.m_ObjectToSlot[lastObject] = iSlot;
    }

    if (!m_ArchetypeHandle.back().IsAssigned()
      || (m_ObjectHandles.back().IsAssigned() 
        && m_ObjectHandles.back() != m_ArchetypeHandle.back()))
    {
      Release(m_ObjectHandles.back());
    }

    m_ObjectHandles.pop_back();
    m_ArchetypeHandle.pop_back();
    m_IndexRef.m_WorldObjects.pop_back();
    m_IndexRef.m_ObjectToSlot.erase(object);
  }

  void SparseGameDataAllocator::GarbageCollect(World& iWorld)
  {
    for (int32_t slot = 0; slot < static_cast<int32_t>(m_IndexRef.m_WorldObjects.size()); ++slot)
    {
      ObjectHandle object = m_IndexRef.m_WorldObjects[slot];
      if (iWorld.IsObjectValid(object))
      {
        continue;
      }

      EraseSlot(slot);

      --slot;
    }
  }

  void SparseGameDataAllocator::Clear()
  {
    GameDataAllocatorBase::Clear();
    m_ObjectHandles.clear();
    m_ArchetypeHandle.clear();
  }

  uint32_t SparseGameDataAllocator::AllocateSlot(ObjectHandle iHandle)
  {
    return AllocateSlot_Inl(iHandle);
  }

  ObjectTableHandle_Base SparseGameDataAllocator::GetDataFromSlot(uint32_t iSlot)
  {
    ObjectTableHandle_Base& mutableHandle = m_ObjectHandles[iSlot];
    ObjectTableHandle_Base archetypeHandle = m_ArchetypeHandle[iSlot];
    if (mutableHandle.IsAssigned() && archetypeHandle != mutableHandle)
    {
      return mutableHandle;
    }
    if (!mutableHandle.IsAssigned())
    {
      mutableHandle = Alloc();
      if (archetypeHandle.IsAssigned() && m_Type)
      {
        void* mutableCopy = m_ObjectData.Get(mutableHandle);
        void const* archetypeData = m_ObjectData.Get(archetypeHandle);
        m_Type->Copy(archetypeData, mutableCopy);
      }
      return mutableHandle;
    }

    return ObjectTableHandle_Base();
  }

  ObjectTableHandle_Base SparseGameDataAllocator::GetDataFromSlot(uint32_t iSlot) const
  {
    return GetDataFromSlot_Inl(iSlot);
  }
}