template <typename T>
SparseGameDataView<T> const* GameDataView<T>::GetSparseView() const
{
  return const_cast<GameDataView<T>*>(this)->GetSparseView();
}

inline uint32_t SparseGameDataAllocator::AllocateSlot_Inl(ObjectHandle iObject)
{
  uint32_t newSlot = m_ObjectHandles.size();
  m_ArchetypeHandle.push_back(ObjectTableHandle_Base());
  m_ObjectHandles.push_back(ObjectTableHandle_Base());
  m_IndexRef.m_ObjectToSlot.insert(std::make_pair(iObject, newSlot));
  m_IndexRef.m_WorldObjects.push_back(iObject);
  return newSlot;
}

inline ObjectTableHandle_Base SparseGameDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot) const
{
  ObjectTableHandle_Base mutableHandle = m_ObjectHandles[iSlot];
  ObjectTableHandle_Base archetypeHandle = m_ArchetypeHandle[iSlot];
  if (mutableHandle.IsAssigned())
  {
    return mutableHandle;
  }
  return archetypeHandle;
}

template <typename T>
SparseGameDataView<T>::SparseGameDataView(World& iWorld, SparseGameDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec)
  : GameDataView<T>(iWorld)
  , m_ObjectSpec(iObjectsSpec)
  , m_Alloc(iAlloc)
{}

template <typename T>
T const* SparseGameDataView<T>::Get(ObjectHandle iObject) const
{
  if (!this->m_World.IsObjectValid(iObject))
  {
    return nullptr;
  }
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    return nullptr;
  }
  ObjectTableHandle_Base constHandle = m_Alloc.GetDataFromSlot_Inl(slot);
  return this->m_ObjectSpec.TryGet(typename ObjectTable<T>::Handle(constHandle));
}

template <typename T>
T* SparseGameDataView<T>::Get(ObjectHandle iObject)
{
  if (!this->m_World.IsObjectValid(iObject))
  {
    return nullptr;
  }
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    return nullptr;
  }
  ObjectTableHandle_Base mutableHandle = m_Alloc.GetDataFromSlot(slot);
  return this->m_ObjectSpec.TryGet(typename ObjectTable<T>::Handle(mutableHandle));
}

template <typename T>
T const* SparseGameDataView<T>::GetDataForDeletion(ObjectHandle iObject)
{
  if (!this->m_World.IsObjectValid(iObject)
    && !this->m_World.IsObjectBeingDestroyed(iObject))
  {
    return nullptr;
  }

  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    return nullptr;
  }

  ObjectTableHandle_Base constHandle = const_cast<const SparseGameDataView<T>*>(this)->m_Alloc.GetDataFromSlot_Inl(slot);
  return this->m_ObjectSpec.TryGet(typename ObjectTable<T>::Handle(constHandle));
}

template <typename T>
T& SparseGameDataView<T>::GetOrCreate(ObjectHandle iObject)
{
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    slot = m_Alloc.AllocateSlot_Inl(iObject);
  }

  ObjectTableHandle_Base mutableHandle = m_Alloc.GetDataFromSlot(slot);
  return this->m_ObjectSpec.Get(typename ObjectTable<T>::Handle(mutableHandle));
}

template <typename T>
void SparseGameDataView<T>::Erase(ObjectHandle iObject)
{
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    return;
  }
  eXl_ASSERT(m_Alloc.m_ObjectHandles[slot].IsAssigned());
  m_Alloc.EraseSlot(slot);
}

template <typename T>
const SparseGameDataAllocator& SparseGameDataView<T>::GetAlloc() const
{
  return m_Alloc;
}

template <typename T>
template <typename Functor>
inline void SparseGameDataView<T>::Iterate(Functor const& iFn)
{
  for (uint32_t slot = 0; slot < m_Alloc.m_IndexRef.m_WorldObjects.size(); ++slot)
  {
    ObjectHandle object = m_Alloc.m_IndexRef.m_WorldObjects[slot];
    if (T* mutableData = Get(object))
    {
      iFn(object, *mutableData);
    }
  }
}

template <typename T>
template <typename Functor>
inline void SparseGameDataView<T>::Iterate(Functor const& iFn) const
{
  for (uint32_t slot = 0; slot < m_Alloc.m_IndexRef.m_WorldObjects.size(); ++slot)
  {
    ObjectHandle object = m_Alloc.m_IndexRef.m_WorldObjects[slot];
    if (T const* data = Get(object))
    {
      iFn(object, *data);
    }
  }
}

template <typename T>
SparsePropertySheetAllocator<T>::SparsePropertySheetAllocator(World& iWorld, Type const* iType)
  : SparseGameDataAllocator(m_Index, iType, m_ObjectsSpec.GetImplementation())
  , m_View(iWorld, *this, m_ObjectsSpec)
{
  m_ViewPtr = &m_View;
}

template <typename T>
ObjectTableHandle_Base SparsePropertySheetAllocator<T>::Alloc()
{
  return m_ObjectsSpec.Alloc();
}

template <typename T>
void SparsePropertySheetAllocator<T>::Release(ObjectTableHandle_Base iHandle)
{
  return m_ObjectsSpec.Release(typename ObjectTable<T>::Handle(iHandle));
}

template <typename T>
void SparsePropertySheetAllocator<T>::Clear()
{
  SparseGameDataAllocator::Clear();
  m_ObjectsSpec.Reset();
}