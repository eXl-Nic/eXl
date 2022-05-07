/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/gamedata.hpp>
#include <core/resource/resource.hpp>
#include <core/type/dynobject.hpp>

namespace eXl
{
  class GameDatabase;

  class EXL_ENGINE_API PropertiesManifest : public RttiObject
  {
    DECLARE_RTTI(PropertiesManifest, RttiObject);
  public:
    template <typename T>
    void RegisterPropertySheet(PropertySheetName iName, bool iIsSparse = true)
    {
      if (iIsSparse)
      {
        RegisterPropertySheet(iName, TypeManager::GetType<T>(), [] (World & iWorld)
        {
          return new SparsePropertySheetAllocator<T>(iWorld, TypeManager::GetType<T>());
        });
      }
      else
      { 
        RegisterPropertySheet(iName, TypeManager::GetType<T>(), [] (World & iWorld)
        {
          return new DensePropertySheetAllocator<T>(iWorld, TypeManager::GetType<T>());
        });
      }
    }

    void RegisterPropertySheet(PropertySheetName, Type const*);
    void RegisterPropertySheet(PropertySheetName, Type const*, std::function<SparseGameDataAllocator* (World&)> iFactory);
    void RegisterPropertySheet(PropertySheetName, Type const*, std::function<DenseGameDataAllocator* (World&)> iFactory);

    Type const* GetTypeFromName(PropertySheetName iName) const;

    Vector<PropertySheetName> GetProperties() const;
    Vector<PropertySheetName> GetArchetypeProperties() const;
  protected:
    struct PropertyEntry
    {
      Type const* type;
      std::function<GameDataAllocatorBase* (World&)> factory;
      bool isSparse;
    };
    friend GameDatabase;
    UnorderedMap<PropertySheetName, PropertyEntry> m_Properties;
  };

  struct GnrPropertySheetAllocator : SparseGameDataAllocator
  {
    GnrPropertySheetAllocator(World& iWorld, Type const* iType);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    static void NullDeleter(void*) {}
    void Clear() override;
    ObjectDataIndex m_Index;
    ObjectTable_Data m_ObjectsSpec;
    Type const* m_Type;
  };

  struct CustomizationData;
  class Archetype;

  class EXL_ENGINE_API GameDatabase : public WorldSystem
  {
    DECLARE_RTTI(GameDatabase, WorldSystem);

  public:

    GameDatabase(PropertiesManifest const& iManifest);
    void GarbageCollect();

    void InstantiateArchetype(ObjectHandle iObject, Archetype const* iArchetype, CustomizationData const* iCusto);
    void ForgetArchetype(Archetype const&);
    DynObject ModifyData(ObjectHandle iObject, PropertySheetName iName);
    ConstDynObject GetData(ObjectHandle iObject, PropertySheetName iName);

    template <typename T>
    T const* GetData(ObjectHandle iObject, PropertySheetName iName)
    {
      //eXl_ASSERT(TypeManager::GetType<T>() == GetTypeFromName(iName));
      return static_cast<T const*>(GetData(iObject, iName).CastBuffer<T>());
    }

    template <typename T>
    T* ModifyData(ObjectHandle iObject, PropertySheetName iName)
    {
      //eXl_ASSERT(TypeManager::GetType<T>() == GetTypeFromName(iName));
      return static_cast<T const*>(ModifyData(iObject, iName).CastBuffer<T>());
    }

  protected:

    struct ArchetypeData
    {
      ArchetypeData();
      ~ArchetypeData();
      ArchetypeData(ArchetypeData const&);

      ResourceHandle<Archetype> m_ArchetypeRsc;
      struct PropEntry
      {
        bool instanced;
        ObjectTableHandle_Base handle;
      };
      UnorderedMap<PropertySheetName, PropEntry> m_Data;
    };

    ArchetypeData const& GetOrCreateArchetypeData(Archetype const&);

  public:

    template <typename T>
    GameDataView<T>* GetView(PropertySheetName iName)
    {
      if (TypeManager::GetType<T>() != m_Manifest.GetTypeFromName(iName))
      {
        return nullptr;
      }
      GameDataAllocatorBase* alloc = m_Allocators[m_AllocatorSlot[iName]].GetAlloc();
      return reinterpret_cast<GameDataView<T>*>(alloc->m_ViewPtr);
    }

    void Register(World& iWorld) override;
  protected:

    PropertiesManifest const& m_Manifest;

    struct AllocatorInfo
    {
      SparseGameDataAllocator* m_SparseAllocator = nullptr;
      DenseGameDataAllocator* m_DenseAllocator = nullptr;
      GameDataAllocatorBase* GetAlloc()
      {
        return m_SparseAllocator 
          ? static_cast<GameDataAllocatorBase*>(m_SparseAllocator)
          : m_DenseAllocator;
      }
    };

    Vector<AllocatorInfo> m_Allocators;
    UnorderedMap<PropertySheetName, uint32_t> m_AllocatorSlot;
    UnorderedMap<Archetype const*, ArchetypeData> m_ArchetypeData;
    uint32_t m_GarbageCollectionCycle = 0;
  };
}