#include <engine/common/gamedata.hpp>

namespace eXl
{
  DataAllocatorBase::DataAllocatorBase(Type const* iType, ObjectTable_Data& iObjects)
    : m_Type(iType)
    , m_ObjectData(iObjects)
  {}

  void DataAllocatorBase::Clear()
  {
    m_WorldObjects.clear();
    m_ObjectToSlot.clear();
  }

  DenseDataAllocator::DenseDataAllocator(Type const* iType, ObjectTable_Data& iObjects)
    : DataAllocatorBase(iType, iObjects)
  {}

  uint32_t DenseDataAllocator::AllocateSlot(ObjectHandle iHandle)
  {
    return AllocateSlot_Inl(iHandle);
  }

  void DenseDataAllocator::EraseSlot(uint32_t iSlot)
  {
    if (iSlot < m_WorldObjects.size() && m_WorldObjects[iSlot].IsAssigned())
    {
      Release(GetDataFromSlot_Inl(iSlot));
      ObjectHandle object = m_WorldObjects[iSlot];
      m_WorldObjects[iSlot] = ObjectHandle();
      m_ObjectToSlot.erase(object);
    }
  }

  void DenseDataAllocator::GarbageCollect(World& iWorld)
  {
    for (int32_t slot = 0; slot < static_cast<int32_t>(m_WorldObjects.size()); ++slot)
    {
      ObjectHandle object = m_WorldObjects[slot];
      if (!object.IsAssigned() || iWorld.IsObjectValid(object))
      {
        continue;
      }

      EraseSlot(slot);
    }
  }

  ObjectTableHandle_Base DenseDataAllocator::GetDataFromSlot(uint32_t iSlot)
  {
    return GetDataFromSlot_Inl(iSlot);
  }

  ObjectTableHandle_Base DenseDataAllocator::GetDataFromSlot(uint32_t iSlot) const
  {
    return GetDataFromSlot_Inl(iSlot);
  }
  
  void DenseDataAllocator::Clear()
  {
    DataAllocatorBase::Clear();
  }

  SparseDataAllocator::SparseDataAllocator(Type const* iType, ObjectTable_Data& iObjects)
    : DataAllocatorBase(iType, iObjects)
  {}

  void SparseDataAllocator::EraseSlot(uint32_t iSlot)
  {
    ObjectHandle object = m_WorldObjects[iSlot];

    if (m_ObjectHandles.size() > 1
      && iSlot != m_ObjectHandles.size() - 1)
    {
      ObjectHandle lastObject = m_WorldObjects.back();
      std::swap(m_ObjectHandles[iSlot], m_ObjectHandles.back());
      std::swap(m_ArchetypeHandle[iSlot], m_ArchetypeHandle.back());
      std::swap(m_WorldObjects[iSlot], m_WorldObjects.back());

      m_ObjectToSlot[lastObject] = iSlot;
    }

    if (!m_ArchetypeHandle.back().IsAssigned()
      || (m_ObjectHandles.back().IsAssigned() 
        && m_ObjectHandles.back() != m_ArchetypeHandle.back()))
    {
      Release(m_ObjectHandles.back());
    }

    m_ObjectHandles.pop_back();
    m_ArchetypeHandle.pop_back();
    m_WorldObjects.pop_back();
    m_ObjectToSlot.erase(object);
  }

  void SparseDataAllocator::GarbageCollect(World& iWorld)
  {
    for (int32_t slot = 0; slot < static_cast<int32_t>(m_WorldObjects.size()); ++slot)
    {
      ObjectHandle object = m_WorldObjects[slot];
      if (iWorld.IsObjectValid(object))
      {
        continue;
      }

      EraseSlot(slot);

      --slot;
    }
  }

  void SparseDataAllocator::Clear()
  {
    DataAllocatorBase::Clear();
    m_ObjectHandles.clear();
    m_ArchetypeHandle.clear();
  }

  uint32_t SparseDataAllocator::AllocateSlot(ObjectHandle iHandle)
  {
    return AllocateSlot_Inl(iHandle);
  }

  ObjectTableHandle_Base SparseDataAllocator::GetDataFromSlot(uint32_t iSlot)
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
        void* mutableCopy = m_ObjectData.Get(m_ObjectHandles[iSlot]);
        void const* archetypeData = m_ObjectData.Get(m_ObjectHandles[iSlot]);
        m_Type->Copy(archetypeData, mutableCopy);
      }
      return mutableHandle;
    }

    return ObjectTableHandle_Base();
  }

  ObjectTableHandle_Base SparseDataAllocator::GetDataFromSlot(uint32_t iSlot) const
  {
    return GetDataFromSlot_Inl(iSlot);
  }
}