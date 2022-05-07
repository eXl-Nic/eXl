template <typename T>
DenseGameDataView<T> const* GameDataView<T>::GetDenseView() const
{
  return const_cast<GameDataView<T>*>(this)->GetDenseView();
}

inline uint32_t DenseGameDataAllocator::AllocateSlot_Inl(ObjectHandle iObject)
{
  ObjectTableHandle_Base handle = Alloc();
  uint32_t newSlot = *reinterpret_cast<uint32_t*>(&handle);
  while (m_IndexRef.m_WorldObjects.size() <= newSlot)
  {
    m_IndexRef.m_WorldObjects.push_back(ObjectHandle());
  }
  m_IndexRef.m_WorldObjects[newSlot] = iObject;
  m_IndexRef.m_ObjectToSlot.insert(std::make_pair(iObject, newSlot));
  return newSlot;
}

inline ObjectTableHandle_Base DenseGameDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot)
{
  return reinterpret_cast<ObjectTableHandle_Base&>(iSlot);
}

inline ObjectTableHandle_Base DenseGameDataAllocator::GetDataFromSlot_Inl(uint32_t iSlot) const
{
  return reinterpret_cast<ObjectTableHandle_Base&>(iSlot);
}


template <typename T>
DenseGameDataView<T>::DenseGameDataView(World& iWorld, DenseGameDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec)
  : GameDataView<T>(iWorld)
  , m_ObjectSpec(iObjectsSpec)
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
  eXl_ASSERT_REPAIR_BEGIN(this->m_World.IsObjectValid(iObject))
  {
    static T s_Dummy;
    return s_Dummy;
  }

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
const DenseGameDataAllocator& DenseGameDataView<T>::GetAlloc() const
{
  return m_Alloc;
}

template <typename T>
template <typename Functor>
inline void DenseGameDataView<T>::Iterate(Functor const& iFn)
{
  this->m_ObjectSpec.Iterate([&iFn, this](typename ObjectTable<T>::Handle iHandle, T& iData)
    {
      uint32_t slot = iHandle.GetId();
      ObjectHandle object = m_Alloc.m_IndexRef.m_WorldObjects[slot];
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
  this->m_ObjectSpec.Iterate([&iFn, this](typename ObjectTable<T>::Handle iHandle, T const& iData)
    {
      uint32_t slot = iHandle.GetId();
      ObjectHandle object = m_Alloc.m_IndexRef.m_WorldObjects[slot];
      if (this->m_World.IsObjectValid(object))
      {
        iFn(object, iData);
      }
    });
}

template <typename T>
T_DensePropertySheetAllocator<T>::T_DensePropertySheetAllocator(World& iWorld, ObjectDataIndex& iIndex, Type const* iType, DenseGameDataView<T>* iViewPtr)
  : DenseGameDataAllocator(iIndex, iType)
{
  m_ViewPtr = iViewPtr;
}

template <typename T>
ObjectTableHandle_Base T_DensePropertySheetAllocator<T>::Alloc()
{
  ObjectTableHandle_Base handle = GetDataFromSlot_Inl(m_ObjectsSpec.GetImplementation().m_Ids.Peek());
  if (!m_ObjectsSpec.GetImplementation().IsValid(handle))
  {
    m_ObjectsSpec.Alloc();
  }
  else
  {
    new(m_ObjectsSpec.GetImplementation().Get(handle)) T;
    m_ObjectsSpec.GetImplementation().m_Ids.Get();
  }
  return handle;
}

template <typename T>
void T_DensePropertySheetAllocator<T>::Release(ObjectTableHandle_Base iHandle)
{
  m_ObjectsSpec.Get(typename ObjectTable<T>::Handle(iHandle)).~T();
  m_ObjectsSpec.GetImplementation().m_Ids.Return(iHandle.GetId());
}

template <typename T>
void T_DensePropertySheetAllocator<T>::Clear()
{
  DenseGameDataAllocator::Clear();
  m_ObjectsSpec.Reset();
}

