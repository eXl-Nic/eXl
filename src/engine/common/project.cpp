#include <engine/common/project.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>

#include <engine/game/archetype.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(Project);
  IMPLEMENT_SERIALIZE_METHODS(Project::Typedecl);
  IMPLEMENT_SERIALIZE_METHODS(Project::Field);

  Err Project::Field::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Name");
    iSerializer &= m_Name;
    iSerializer.PopKey();
    iSerializer.PushKey("TypeName");
    iSerializer &= m_TypeName;
    iSerializer.PopKey();
    iSerializer.PushKey("IsArray");
    iSerializer &= m_IsArray;
    iSerializer.PopKey();
    iSerializer.EndStruct();

    return Err::Success;
  }

  Err Project::Typedecl::Serialize(Serializer iSerializer)
  {
    return iSerializer &= m_Fields;
  }

  class ProjectLoader : public ResourceLoader
  {

  public:

    static ProjectLoader& Get()
    {
      static ProjectLoader s_This;
      return s_This;
    }

    ProjectLoader()
      : ResourceLoader(Project::StaticLoaderName(), 1)
    {
      
    }

    Project* Create(String const& iName)
    {
      ResourceMetaData* metaData = CreateNewMetaData(iName);

      return eXl_NEW Project(*metaData);
    }
  protected:
    Project* Create_Impl(ResourceMetaData* iMetaData) const override
    {
      return eXl_NEW Project(*iMetaData);
    }
  };

  void Project::Init()
  {
    ResourceManager::AddLoader(&ProjectLoader::Get(), Project::StaticRtti());
  }

  ResourceLoaderName Project::StaticLoaderName()
  {
    static ResourceLoaderName s_Name("Project");
    return s_Name;
  }

  Project* Project::Create(String const& iName)
  {
    return ProjectLoader::Get().Create(iName);
  }

  Project::Project(ResourceMetaData& iMetaData)
    : Resource(iMetaData)
  {

  }

  Err Project::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<Project*>(this)->Serialize(Serializer(iStreamer));
  }

  Err Project::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  Err Project::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("ProjectTypes");
    iSerializer.HandleMap(m_Types);
    iSerializer.PopKey();
    iSerializer.EndStruct();

    return Err::Success;
  }

  uint32_t Project::ComputeHash()
  {
    return 0;
  }

  void Project::FillProperties(ProjectTypes& oTypes, PropertiesManifest& oManifest) const
  {
    for (auto const& typeDecl : m_Types)
    {
      TypeName const& typeName = typeDecl.first;

      List<FieldDesc> fields;
      size_t curOffset = 0;
      for (auto const& field : typeDecl.second.m_Fields)
      {
        Type const* fieldType = TypeManager::GetCoreTypeFromName(field.m_TypeName);
        if (field.m_IsArray)
        {
          fieldType = TypeManager::GetArrayType(fieldType);
        }
        eXl_ASSERT(fieldType != nullptr);
        fields.push_back(FieldDesc(field.m_Name, curOffset, fieldType));
        curOffset += fieldType->GetSize();
      }

      TupleType* newType = TupleTypeStruct::Create(fields, typeName);
      oTypes.m_Types.push_back(std::unique_ptr<TupleType>(newType));
      oManifest.RegisterPropertySheet(PropertySheetName(typeName.c_str()), newType, true);
    }
  }

}