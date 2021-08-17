
template <typename T>
GameDataView<T>::GameDataView(World& iWorld, PropertySheetAllocatorBase& iAlloc, ObjectTable<T>& iObjectsSpec)
  : m_World(iWorld)
  , m_Alloc(iAlloc)
  , m_ObjectSpec(iObjectsSpec)
{}

template <typename T>
T const* GameDataView<T>::Get(ObjectHandle iObject) const
{
  return const_cast<GameDataView<T>*>(this)->Get(iObject);
}

template <typename T>
T* GameDataView<T>::Get(ObjectHandle iObject)
{
  if (!m_World.IsObjectValid(iObject))
  {
    return nullptr;
  }
  m_Alloc.EnsureStorage(iObject);
  if (m_Alloc.m_WorldObjects[iObject.GetId()] != iObject)
  {
    return nullptr;
  }

  return &m_ObjectSpec.Get(ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[iObject.GetId()]));
}

template <typename T>
T& GameDataView<T>::GetOrCreate(ObjectHandle iObject)
{
  m_Alloc.EnsureStorage(iObject);
  if (m_Alloc.m_WorldObjects[iObject.GetId()] != iObject)
  {
    m_Alloc.m_WorldObjects[iObject.GetId()] = iObject;
    if (!m_Alloc.m_ObjectHandles[iObject.GetId()].IsAssigned())
    {
      m_Alloc.m_ObjectHandles[iObject.GetId()] = m_ObjectSpec.Alloc();
    }
  }

  return m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[iObject.GetId()]));
}

template <typename T>
template <typename Functor>
void GameDataView<T>::Iterate(Functor const& iFn)
{
  for (auto const& object : m_Alloc.m_WorldObjects)
  {
    if (m_World.IsObjectValid(object)
      && m_Alloc.m_ObjectHandles[object.GetId()].IsAssigned())
    {
      iFn(object, m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[object.GetId()])));
    }
  }
}

template <typename T>
template <typename Functor>
void GameDataView<T>::Iterate(Functor const& iFn) const
{
  for (auto const& object : m_Alloc.m_WorldObjects)
  {
    if (m_World.IsObjectValid(object)
      && m_Alloc.m_ObjectHandles[object.GetId()].IsAssigned())
    {
      iFn(object, const_cast<T const&>(m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[object.GetId()]))));
    }
  }
}

template <typename T>
PropertySheetAllocator<T>::PropertySheetAllocator(World& iWorld)
  : PropertySheetAllocatorBase(TypeManager::GetType<T>(), m_ObjectsSpec.GetImplementation())
  , m_View(iWorld, *this, m_ObjectsSpec)
{
  m_HasView = true;
}

template <typename T>
ObjectTableHandle_Base PropertySheetAllocator<T>::Alloc()
{
  return m_ObjectsSpec.Alloc();
}

template <typename T>
void PropertySheetAllocator<T>::Release(ObjectTableHandle_Base iHandle)
{
  return m_ObjectsSpec.Release(typename ObjectTable<T>::Handle(iHandle));
}

template <typename T>
void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, bool iIsArchetype)
{
  RegisterPropertySheet(iName, TypeManager::GetType<T>(), [](World& iWorld) { return new PropertySheetAllocator<T>(iWorld); }, iIsArchetype);
}