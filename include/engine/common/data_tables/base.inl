inline GameDataAllocatorBase::ConstIterator::ConstIterator(World& iWorld, GameDataAllocatorBase const& iAlloc, ObjectTable_Data* iData, ObjectToSlotMap::const_iterator iIter)
  : m_World(&iWorld)
  , m_Alloc(&iAlloc)
  , m_Data(iData)
  , m_Iter(iIter)
{
  while (IsValid() && !m_World->IsObjectValid(m_Iter->first))
  {
    ++m_Iter;
  }
}

inline bool GameDataAllocatorBase::ConstIterator::IsValid() const
{
  return m_Alloc != nullptr && m_Iter != m_Alloc->m_IndexRef.m_ObjectToSlot.end();
}

inline GameDataAllocatorBase::ConstIterator::ConstIterator()
  : m_Alloc(nullptr)
{}

inline GameDataAllocatorBase::ConstIterator::value_type GameDataAllocatorBase::ConstIterator::operator*() const
{
  eXl_ASSERT_REPAIR_RET(IsValid(), value_type());
  ObjectTableHandle_Base dataHandle = m_Alloc->GetDataFromSlot(m_Iter->second);
  void const* data = m_Data->Get(dataHandle);
  ConstDynObject obj;
  obj.SetTypeConst(m_Alloc->m_Type, data);
  return value_type(m_Iter->first, std::move(obj));
}

inline GameDataAllocatorBase::ConstIterator& GameDataAllocatorBase::ConstIterator::operator++()
{
  eXl_ASSERT_REPAIR_RET(IsValid(), *this);
  do
  {
    m_Iter++;
  } while (IsValid() && !m_World->IsObjectValid(m_Iter->first));
  return *this;
}

inline bool GameDataAllocatorBase::ConstIterator::operator ==(GameDataAllocatorBase::ConstIterator const& iOther) const
{
  return m_Iter == iOther.m_Iter;
}

inline bool GameDataAllocatorBase::ConstIterator::operator !=(ConstIterator const& iOther) const
{
  return m_Iter != iOther.m_Iter;
}

inline GameDataAllocatorBase::Iterator::Iterator(World& iWorld, GameDataAllocatorBase& iAlloc, ObjectTable_Data* iData, ObjectToSlotMap::const_iterator iIter)
  : m_World(&iWorld)
  , m_Alloc(&iAlloc)
  , m_Data(iData)
  , m_Iter(iIter)
{
  while (IsValid() && !m_World->IsObjectValid(m_Iter->first))
  {
    ++m_Iter;
  }
}

inline bool GameDataAllocatorBase::Iterator::IsValid() const
{
  return m_Alloc != nullptr && m_Iter != m_Alloc->m_IndexRef.m_ObjectToSlot.end();
}

inline GameDataAllocatorBase::Iterator::Iterator()
  : m_Alloc(nullptr)
{}


inline GameDataAllocatorBase::Iterator::value_type GameDataAllocatorBase::Iterator::operator*() const
{
  eXl_ASSERT_REPAIR_RET(IsValid(), value_type());
  ObjectTableHandle_Base dataHandle = m_Alloc->GetDataFromSlot(m_Iter->second);
  void* data = m_Data->Get(dataHandle);
  DynObject obj;
  obj.SetType(m_Alloc->m_Type, data, false);
  return value_type(m_Iter->first, std::move(obj));
}

inline GameDataAllocatorBase::Iterator& GameDataAllocatorBase::Iterator::operator++()
{
  eXl_ASSERT_REPAIR_RET(IsValid(), *this);
  do
  {
    m_Iter++;
  } while (IsValid() && !m_World->IsObjectValid(m_Iter->first));
  return *this;
}

inline bool GameDataAllocatorBase::Iterator::operator ==(Iterator const& iOther) const
{
  return m_Iter == iOther.m_Iter;
}

inline bool GameDataAllocatorBase::Iterator::operator !=(Iterator const& iOther) const
{
  return m_Iter != iOther.m_Iter;
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
  //else if (auto* stridedView = GetStridedView())
  //{
  //  stridedView->StridedGameDataView<T>::Iterate(iFn);
  //}
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
  //else if (auto* stridedView = GetStridedView())
  //{
  //  stridedView->StridedGameDataView<T>::Iterate(iFn);
  //}
}

inline uint32_t GameDataAllocatorBase::GetSlot(ObjectHandle iHandle) const
{
  auto iter = m_IndexRef.m_ObjectToSlot.find(iHandle);
  return m_IndexRef.m_ObjectToSlot.end() != iter ? iter->second : -1;
}
