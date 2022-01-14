/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/archetype.hpp>
#include <engine/common/world.hpp>
#include <engine/game/commondef.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <boost/optional.hpp>

#include <core/type/tupletype.hpp>
#include <core/type/arraytype.hpp>

namespace eXl
{
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
      iStreamer.Write(&component);
    }
    iStreamer.EndSequence();
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  Err Archetype::Unstream_Data(Unstreamer& iStreamer)
  {
    auto& manifest = *iStreamer.GetManifest<PropertiesManifest>();
    auto& compManifest = *iStreamer.GetManifest<ComponentManifest>();
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
        iStreamer.ReadString(&tempStr);
        
        ComponentName name(tempStr);
        UnorderedSet<PropertySheetName> const* reqData = compManifest.GetRequiredDataForComponent(name);
        eXl_ASSERT_REPAIR_BEGIN(reqData != nullptr){}
        else
        {
          AddComponent(name, compManifest, manifest);
        }
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

  bool HasObjectReferences(Type const* iType)
  {
    if (iType == TypeManager::GetType<ObjectReference>())
    {
      return true;
    }
    if (TupleType const* tuple = iType->IsTuple())
    {
      for (uint32_t i = 0; i < tuple->GetNumField(); ++i)
      {
        Type const* fieldType = tuple->GetFieldDetails(i);
        
        if(HasObjectReferences(fieldType))
        {
          return true;
        }
      }
      return false;
    }
    if (ArrayType const* arrayType = ArrayType::DynamicCast(iType))
    {
      return HasObjectReferences(arrayType->GetElementType());
    }

    return false;
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
    iter->second.m_HasObjRef = HasObjectReferences(iObject.GetType());
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

  void Archetype::RemoveProperty(PropertySheetName iName, ComponentManifest const& iManifest)
  {
    m_Properties.erase(iName);
    SmallVector<ComponentName, 2> componentsToRemove;
    for (auto comp : m_Components)
    {
      UnorderedSet<PropertySheetName> const* iProperties = iManifest.GetRequiredDataForComponent(comp);
      if (iProperties == nullptr)
      {
        continue;
      }

    }
  }

  void Archetype::AddComponent(ComponentName iName, ComponentManifest const& iManifest, PropertiesManifest const& iPropDesc)
  {
    if (HasComponent(iName))
    {
      return;
    }

    UnorderedSet<PropertySheetName> const* properties = iManifest.GetRequiredDataForComponent(iName);
    if (properties == nullptr)
    {
      return;
    }

    for (auto prop : *properties)
    {
      if (m_Properties.count(prop) == 0)
      {
        Type const* type = iPropDesc.GetTypeFromName(prop);
        eXl_ASSERT_REPAIR_BEGIN(type != nullptr)
        {
          continue;
        }
        DynObject defaultData;
        defaultData.SetType(type, type->Build(), true);
        SetProperty(prop, defaultData, true);
      }
    }
    m_Components.emplace(iName);
  }

  bool Archetype::HasComponent(ComponentName iName) const
  {
    return m_Components.count(iName) > 0;
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
        if (propEntry.second.m_Instanced
          && propEntry.second.m_HasObjRef)
        {
          DynObject propData = db->ModifyData(iHandle, propEntry.first);
          PatchObjectReferences(iWorld, propData);
        }
      }
    }

    for (auto const& component : m_Components)
    {
      ComponentFactory const* factory = iWorld.GetComponents().GetComponentFactory(component);
      if (factory && (*factory))
      {
        (*factory)(iWorld, iHandle);
      }
    }
  }
}