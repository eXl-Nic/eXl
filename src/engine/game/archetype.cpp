/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/archetype.hpp>
#include <engine/game/commondef.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <boost/optional.hpp>

#include <core/type/tupletype.hpp>
#include <core/type/arraytype.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(PropertiesManifest);
  IMPLEMENT_RTTI(GameDatabase);
  IMPLEMENT_RTTI(Archetype);

  IMPLEMENT_SERIALIZE_METHODS(CustomizationData)

  using ArchetypeLoader = TResourceLoader <Archetype, ResourceLoader>;

  void Archetype::Init()
  {
    ResourceManager::AddLoader(&ArchetypeLoader::Get(), Archetype::StaticRtti());
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  Archetype* Archetype::Create(Path const& iDir, String const& iName)
  {
    return ArchetypeLoader::Get().Create(iDir, iName);
  }
#endif

  ResourceLoaderName Archetype::StaticLoaderName()
  {
    return ResourceLoaderName("Archetype");
  }

  Err Archetype::Stream_Data(Streamer& iStreamer) const
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Properties");
    iStreamer.BeginSequence();
    for (auto property : m_Properties)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.WriteString(property.first.get());
      iStreamer.PopKey();
      iStreamer.PushKey("Instanced");
      iStreamer.Write(&property.second.m_Instanced);
      iStreamer.PopKey();
      iStreamer.PushKey("Data");
      
      Type const* propType = property.second.m_Data.GetType();

      auto& manifest = *iStreamer.GetManifest<PropertiesManifest>();
      eXl_ASSERT(manifest.GetTypeFromName(property.first) == propType);

      propType->Stream(property.second.m_Data.GetBuffer(), &iStreamer);

      iStreamer.PopKey();
      iStreamer.EndStruct();
    }
    iStreamer.EndSequence();
    iStreamer.PopKey();

    iStreamer.PushKey("Components");
    iStreamer.BeginSequence();
    for (auto component : m_Components)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.WriteString(component.first.get());
      iStreamer.PopKey();
      iStreamer.PushKey("Data");

      Type const* compType = component.second.GetType();

      auto& manifest = *iStreamer.GetManifest<ComponentManifest>();
      eXl_ASSERT(manifest.GetComponentTypeFromName(component.first) == compType);

      compType->Stream(component.second.GetBuffer(), &iStreamer);

      iStreamer.PopKey();
      iStreamer.EndStruct();
    }
    iStreamer.EndSequence();
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  Err Archetype::Unstream_Data(Unstreamer& iStreamer)
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Properties");
    if (iStreamer.BeginSequence())
    {
      do
      {
        String tempStr;
        iStreamer.BeginStruct();
        iStreamer.PushKey("Name");
        iStreamer.ReadString(&tempStr);
        iStreamer.PopKey();
        iStreamer.PushKey("Instanced");
        bool instanced = false;
        iStreamer.Read(&instanced);
        iStreamer.PopKey();
        iStreamer.PushKey("Data");

        PropertySheetName name(tempStr);

        auto& manifest = *iStreamer.GetManifest<PropertiesManifest>();
        Type const* propertyType = manifest.GetTypeFromName(name);
        eXl_ASSERT_REPAIR_BEGIN(propertyType != nullptr){}
        else
        {
          void* buffer = nullptr;
          propertyType->Unstream(buffer, &iStreamer);

          PropertyEntry newEntry;
          newEntry.m_Data.SetType(propertyType, buffer, true);
          newEntry.m_Instanced = instanced;

          m_Properties.emplace(std::make_pair(name, std::move(newEntry)));
        }

        iStreamer.PopKey();
        iStreamer.EndStruct();

      } while (iStreamer.NextSequenceElement());
    }
    iStreamer.PopKey();

    iStreamer.PushKey("Components");
    if (iStreamer.BeginSequence())
    {
      do
      {
        String tempStr;
        iStreamer.BeginStruct();
        iStreamer.PushKey("Name");
        iStreamer.ReadString(&tempStr);
        iStreamer.PopKey();
        iStreamer.PushKey("Data");

        ComponentName name(tempStr);
        auto& manifest = *iStreamer.GetManifest<ComponentManifest>();
        Type const* componentType = manifest.GetComponentTypeFromName(name);
        eXl_ASSERT_REPAIR_BEGIN(componentType != nullptr){}
        else
        {
          void* buffer = nullptr;
          componentType->Unstream(buffer, &iStreamer);
          DynObject componentData;
          componentData.SetType(componentType, buffer, true);
          m_Components.emplace(std::make_pair(name, std::move(componentData)));
        }
        iStreamer.PopKey();
        iStreamer.EndStruct();


      } while (iStreamer.NextSequenceElement());
    }
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  uint32_t Archetype::ComputeHash()
  {
    return 0;
  }

  Archetype::Archetype(ResourceMetaData& iMeta)
    : Resource(iMeta)
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
      m_Allocators.insert(std::make_pair(entry.first, entry.second.factory(iWorld)));
    }
  }

  GnrPropertySheetAllocator::GnrPropertySheetAllocator(World& iWorld, Type const* iType)
    : PropertySheetAllocatorBase(iType, m_ObjectsSpec)
    , m_ObjectsSpec(iType->GetSize(), 8)
    , m_Type(iType)
  {
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
      if (entry.second.isArchetype)
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

  void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, Type const* iType, bool iIsArchetype)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Properties.count(iName) == 0);

    PropertyEntry newEntry;
    newEntry.type = iType;
    newEntry.factory = [iType](World& iWorld)
    {
      return new GnrPropertySheetAllocator(iWorld, iType);
    };
    m_Properties.insert(std::make_pair(iName, newEntry));
  }

  void PropertiesManifest::RegisterPropertySheet(PropertySheetName iName, Type const* iType, std::function<PropertySheetAllocatorBase*(World&)> iFactory, bool iIsArchetype)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Properties.count(iName) == 0);
    
    PropertyEntry newEntry;
    newEntry.type = iType;
    newEntry.factory = std::move(iFactory);
    m_Properties.insert(std::make_pair(iName, newEntry));
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
      auto iter = m_Allocators.find(propertySheet);
      eXl_ASSERT_REPAIR_BEGIN(iter != m_Allocators.end()) { continue; }

      PropertySheetAllocatorBase* alloc = iter->second;
      alloc->EnsureStorage(iObject);

      bool instanced = entry.second.instanced;
      uint32_t const objIdx = iObject.GetId();
      ObjectTableHandle_Base existingHandle = alloc->m_ObjectHandles[objIdx];

      if (!instanced && alloc->m_ArchetypeHandle[objIdx] != existingHandle)
      {
        alloc->Release(existingHandle);
      }
      alloc->m_WorldObjects[objIdx] = iObject;
      alloc->m_ArchetypeHandle[objIdx] = entry.second.handle;
      
      if (instanced)
      {
        if (!existingHandle.IsAssigned())
        {
          existingHandle = alloc->Alloc();
          alloc->m_ObjectHandles[objIdx] = existingHandle;
        }
        void* sheetData = alloc->m_ObjectData.Get(existingHandle);
        void const* archetypeData = alloc->m_ObjectData.Get(entry.second.handle);
        alloc->m_Type->Copy(archetypeData, sheetData);
        if (iCusto)
        {
          DynObject data(alloc->m_Type, sheetData);
          iCusto->ApplyCustomization(propertySheet, data);
        }
      }
      else
      {
        alloc->m_ObjectHandles[objIdx] = entry.second.handle;
      }
    }
  }

  DynObject GameDatabase::ModifyData(ObjectHandle iObject, PropertySheetName iName)
  {
    auto iter = m_Allocators.find(iName);
    if (iter == m_Allocators.end()
      || !GetWorld().IsObjectValid(iObject))
    {
      return DynObject();
    }

    PropertySheetAllocatorBase* alloc = iter->second;
    alloc->EnsureStorage(iObject);
    if (alloc->m_WorldObjects[iObject.GetId()] != iObject
      || alloc->m_ObjectHandles[iObject.GetId()] == alloc->m_ArchetypeHandle[iObject.GetId()])
    {
      return DynObject();
    }

    return DynObject(alloc->m_Type, alloc->m_ObjectData.Get(alloc->m_ObjectHandles[iObject.GetId()]));
  }

  ConstDynObject GameDatabase::GetData(ObjectHandle iObject, PropertySheetName iName)
  {
    auto iter = m_Allocators.find(iName);
    if (iter == m_Allocators.end()
      || !GetWorld().IsObjectValid(iObject))
    {
      return ConstDynObject();
    }

    PropertySheetAllocatorBase* alloc = iter->second;
    alloc->EnsureStorage(iObject);
    if (alloc->m_WorldObjects[iObject.GetId()] != iObject)
    {
      return ConstDynObject();
    }

    return ConstDynObject(alloc->m_Type, alloc->m_ObjectData.Get(alloc->m_ObjectHandles[iObject.GetId()]));
  }

  void PropertySheetAllocatorBase::EnsureStorage(ObjectHandle iHandle)
  {
    while (m_WorldObjects.size() <= iHandle.GetId())
    {
      m_WorldObjects.push_back(ObjectHandle());
      m_ObjectHandles.push_back(ObjectTableHandle_Base());
      m_ArchetypeHandle.push_back(ObjectTableHandle_Base());
    }
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

        auto iter = m_Allocators.find(propertySheet);
        eXl_ASSERT_REPAIR_BEGIN(iter != m_Allocators.end()) { continue; }

        PropertySheetAllocatorBase* alloc = iter->second;
        ObjectTableHandle_Base newHandle;
        alloc->m_ObjectData.Alloc(newHandle);

        void* sheetData = alloc->m_ObjectData.Get(newHandle);
        void const* archetypeData = prop.second.m_Data.GetBuffer();
        alloc->m_Type->Copy_Uninit(archetypeData, sheetData);

        ArchetypeData::PropEntry newEntry;
        newEntry.handle = newHandle;
        newEntry.instanced = prop.second.m_Instanced;

        newData.m_Data.insert(std::make_pair(propertySheet, newEntry));
      }
      iter = m_ArchetypeData.insert(std::make_pair(&iArchetype, std::move(newData))).first;
    }

    return iter->second;
  }

  template <typename Entry>
  void StreamDynObjEntry(Entry const& iEntry, Streamer& iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Name");
    iStreamer.Write(&iEntry.first);
    iStreamer.PopKey();
    iStreamer.PushKey("Data");
    iEntry.second.GetType()->Stream(iEntry.second.GetBuffer(), &iStreamer);
    iStreamer.PopKey();
    iStreamer.EndStruct();
  }

  template <typename Entry>
  void StreamCustoMap(String const& iName, Entry const& iEntry, Streamer& iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey(iName);
    iStreamer.Write(&iEntry.first);
    iStreamer.PopKey();
    iStreamer.PushKey("Fields");
    iStreamer.BeginSequence();
    for (auto const& entry : iEntry.second)
    {
      StreamDynObjEntry(entry, iStreamer);
    }
    iStreamer.PopKey();
    iStreamer.EndSequence();
    iStreamer.EndStruct();
  }

  template <typename MapType, typename GetTypeFunctor>
  void UnstreamDynObjEntry(MapType& oMap, Unstreamer& iStreamer, GetTypeFunctor const& iGetType)
  {
    using value_type = typename MapType::value_type;
    using name_type = typename value_type::first_type;
    String tempStr;
    iStreamer.BeginStruct();
    iStreamer.PushKey("Name");
    iStreamer.Read(&tempStr);
    iStreamer.PopKey();
    name_type name(tempStr);
    Type const* type = iGetType(name);
    eXl_ASSERT_REPAIR_BEGIN(type != nullptr) {}
    else
    {
    iStreamer.PushKey("Data");
    void* buffer = nullptr;
    type->Unstream(buffer, &iStreamer);
    DynObject overrideData;
    overrideData.SetType(type, buffer, true);
    oMap.emplace(std::make_pair(name, std::move(overrideData)));
    iStreamer.PopKey();
    }
  }

  template <typename MapType, typename GetTypeFunctor>
  void UnstreamCustoMap(String const& iName, MapType& oMap, Unstreamer& iStreamer, GetTypeFunctor const& iGetType)
  {
    using value_type = typename MapType::value_type;
    using name_type = typename value_type::first_type;
    using innerMap_type = typename value_type::second_type;
    String tempStr;
    iStreamer.BeginStruct();
    iStreamer.PushKey(iName);
    iStreamer.Read(&tempStr);
    iStreamer.PopKey();
    name_type name(tempStr);
    TupleType const* structType = iGetType(name)->IsTuple();
    eXl_ASSERT_REPAIR_BEGIN(structType != nullptr) {}
    else
    {
    innerMap_type newMap;
    iStreamer.PushKey("Fields");
    if (iStreamer.BeginSequence())
    {
      do
      {
        UnstreamDynObjEntry(newMap, iStreamer, [structType](TypeFieldName iName)
        {
          return structType->GetFieldDetails(iName);
        });
      } while (iStreamer.NextSequenceElement());
    }
    iStreamer.PopKey();
    oMap.emplace(std::make_pair(name, std::move(newMap)));
    }
  }

  Err CustomizationData::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    String const propKeyName("Property");

    iStreamer.PushKey("Properties");
    iStreamer.HandleSequence(m_PropertyCustomization,
      [&](decltype(m_PropertyCustomization)& oMap, Unstreamer& iStreamer)
    {
      PropertiesManifest const& manifest = *iStreamer.GetManifest<PropertiesManifest>();
      UnstreamCustoMap(propKeyName, oMap, iStreamer,
        [&manifest](PropertySheetName iName) { return manifest.GetTypeFromName(iName); });

    },
      [&](decltype(m_PropertyCustomization)::value_type const& iProperty, Streamer& iStreamer)
    {
      StreamCustoMap(propKeyName, iProperty, iStreamer);
    });
    iStreamer.PopKey();

    String const compKeyName("Component");

    iStreamer.PushKey("Components");
    iStreamer.HandleSequence(m_ComponentCustomization,
      [&](decltype(m_ComponentCustomization)& oMap, Unstreamer& iStreamer)
    {
      UnstreamCustoMap(compKeyName, oMap, iStreamer,
        [](ComponentName iName) { return EngineCommon::GetComponents().GetComponentTypeFromName(iName); });

    },
      [&](decltype(m_ComponentCustomization)::value_type const& iProperty, Streamer& iStreamer)
    {
      StreamCustoMap(compKeyName, iProperty, iStreamer);
    });
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  void CustomizationData::ApplyCustomization(DynObject& ioData, FieldsMap const& iFields)
  {
    TupleType const* objType = ioData.GetType()->IsTuple();
    for (auto fieldEntry : iFields)
    {
      TypeFieldName fieldName = fieldEntry.first;
      uint32_t fieldIdx;
      if (objType->GetFieldDetails(fieldName, fieldIdx) != nullptr)
      {
        Type const* fieldType = nullptr;
        void* destField = objType->GetField(ioData.GetBuffer(), fieldIdx, fieldType);

        ConstDynObject const& fieldData = fieldEntry.second;

        eXl_ASSERT_REPAIR_RET(fieldType != nullptr, );
        eXl_ASSERT_REPAIR_RET(fieldType->CanAssignFrom(fieldData.GetType()), );
        fieldType->Copy(fieldData.GetBuffer(), destField);
      }
    }
  }

  void CustomizationData::ApplyCustomization(ComponentName iName, DynObject& ioData) const
  {
    TupleType const* objType = EngineCommon::GetComponents().GetComponentTypeFromName(iName)->IsTuple();
    eXl_ASSERT_REPAIR_RET(ioData.GetType() == objType, );

    auto compCusto = m_ComponentCustomization.find(iName);
    if (compCusto != m_ComponentCustomization.end())
    {
      ApplyCustomization(ioData, compCusto->second);
    }
  }

  void CustomizationData::ApplyCustomization(PropertySheetName iName, DynObject& ioData) const
  {
    //TupleType const* objType = GameDatabase::GetTypeFromName(iName)->IsTuple();
    //eXl_ASSERT_REPAIR_RET(ioData.GetType() == objType, );

    auto compCusto = m_PropertyCustomization.find(iName);
    if (compCusto != m_PropertyCustomization.end())
    {
      ApplyCustomization(ioData, compCusto->second);
    }
  }

  void Archetype::SetProperty(PropertySheetName iName, ConstDynObject const& iObject, bool iInstanced)
  {
    //eXl_ASSERT(GameDatabase::GetTypeFromName(iName) == iObject.GetType());
    auto iter = m_Properties.find(iName);
    if (iter == m_Properties.end())
    {
      iter = m_Properties.insert(std::make_pair(iName, PropertyEntry())).first;
    }
    iter->second.m_Data = DynObject(&iObject);
    iter->second.m_Instanced = iInstanced;
  }

  ConstDynObject const& Archetype::GetProperty(PropertySheetName iName) const
  {
    static ConstDynObject s_Empty;
    auto iter = m_Properties.find(iName);
    if (iter != m_Properties.end())
    {
      return iter->second.m_Data;
    }
    return s_Empty;
  }

  void Archetype::RemoveProperty(PropertySheetName iName)
  {
    m_Properties.erase(iName);
  }

  void Archetype::SetComponent(ComponentName iName, ConstDynObject const& iObject)
  {
    //eXl_ASSERT(GameDatabase::GetComponentTypeFromName(iName) == iObject.GetType());
    auto iter = m_Components.find(iName);
    if (iter == m_Components.end())
    {
      iter = m_Components.insert(std::make_pair(iName, DynObject())).first;
    }
    iter->second = DynObject(&iObject);
  }

  ConstDynObject const& Archetype::GetComponent(ComponentName iName) const
  {
    static ConstDynObject s_Empty;
    auto iter = m_Components.find(iName);
    if (iter != m_Components.end())
    {
      return iter->second;
    }
    return s_Empty;
  }

  
  void Archetype::RemoveComponent(ComponentName iName)
  {
    m_Components.erase(iName);
  }

  void PatchObjectReferences(World& iWorld, DynObject& iObj)
  {
    Type const* objectType = iObj.GetType();
    if (objectType == TypeManager::GetType<ObjectReference>())
    {
      ObjectReference* ref = iObj.CastBuffer<ObjectReference>();
      if (ref->GetPersistentId() != 0)
      {
        ObjectHandle handle = ref->Resolve(iWorld);
        if (!handle.IsAssigned())
        {
          LOG_ERROR << "Could not find object in <debug string for field path>";
        }
      }
      
      return;
    }
    if (TupleType const* tuple = objectType->IsTuple())
    {
      for (uint32_t i = 0; i < tuple->GetNumField(); ++i)
      {
        DynObject field;
        if (iObj.GetField(i, field))
        {
          PatchObjectReferences(iWorld, field);
        }
      }
      return;
    }
    if (ArrayType const* arrayType = ArrayType::DynamicCast(objectType))
    {
      uint32_t numElems = arrayType->GetArraySize(iObj.GetBuffer());
      for (uint32_t i = 0; i < numElems; ++i)
      {
        DynObject element;
        if (iObj.GetElement(i, element))
        {
          PatchObjectReferences(iWorld, element);
        }
      }
      return;
    }
  }

  void Archetype::Instantiate(ObjectHandle iHandle, World& iWorld, CustomizationData const* iCusto) const
  {
    if (GameDatabase* db = iWorld.GetSystem<GameDatabase>())
    {
      db->InstantiateArchetype(iHandle, this, iCusto);
      for (auto propEntry : m_Properties)
      {
        if (propEntry.second.m_Instanced)
        {
          DynObject propData = db->ModifyData(iHandle, propEntry.first);
          PatchObjectReferences(iWorld, propData);
        }
      }
    }

    for (auto const& component : m_Components)
    {
      DynObject componentData = component.second.Ref();
      componentData = DynObject(&componentData);
      if (iCusto)
      {
        auto iter = iCusto->m_ComponentCustomization.find(component.first);
        if (iter != iCusto->m_ComponentCustomization.end())
        {
          
          iCusto->ApplyCustomization(component.first, componentData);
        }
      }
      PatchObjectReferences(iWorld, componentData);
      ComponentFactory const* factory = iWorld.GetComponents().GetComponentFactory(component.first);
      if (factory && (*factory))
      {
        (*factory)(iWorld, iHandle, componentData);
      }
    }
  }
}