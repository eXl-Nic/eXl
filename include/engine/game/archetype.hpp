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
#include <engine/common/gamedatabase.hpp>

namespace eXl
{
  class ComponentManifest;

  struct EXL_ENGINE_API CustomizationData
  {
    using FieldsMap = UnorderedMap<TypeFieldName, DynObject>;

    UnorderedMap<PropertySheetName, FieldsMap> m_PropertyCustomization;
    UnorderedMap<ComponentName, FieldsMap> m_ComponentCustomization;

    static void ApplyCustomization(DynObject& ioData, FieldsMap const& iFields);

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
      bool m_HasObjRef = false;
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
    void RemoveProperty(PropertySheetName iName, ComponentManifest const& iManifest);
    ConstDynObject const& GetProperty(PropertySheetName iName) const;
    UnorderedMap<PropertySheetName, PropertyEntry> const& GetProperties() const
    {
      return m_Properties;
    }

    void AddComponent(ComponentName iName, ComponentManifest const& iManifest, PropertiesManifest const& iPropDesc);
    void RemoveComponent(ComponentName iName);
    bool HasComponent(ComponentName iName) const;
    UnorderedSet<ComponentName> const& GetComponents() const
    {
      return m_Components;
    }

  protected:
    friend TResourceLoader <Archetype, ResourceLoader>;
    Archetype(ResourceMetaData&);

    UnorderedMap<PropertySheetName, PropertyEntry> m_Properties;
    UnorderedSet<ComponentName> m_Components;
  };

#include "archetype.inl"
}