/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/path.hpp>
#include <core/resource/resource.hpp>
#include <core/type/dynobject.hpp>
#include <core/stream/serializer.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  MAKE_NAME(PropertySheetName);

  class Archetype;
  class GameDatabase;

  struct PropertySheetAllocatorBase;

  class EXL_ENGINE_API PropertiesManifest : public RttiObject
  {
    DECLARE_RTTI(PropertiesManifest, RttiObject);
  public:
    template <typename T>
    void RegisterPropertySheet(PropertySheetName iName, bool iIsArchetype = true);
    void RegisterPropertySheet(PropertySheetName, Type const*, bool iIsArchetype = true);
    void RegisterPropertySheet(PropertySheetName, Type const*, std::function<PropertySheetAllocatorBase*(World&)> iFactory, bool iIsArchetype = true);

    Type const* GetTypeFromName(PropertySheetName iName) const;
    
    Vector<PropertySheetName> GetProperties() const;
    Vector<PropertySheetName> GetArchetypeProperties() const;
  protected:
    struct PropertyEntry
    {
      Type const* type;
      std::function<PropertySheetAllocatorBase*(World&)> factory;
      bool isArchetype;
    };
    friend GameDatabase;
    UnorderedMap<PropertySheetName, PropertyEntry> m_Properties;
  };

  struct EXL_ENGINE_API PropertySheetAllocatorBase
  {
    PropertySheetAllocatorBase(Type const* iType, ObjectTable_Data& iObjects)
      : m_Type(iType)
      , m_ObjectData(iObjects)
    {}
    virtual ~PropertySheetAllocatorBase() = default;
    virtual ObjectTableHandle_Base Alloc() = 0;
    virtual void Release(ObjectTableHandle_Base) = 0;
    Type const* m_Type;
    ObjectTable_Data& m_ObjectData;
    void EnsureStorage(ObjectHandle iHandle);
    Vector<ObjectHandle> m_WorldObjects;
    Vector<ObjectTableHandle_Base> m_ObjectHandles;
    Vector<ObjectTableHandle_Base> m_ArchetypeHandle;
    bool m_HasView = false;
  };

  template <typename T>
  class GameDataView
  {
  public:
    GameDataView(World& iWorld, PropertySheetAllocatorBase& iAlloc, ObjectTable<T>& iObjectsSpec);

    T const* Get(ObjectHandle iObject) const;
    T* Get(ObjectHandle iObject);
    T& GetOrCreate(ObjectHandle iObject);

    template <typename Functor>
    void Iterate(Functor const& iFn);

    template <typename Functor>
    void Iterate(Functor const& iFn) const;

  protected:
    World& m_World;
    PropertySheetAllocatorBase& m_Alloc;
    ObjectTable<T>& m_ObjectSpec;
  };

  template <typename T>
  struct PropertySheetAllocator : PropertySheetAllocatorBase
  {
    PropertySheetAllocator(World& iWorld);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    ObjectTable<T> m_ObjectsSpec;
    GameDataView<T> m_View;
  };

  struct GnrPropertySheetAllocator : PropertySheetAllocatorBase
  {
    GnrPropertySheetAllocator(World& iWorld, Type const* iType);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    static void NullDeleter(void*)
    {}
    ObjectTable_Data m_ObjectsSpec;
    Type const* m_Type;
  };

  struct CustomizationData;

  class EXL_ENGINE_API GameDatabase : public WorldSystem
  {
    DECLARE_RTTI(GameDatabase, WorldSystem);

  public:

    GameDatabase(PropertiesManifest const& iManifest);

    void InstantiateArchetype(ObjectHandle iObject, Archetype const* iArchetype, CustomizationData const* iCusto);
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
      PropertySheetAllocatorBase* alloc = m_Allocators[iName];
      if (!alloc->m_HasView)
      {
        return nullptr;
      }
      return &static_cast<PropertySheetAllocator<T>*>(m_Allocators[iName])->m_View;
    }

    void Register(World& iWorld) override;
  protected:
    UnorderedMap<PropertySheetName, PropertySheetAllocatorBase*> m_Allocators;
    UnorderedMap<Archetype const*, ArchetypeData> m_ArchetypeData;

    PropertiesManifest const& m_Manifest;
  };

  struct EXL_ENGINE_API CustomizationData
  {
    using FieldsMap = UnorderedMap<TypeFieldName, DynObject>;

    UnorderedMap<PropertySheetName, FieldsMap> m_PropertyCustomization;
    UnorderedMap<ComponentName, FieldsMap> m_ComponentCustomization;

    static void ApplyCustomization(DynObject& ioData, FieldsMap const& iFields);

    void ApplyCustomization(ComponentName iName, DynObject& ioData) const;
    void ApplyCustomization(PropertySheetName iName, DynObject& ioData) const;
    SERIALIZE_METHODS;
  };

  class EXL_ENGINE_API Archetype : public Resource
  {
    DECLARE_RTTI(Archetype, Resource)
  public:

    struct PropertyEntry
    {
      DynObject m_Data;
      bool m_Instanced = false;
    };

    static void Init();

#ifdef EXL_RSC_HAS_FILESYSTEM
    static Archetype* Create(Path const& iDir, String const& iName);
#endif

    static ResourceLoaderName StaticLoaderName();

    void Instantiate(ObjectHandle iHandle, World& iWorld, CustomizationData const* iCusto) const;

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;

    void SetProperty(PropertySheetName iName, ConstDynObject const& iObject, bool iInstanced);
    void RemoveProperty(PropertySheetName iName);
    ConstDynObject const& GetProperty(PropertySheetName iName) const;
    UnorderedMap<PropertySheetName, PropertyEntry> const& GetProperties() const
    {
      return m_Properties;
    }

    void SetComponent(ComponentName iName, ConstDynObject const& iObject);
    void RemoveComponent(ComponentName iName);
    ConstDynObject const& GetComponent(ComponentName iName) const;
    UnorderedMap<ComponentName, DynObject> const& GetComponents() const
    {
      return m_Components;
    }

  protected:
    friend TResourceLoader <Archetype, ResourceLoader>;
    Archetype(ResourceMetaData&);

    UnorderedMap<PropertySheetName, PropertyEntry> m_Properties;
    UnorderedMap<ComponentName, DynObject> m_Components;
  };

#include "archetype.inl"
}