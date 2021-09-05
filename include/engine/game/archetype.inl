
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
T* GameDataView<T>::GetDataForDeletion(ObjectHandle iObject)
{
  if (!m_World.IsObjectBeingDestroyed(iObject))
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
    else
    {
      T& prop = m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[iObject.GetId()]));
      
      // Ensure data is in default state when recycling storage
      prop.~T();
      new(&prop) T;

      return prop;
    }
  }

  return m_ObjectSpec.Get(typename ObjectTable<T>::Handle(m_Alloc.m_ObjectHandles[iObject.GetId()]));
}

template <typename T>
void GameDataView<T>::Erase(ObjectHandle iObject)
{
  if (m_Alloc.m_WorldObjects.size() > iObject.GetId()
    && m_Alloc.m_WorldObjects[iObject.GetId()].IsAssigned())
  {
    m_Alloc.m_WorldObjects[iObject.GetId()] = ObjectHandle();
    if (!m_Alloc.m_ArchetypeHandle[iObject.GetId()].IsAssigned()
      || m_Alloc.m_ObjectHandles[iObject.GetId()] != m_Alloc.m_ArchetypeHandle[iObject.GetId()])
    {
      m_Alloc.Release(m_Alloc.m_ObjectHandles[iObject.GetId()]);
      m_Alloc.m_ObjectHandles[iObject.GetId()] = ObjectTableHandle_Base();
    }
  }
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
PropertySheetAllocator<T>::PropertySheetAllocator(World& iWorld, Type const* iType)
  : PropertySheetAllocatorBase(iType, m_ObjectsSpec.GetImplementation())
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
  RegisterPropertySheet(iName, TypeManager::GetType<T>(), [](World& iWorld) { return new PropertySheetAllocator<T>(iWorld, TypeManager::GetType<T>()); }, iIsArchetype);
}