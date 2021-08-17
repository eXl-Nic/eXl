#pragma once

#include <core/resource/resource.hpp>
#include <core/path.hpp>
#include <core/stream/serializer.hpp>
#include <engine/enginelib.hpp>

namespace eXl
{ 
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

    UnorderedMap<TypeName, Typedecl> m_Types;

    struct ProjectTypes
    {
      Vector<std::unique_ptr<Type const>> m_Types;
    };
    void FillProperties(ProjectTypes& oTypes, PropertiesManifest& oManifest) const;

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