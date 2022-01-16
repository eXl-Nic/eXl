/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/gamedatabase.hpp>
#include <engine/game/archetype.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(PropertiesManifest);
  IMPLEMENT_RTTI(GameDatabase);

  GnrPropertySheetAllocator::GnrPropertySheetAllocator(World& iWorld, Type const* iType)
    : SparseDataAllocator(iType, m_ObjectsSpec)
    , m_ObjectsSpec(iType->GetSize(), 8)
    , m_Type(iType)
  {
  }

  void GnrPropertySheetAllocator::Clear()
  {
    SparseDataAllocator::Clear();
    m_ObjectsSpec.Reset(&NullDeleter);
  }

  ObjectTableHandle_Base GnrPropertySheetAllocator::Alloc()
  {
    ObjectTableHandle_Base handle;
    void* data = m_ObjectsSpec.Alloc(handle);
    m_Type->Construct(data);

    return handle;
  }

  void GnrPropertySheetAllocator::Release(ObjectTableHandle_Base iHandle)
  {
    void* data = m_ObjectsSpec.Get(iHandle);
    if (data)
    {
      m_Type->Destruct(data);
      m_ObjectsSpec.Release(iHandle, &GnrPropertySheetAllocator::NullDeleter);
    }
  }

  Vector<PropertySheetName> PropertiesManifest::GetProperties() const
  {
    Vector<PropertySheetName> names;
    for (auto const& entry : m_Properties)
    {
      names.push_back(entry.first);
    }

    return names;
  }

  Vector<PropertySheetName> PropertiesManifest::GetArchetypeProperties() const
  {
    Vector<PropertySheetName> names;
    for (auto const& entry : m_Properties)
    {
      if (entry.second.isSparse)
      {
        names.push_back(entry.first);
      }
    }

    return names;
  }

  Type const* PropertiesManifest::GetTypeFromName(PropertySheetName iName) const
  {
    auto iter = m_Properties.find(iName);
    if (iter == m_Properties.end())
    {
      return nullptr;
    }

    return iter->second.type;
  }

  void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, Type const* iType)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Properties.count(iName) == 0);

    PropertyEntry newEntry;
    newEntry.type = iType;
    newEntry.isSparse = true;
    newEntry.factory = [iType](World& iWorld)
    {
      return new GnrPropertySheetAllocator(iWorld, iType);
    };
    m_Properties.insert(std::make_pair(iName, newEntry));
  }

  void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, Type const* iType, std::function<SparseDataAllocator* (World&)> iFactory)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Properties.count(iName) == 0);

    PropertyEntry newEntry;
    newEntry.type = iType;
    newEntry.factory = [iFactory](World& iWorld) { return iFactory(iWorld); };
    newEntry.isSparse = true;
    m_Properties.insert(std::make_pair(iName, newEntry));
  }

  void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, Type const* iType, std::function<DenseDataAllocator* (World&)> iFactory)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Properties.count(iName) == 0);

    PropertyEntry newEntry;
    newEntry.type = iType;
    newEntry.factory = [iFactory](World& iWorld) { return iFactory(iWorld); };
    newEntry.isSparse = false;
    m_Properties.insert(std::make_pair(iName, newEntry));
  }

  GameDatabase::ArchetypeData::ArchetypeData() = default;
  GameDatabase::ArchetypeData::~ArchetypeData() = default;
  GameDatabase::ArchetypeData::ArchetypeData(ArchetypeData const& iOther)
    : m_ArchetypeRsc(iOther.m_ArchetypeRsc)
    , m_Data(iOther.m_Data)
  {
  }

  GameDatabase::GameDatabase(PropertiesManifest const& iManifest)
    : m_Manifest(iManifest)
  {}

  void GameDatabase::Register(World& iWorld)
  {
    WorldSystem::Register(iWorld);
    for (auto entry : m_Manifest.m_Properties)
    {
      m_AllocatorSlot.insert(std::make_pair(entry.first, m_Allocators.size()));
      AllocatorInfo newInfo;
      if (entry.second.isSparse)
      {
        newInfo.m_SparseAllocator = static_cast<SparseDataAllocator*>(entry.second.factory(iWorld));
      }
      else
      {
        newInfo.m_DenseAllocator = static_cast<DenseDataAllocator*>(entry.second.factory(iWorld));
      }
      m_Allocators.push_back(newInfo);
    }
  }

  void GameDatabase::InstantiateArchetype(ObjectHandle iObject, Archetype const* iArchetype, CustomizationData const* iCusto)
  {
    if (iArchetype == nullptr)
    {
      return;
    }

    ArchetypeData const& data = GetOrCreateArchetypeData(*iArchetype);
    for (auto const& entry : data.m_Data)
    {
      PropertySheetName propertySheet = entry.first;
      auto iter = m_AllocatorSlot.find(propertySheet);
      eXl_ASSERT_REPAIR_BEGIN(iter != m_AllocatorSlot.end()) { continue; }

      if(SparseDataAllocator* sparseAlloc = m_Allocators[iter->second].m_SparseAllocator)
      {
        uint32_t slot = sparseAlloc->GetSlot(iObject);

        ObjectTableHandle_Base existingHandle;

        if (slot == -1)
        {
          slot = sparseAlloc->AllocateSlot_Inl(iObject);
        }
        else if(sparseAlloc)
        {
          existingHandle = sparseAlloc->m_ObjectHandles[slot];
        }

        if (sparseAlloc->m_ArchetypeHandle[slot] != existingHandle)
        {
          sparseAlloc->Release(existingHandle);
        }

        sparseAlloc->m_ArchetypeHandle[slot] = entry.second.handle;

        if (iCusto && iCusto->m_PropertyCustomization.count(propertySheet) > 0)
        {
          if (!existingHandle.IsAssigned())
          {
            existingHandle = sparseAlloc->Alloc();
            sparseAlloc->m_ObjectHandles[slot] = existingHandle;
          }

          void* sheetData = sparseAlloc->m_ObjectData.Get(existingHandle);
          void const* archetypeData = sparseAlloc->m_ObjectData.Get(entry.second.handle);
          sparseAlloc->m_Type->Copy(archetypeData, sheetData);

          DynObject data(sparseAlloc->m_Type, sheetData);
          iCusto->ApplyCustomization(propertySheet, data);
        }
      }
      else
      {
        DenseDataAllocator* denseAlloc = m_Allocators[iter->second].m_DenseAllocator;
        ObjectTableHandle_Base handle = denseAlloc->GetDataFromSlot_Inl(denseAlloc->AllocateSlot_Inl(iObject));
        void* sheetData = denseAlloc->m_ObjectData.Get(handle);

        void const* archetypeData = denseAlloc->m_ObjectData.Get(entry.second.handle);
        denseAlloc->m_Type->Copy(archetypeData, sheetData);
        if (iCusto && iCusto->m_PropertyCustomization.count(propertySheet) > 0)
        {
          DynObject data(denseAlloc->m_Type, sheetData);
          iCusto->ApplyCustomization(propertySheet, data);
        }
      }
    }
  }

  DynObject GameDatabase::ModifyData(ObjectHandle iObject, PropertySheetName iName)
  {
    auto iter = m_AllocatorSlot.find(iName);
    if (iter == m_AllocatorSlot.end()
      || !GetWorld().IsObjectValid(iObject))
    {
      return DynObject();
    }
    DataAllocatorBase* alloc = m_Allocators[iter->second].GetAlloc();
    uint32_t slot = alloc->GetSlot(iObject);
    ObjectTableHandle_Base dataHandle;
    if (slot == -1 || !(dataHandle = alloc->GetDataFromSlot(slot)).IsAssigned())
    {
      return DynObject();
    }
    
    void* sheetData = alloc->m_ObjectData.Get(dataHandle);
    return DynObject(alloc->m_Type, sheetData);
  }

  ConstDynObject GameDatabase::GetData(ObjectHandle iObject, PropertySheetName iName)
  {
    auto iter = m_AllocatorSlot.find(iName);
    if (iter == m_AllocatorSlot.end()
      || !GetWorld().IsObjectValid(iObject))
    {
      return ConstDynObject();
    }

    DataAllocatorBase const* alloc = m_Allocators[iter->second].GetAlloc();
    uint32_t slot = alloc->GetSlot(iObject);
    ObjectTableHandle_Base dataHandle;
    if (slot == -1 || !(dataHandle = alloc->GetDataFromSlot(slot)).IsAssigned())
    {
      return ConstDynObject();
    }

    void* sheetData = alloc->m_ObjectData.Get(dataHandle);
    return ConstDynObject(alloc->m_Type, sheetData);
  }

  GameDatabase::ArchetypeData const& GameDatabase::GetOrCreateArchetypeData(Archetype const& iArchetype)
  {
    auto iter = m_ArchetypeData.find(&iArchetype);
    if (iter == m_ArchetypeData.end())
    {
      ArchetypeData newData;
      newData.m_ArchetypeRsc.Set(&iArchetype);
      for (auto const& prop : iArchetype.GetProperties())
      {
        PropertySheetName propertySheet = prop.first;

        auto iter = m_AllocatorSlot.find(propertySheet);
        eXl_ASSERT_REPAIR_BEGIN(iter != m_AllocatorSlot.end()) { continue; }

        if (SparseDataAllocator* sparseAlloc = m_Allocators[iter->second].m_SparseAllocator)
        {
          ObjectTableHandle_Base newHandle;
          sparseAlloc->m_ObjectData.Alloc(newHandle);

          void* sheetData = sparseAlloc->m_ObjectData.Get(newHandle);
          void const* archetypeData = prop.second.m_Data.GetBuffer();
          sparseAlloc->m_Type->Copy_Uninit(archetypeData, sheetData);

          ArchetypeData::PropEntry newEntry;
          newEntry.handle = newHandle;
          newEntry.instanced = prop.second.m_Instanced;

          newData.m_Data.insert(std::make_pair(propertySheet, newEntry));
        }
      }
      iter = m_ArchetypeData.insert(std::make_pair(&iArchetype, std::move(newData))).first;
    }

    return iter->second;
  }

  void GameDatabase::ForgetArchetype(Archetype const& iArchetype)
  {
    auto iter = m_ArchetypeData.find(&iArchetype);
    if (iter != m_ArchetypeData.end())
    {
      for (auto const& prop : iter->second.m_Data)
      {
        PropertySheetName propertySheet = prop.first;

        auto iter = m_AllocatorSlot.find(propertySheet);
        eXl_ASSERT_REPAIR_BEGIN(iter != m_AllocatorSlot.end()) { continue; }

        if (SparseDataAllocator* sparseAlloc = m_Allocators[iter->second].m_SparseAllocator)
        {
          for (uint32_t i = 0; i < sparseAlloc->m_ArchetypeHandle.size(); ++i)
          {
            if (sparseAlloc->m_ArchetypeHandle[i] == prop.second.handle
              && sparseAlloc->m_ObjectHandles[i] == prop.second.handle)
            {
              // Detach from archetype data by requesting a mutable copy;
              sparseAlloc->m_ObjectHandles[i] = ObjectTableHandle_Base();
              sparseAlloc->GetDataFromSlot(i);
              sparseAlloc->m_ArchetypeHandle[i] = ObjectTableHandle_Base();
            }
          }
          sparseAlloc->Release(prop.second.handle);
        }
      }
      m_ArchetypeData.erase(iter);
    }
  }

  void GameDatabase::GarbageCollect()
  {
    if (m_Allocators.empty())
    {
      return;
    }
    DataAllocatorBase* alloc = m_Allocators[m_GarbageCollectionCycle].GetAlloc();
    alloc->GarbageCollect(GetWorld());
    m_GarbageCollectionCycle = (m_GarbageCollectionCycle + 1) % m_Allocators.size();
  }
}