/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/project.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>

#include <engine/common/gamedatabase.hpp>
#include <engine/game/archetype.hpp>
#include <engine/map/map.hpp>
#include <engine/script/eventsystem.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(Project);
  IMPLEMENT_SERIALIZE_METHODS(Project::TypeDecl);
  IMPLEMENT_SERIALIZE_METHODS(Project::Field);
  IMPLEMENT_SERIALIZE_METHODS(Project::FunctionDecl);

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

  Err Project::TypeDecl::Serialize(Serializer iSerializer)
  {
    return iSerializer &= m_Fields;
  }

  Err Project::FunctionDecl::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Args");
    iSerializer &= m_Fields;
    iSerializer.PopKey();
    iSerializer.PushKey("Ret");
    iSerializer &= m_Ret;
    iSerializer.PopKey();
    iSerializer.EndStruct();

    return Err::Success;
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
    ResourceManager::AddLoader(&ProjectLoader::Get(), Project::StaticRtti(), GetType());
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

  Project::~Project() = default;

  Err Project::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<Project*>(this)->Serialize(Serializer(iStreamer));
  }

  Err Project::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  template <>
  struct StreamerTemplateHandler<UnorderedMap<String, Project::FunctionDecl> >
  {
    static Err Do(Streamer& iStreamer, UnorderedMap<String, Project::FunctionDecl> const* iObj)
    {
      Serializer serializer(iStreamer);
      serializer.HandleMapSorted(const_cast<UnorderedMap<String, Project::FunctionDecl>&>(*iObj));
      return Err::Success;
    }
  };

  template <>
  struct UnstreamerTemplateHandler<UnorderedMap<String, Project::FunctionDecl> >
  {
    static Err Do(Unstreamer& iStreamer, UnorderedMap<String, Project::FunctionDecl>* iObj)
    {
      Serializer serializer(iStreamer);
      serializer.HandleMapSorted(*iObj);
      return Err::Success;
    }
  };

  Err Project::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    if (iSerializer.PushKey("GameDll"))
    {
      iSerializer &= m_GameDll;
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("PlayerArchetype"))
    {
      iSerializer &= m_PlayerArchetype;
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("StartupMap"))
    {
      iSerializer &= m_StartupMap;
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("ProjectTypes"))
    {
      iSerializer.HandleMapSorted(m_Types);
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("Events"))
    {
      iSerializer.HandleMapSorted(m_Events);
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("ClientCommands"))
    {
      iSerializer.HandleMapSorted(m_ClientCommands);
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("ServerCommands"))
    {
      iSerializer.HandleMapSorted(m_ServerCommands);
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("GameSettings"))
    {
      iSerializer.HandleMapSorted(m_GameSettings);
      iSerializer.PopKey();
    }
    if (iSerializer.PushKey("PlayerAdditionalParameters"))
    {
      iSerializer &= m_PlayerAdditionalParameters;
      iSerializer.PopKey();
    }
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
        eXl_ASSERT_MSG(fieldType != nullptr, eXl_FORMAT("Invalid type name %s", field.m_TypeName.c_str()));
        fields.push_back(FieldDesc(field.m_Name, curOffset, fieldType));
        curOffset += fieldType->GetSize();
      }

      TupleType* newType = TupleTypeStruct::Create(fields, typeName);
      oTypes.m_Types.push_back(std::unique_ptr<TupleType>(newType));
      oManifest.RegisterPropertySheet(PropertySheetName(typeName.c_str()), newType);
    }
  }

  static void ProcessDecl(String const& iFunName, Project::FunctionDecl const& iDecl, EventsManifest::FunctionsMap& oMap)
  {
    Type const* retType = nullptr;
    if (!iDecl.m_Ret.empty() && iDecl.m_Ret != "void")
    {
      retType = TypeManager::GetCoreTypeFromName(iDecl.m_Ret);
      eXl_ASSERT_MSG(retType != nullptr, eXl_FORMAT("Invalid type name %s", iDecl.m_Ret.c_str()));
    }
    Vector<Type const*> args;
    for (auto const& field : iDecl.m_Fields)
    {
      Type const* fieldType = TypeManager::GetCoreTypeFromName(field.m_TypeName);
      eXl_ASSERT_MSG(fieldType != nullptr, eXl_FORMAT("Invalid type name %s", field.m_TypeName.c_str()));
      if (field.m_IsArray)
      {
        fieldType = TypeManager::GetArrayType(fieldType);
      }
      args.push_back(fieldType);
    }

    oMap.emplace(iFunName, FunDesc(retType, std::move(args)));
  }

  void Project::FillEvents(EventsManifest& oManifest) const
  {
    {
      auto insertRet = oManifest.m_Interfaces.insert(std::make_pair(s_ClientInterface, EventsManifest::FunctionsMap()));
      eXl_ASSERT_MSG(insertRet.second, eXl_FORMAT("Duplicated interface name %s", s_ClientInterface));
      auto& funMap = insertRet.first->second;

      for (auto const& funDesc : m_ClientCommands)
      {
        ProcessDecl(funDesc.first, funDesc.second, funMap);
      }
    }

    {
      auto insertRet = oManifest.m_Interfaces.insert(std::make_pair(s_ServerInterface, EventsManifest::FunctionsMap()));
      eXl_ASSERT_MSG(insertRet.second, eXl_FORMAT("Duplicated interface name %s", s_ServerInterface));
      auto& funMap = insertRet.first->second;

      for (auto const& funDesc : m_ServerCommands)
      {
        ProcessDecl(funDesc.first, funDesc.second, funMap);
      }
    }

    for (auto const& itf : m_Events)
    {
      String const& itfName = itf.first;
      auto insertRet = oManifest.m_Interfaces.insert(std::make_pair(itfName, EventsManifest::FunctionsMap()));
      eXl_ASSERT_MSG(insertRet.second, eXl_FORMAT("Duplicated interface name %s", itfName.c_str()));
      auto& funMap = insertRet.first->second;

      for (auto const& funDesc : itf.second)
      {
        ProcessDecl(funDesc.first, funDesc.second, funMap);
      }
    }
  }

}