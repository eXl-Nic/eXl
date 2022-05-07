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
