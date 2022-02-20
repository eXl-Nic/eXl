/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/resource/resource.hpp>
#include <core/path.hpp>
#include <core/stream/serializer.hpp>
#include <engine/enginelib.hpp>

namespace eXl
{ 
  class Archetype;
  class MapResource;
  class ProjectLoader;
  class PropertiesManifest;

  class EXL_ENGINE_API Project : public Resource
  {
    DECLARE_RTTI(Project, Resource);
  public:
    static void Init();

    static ResourceLoaderName StaticLoaderName();

    static Project* Create(String const& iName);

    struct Field
    {
      TypeFieldName m_Name;
      TypeName m_TypeName;
      bool m_IsArray;
      SERIALIZE_METHODS;
    };

    struct Typedecl
    {
      Vector<Field> m_Fields;
      SERIALIZE_METHODS;
    };

    ResourceHandle<Archetype> m_PlayerArchetype;
    ResourceHandle<MapResource> m_StartupMap;
    UnorderedMap<TypeName, Typedecl> m_Types;

    struct ProjectTypes
    {
      Vector<std::unique_ptr<Type const>> m_Types;
    };
    void FillProperties(ProjectTypes& oTypes, PropertiesManifest& oManifest) const;

    ~Project();
  protected:

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
    uint32_t ComputeHash() override;

  private:
    Err Serialize(Serializer iSerializer);

    friend ProjectLoader;
    Project(ResourceMetaData&);
  };
}