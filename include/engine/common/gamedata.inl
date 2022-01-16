template <typename T>
DenseGameDataView<T> const* GameDataView<T>::GetDenseView() const
{
  return const_cast<GameDataView<T>*>(this)->GetDenseView();
}

template <typename T>
SparseGameDataView<T> const* GameDataView<T>::GetSparseView() const
{
  return const_cast<GameDataView<T>*>(this)->GetSparseView();
}

template <typename T>
template <typename Functor>
void GameDataView<T>::Iterate(Functor const& iFn)
{
  if (auto* denseView = GetDenseView())
  {
    denseView->DenseGameDataView<T>::Iterate(iFn);
  }
  else if (auto* sparseView = GetSparseView())
  {
    sparseView->SparseGameDataView<T>::Iterate(iFn);
  }
}

template <typename T>
template <typename Functor>
void GameDataView<T>::Iterate(Functor const& iFn) const
{
  if (auto* denseView = GetDenseView())
  {
    denseView->DenseGameDataView<T>::Iterate(iFn);
  }
  else if (auto* sparseView = GetSparseView())
  {
    sparseView->SparseGameDataView<T>::Iterate(iFn);
  }
}

inline uint32_t DataAllocatorBase::GetSlot(ObjectHandle iHandle) const
{
  auto iter = m_ObjectToSlot.find(iHandle);
  return m_ObjectToSlot.end() != iter ? iter->second : -1;
}

inline uint32_t DenseDataAllocator::AllocateSlot_Inl(ObjectHandle iObject)
{
  ObjectTableHandle_Base handle = Alloc();
  uint32_t newSlot = *reinterpret_cast<uint32_t*>(&handle);
  while (m_WorldObjects.size() <= newSlot)
  {
    m_WorldObjects.push_back(ObjectHandle());
  }
  m_WorldObjects[newSlot] = iObject;
  m_ObjectToSlot.insert(std::make_pair(iObject, newSlot));
  return newSlot;
}

inline ObjectTableHandle_Base DenseDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot)
{
  return reinterpret_cast<ObjectTableHandle_Base&>(iSlot);
}

inline ObjectTableHandle_Base DenseDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot) const
{
  return reinterpret_cast<ObjectTableHandle_Base&>(iSlot);
}

inline uint32_t SparseDataAllocator::AllocateSlot_Inl(ObjectHandle iObject)
{
  uint32_t newSlot = m_ObjectHandles.size();
  m_ArchetypeHandle.push_back(ObjectTableHandle_Base());
  m_ObjectHandles.push_back(ObjectTableHandle_Base());
  m_ObjectToSlot.insert(std::make_pair(iObject, newSlot));
  m_WorldObjects.push_back(iObject);
  return newSlot;
}

inline ObjectTableHandle_Base SparseDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot) const
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
DenseGameDataView<T>::DenseGameDataView(World& iWorld, DenseDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec)
  : GameDataView<T>(iWorld, iObjectsSpec)
  , m_Alloc(iAlloc)
{}

template <typename T>
T const* DenseGameDataView<T>::Get(ObjectHandle iObject) const
{
  return const_cast<DenseGameDataView<T>*>(this)->Get(iObject);
}

template <typename T>
T* DenseGameDataView<T>::Get(ObjectHandle iObject)
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

  return &this->m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.GetDataFromSlot_Inl(slot)));
}

template <typename T>
T const* DenseGameDataView<T>::GetDataForDeletion(ObjectHandle iObject)
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

  return &this->m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.GetDataFromSlot_Inl(slot)));
}

template <typename T>
T& DenseGameDataView<T>::GetOrCreate(ObjectHandle iObject)
{
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    slot = m_Alloc.AllocateSlot_Inl(iObject);
  }

  return this->m_ObjectSpec.Get(reinterpret_cast<typename ObjectTable<T>::Handle&>(slot));
}

template <typename T>
void DenseGameDataView<T>::Erase(ObjectHandle iObject)
{
  uint32_t slot = m_Alloc.GetSlot(iObject);
  if (slot == -1)
  {
    return;
  }

  m_Alloc.EraseSlot(slot);
}

template <typename T>
const DataAllocatorBase& DenseGameDataView<T>::GetAlloc() const
{
  return m_Alloc;
}

template <typename T>
SparseGameDataView<T>::SparseGameDataView(World& iWorld, SparseDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec)
  : GameDataView<T>(iWorld, iObjectsSpec)
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
const DataAllocatorBase& SparseGameDataView<T>::GetAlloc() const
{
  return m_Alloc;
}

template <typename T>
template <typename Functor>
inline void DenseGameDataView<T>::Iterate(Functor const& iFn)
{
  this->m_ObjectSpec.Iterate([&iFn, this](T& iData, typename ObjectTable<T>::Handle iHandle)
    {
      uint32_t slot = iHandle.GetId();
      ObjectHandle object = m_Alloc.m_WorldObjects[slot];
      if (this->m_World.IsObjectValid(object))
      {
        iFn(object, iData);
      }
    });
}

template <typename T>
template <typename Functor>
inline void DenseGameDataView<T>::Iterate(Functor const& iFn) const
{
  this->m_ObjectSpec.Iterate([&iFn, this](T const& iData, typename ObjectTable<T>::Handle iHandle)
    {
      uint32_t slot = iHandle.GetId();
      ObjectHandle object = m_Alloc.m_WorldObjects[slot];
      if (this->m_World.IsObjectValid(object))
      {
        iFn(object, iData);
      }
    });
}

template <typename T>
template <typename Functor>
inline void SparseGameDataView<T>::Iterate(Functor const& iFn)
{
  for (uint32_t slot = 0; slot < m_Alloc.m_WorldObjects.size(); ++slot)
  {
    ObjectHandle object = m_Alloc.m_WorldObjects[slot];
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
  for (uint32_t slot = 0; slot < m_Alloc.m_WorldObjects.size(); ++slot)
  {
    ObjectHandle object = m_Alloc.m_WorldObjects[slot];
    if (T const* data = Get(object))
    {
      iFn(object, *data);
    }
  }
}

template <typename T>
DensePropertySheetAllocator<T>::DensePropertySheetAllocator(World& iWorld, Type const* iType)
  : DenseDataAllocator(iType, m_ObjectsSpec.GetImplementation())
  , m_View(iWorld, *this, m_ObjectsSpec)
{
  m_ViewPtr = &m_View;
}

template <typename T>
ObjectTableHandle_Base DensePropertySheetAllocator<T>::Alloc()
{
  ObjectTableHandle_Base handle = GetDataFromSlot_Inl(m_ObjectData.m_Ids.Peek());
  if (!m_ObjectData.IsValid(handle))
  {
    m_ObjectsSpec.Alloc();
  }
  else
  {
    new(m_ObjectData.Get(handle)) T;
  }
  return handle;
}

template <typename T>
void DensePropertySheetAllocator<T>::Release(ObjectTableHandle_Base iHandle)
{
  m_ObjectsSpec.Get(typename ObjectTable<T>::Handle(iHandle)).~T();
  m_ObjectData.m_Ids.Return(iHandle.GetId());
}

template <typename T>
void DensePropertySheetAllocator<T>::Clear()
{
  DenseDataAllocator::Clear();
  m_ObjectsSpec.Reset();
}

template <typename T>
SparsePropertySheetAllocator<T>::SparsePropertySheetAllocator(World& iWorld, Type const* iType)
  : SparseDataAllocator(iType, m_ObjectsSpec.GetImplementation())
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
  SparseDataAllocator::Clear();
  m_ObjectsSpec.Reset();
}
