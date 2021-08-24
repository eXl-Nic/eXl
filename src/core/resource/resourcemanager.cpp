/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/resource/resourcemanager.hpp>
#include <core/stream/stream_base.hpp>
#include <core/stream/jsonunstreamer.hpp>
#include <core/stream/jsonstreamer.hpp>
#include <core/lua/luascript.hpp>
#include <core/log.hpp>
#include <boost/uuid/random_generator.hpp>
#include <core/type/resourcehandletype.hpp>
#include <core/type/typemanager.hpp>
#include <fstream>

#define CLEAR_WHITESPACES  do {\
if (!iReader.ClearWhiteSpaces()) \
{                                \
  return header;                 \
} \
}while(false)

#define EXPECT_CHAR(c) do {\
if (iReader.get() != c) \
{                       \
return header;        \
} \
}while(false)

#define EXPECT_STRING(s, statement) do{\
for (char c : s)           \
{                          \
  if (iReader.get() != c)  \
  {                        \
    iReader.reset();       \
    statement;             \
  }                        \
} \
}while(false)

namespace eXl
{
  namespace ResourceManager
  {
    using UUIDToEntryMap = UnorderedMap<boost::uuids::uuid, ResourceMetaData*>;
    using PathToEntryMap = UnorderedMap<String, ResourceMetaData*>;

    struct ResourceTypeEntry
    {
      ResourceLoader* loader;
      Rtti const* rtti;
      Type const* handleType;
    };

    struct Impl
    {
      UnorderedMap<ResourceLoaderName, ResourceTypeEntry> m_Loaders;
      UnorderedMap<Rtti const*, ResourceLoaderName> m_RttiToLoader;

      UUIDToEntryMap m_UUIDToEntry;
      PathToEntryMap m_PathToEntry;

      boost::uuids::random_generator m_UUIDGen;

      TextFileReadFactory m_TextFileRead;

      UnorderedMap<Rtti const*, RttiObject const*> m_Manifests;
    };

    Impl& GetImpl()
    {
      static Impl s_Mgr;
      return s_Mgr;
    }
  }

  struct ResourceMetaData: public HeapObject
  {
    Resource::Header m_Header;
    String m_Path;
    Resource* m_Rsc;
  };

  Resource::Header const& Resource::GetHeader() const
  {
    return m_MetaData.m_Header;
  }

  void Resource::SetFlags(uint32_t iFlag)
  {
    m_MetaData.m_Header.m_Flags |= iFlag;
  }

  void Resource::ClearFlags(uint32_t iFlag)
  {
    m_MetaData.m_Header.m_Flags &= (~iFlag);
  }

  ResourceMetaData* ResourceLoader::CreateNewMetaData(String const& iName) const
  {
    auto metaData = eXl_NEW ResourceMetaData;
    metaData->m_Rsc = nullptr;
    metaData->m_Header.m_ResourceName = iName;
    metaData->m_Header.m_ResourceId.uuid = ResourceManager::GetImpl().m_UUIDGen();
    metaData->m_Header.m_LoaderName = m_Name;
    metaData->m_Header.m_LoaderVersion = m_Version;

    ResourceManager::GetImpl().m_UUIDToEntry.insert(std::make_pair(metaData->m_Header.m_ResourceId.uuid, metaData));

    return metaData;
  }

  ResourceMetaData* ResourceLoader::CreateBakedMetaData(ResourceMetaData const& iMetaData) const
  {
    auto metaData = eXl_NEW ResourceMetaData;
    metaData->m_Rsc = nullptr;
    metaData->m_Header.m_ResourceName = iMetaData.m_Header.m_ResourceName;
    metaData->m_Header.m_ResourceId = iMetaData.m_Header.m_ResourceId;
    metaData->m_Header.m_LoaderName = iMetaData.m_Header.m_LoaderName;
    metaData->m_Header.m_LoaderVersion = iMetaData.m_Header.m_LoaderVersion;
    metaData->m_Header.m_Flags = iMetaData.m_Header.m_Flags | Resource::BakedResource;

    return metaData;
  }

  Resource::Resource(ResourceMetaData& iMetaData)
    : m_MetaData(iMetaData)
  {
    m_MetaData.m_Rsc = this;
  }

  Stream_Base::Stream_Base()
  {
    ResourceManager::PopulateManifests(*this);
  }

  namespace ResourceManager
  {
    void AddManifest(RttiObject const& iManifest)
    {
      GetImpl().m_Manifests.insert(std::make_pair(&iManifest.GetRtti(), &iManifest));
    }

    void RemoveManifest(RttiObject const& iManifest)
    {
      GetImpl().m_Manifests.erase(&iManifest.GetRtti());
    }

    void RemoveManifest(Rtti const& iManifest)
    {
      GetImpl().m_Manifests.erase(&iManifest);
    }

    void PopulateManifests(Stream_Base& iStreamer)
    {
      for (auto const& entry : GetImpl().m_Manifests)
      {
        iStreamer.AddManifest(*entry.second);
      }
    }

    String const& GetAssetExtension()
    {
      static const String s_Extension(".eXlAsset");
      return s_Extension;
    }

    void SetTextFileReadFactory(TextFileReadFactory iFactory)
    {
      GetImpl().m_TextFileRead = std::move(iFactory);
    }

    ResourceLoaderName GetLoaderFromRtti(Rtti const& iRtti)
    {
      auto iter = GetImpl().m_RttiToLoader.find(&iRtti);
      if (iter != GetImpl().m_RttiToLoader.end())
      {
        return iter->second;
      }

      return ResourceLoaderName("");
    }

    Type const* GetHandleType(Rtti const& iRtti)
    {
      auto iter = GetImpl().m_RttiToLoader.find(&iRtti);
      if (iter != GetImpl().m_RttiToLoader.end())
      {
        return GetImpl().m_Loaders[iter->second].handleType;
      }

      return nullptr;
    }

    Type const* GetHandleType(ResourceLoaderName iLoaderName)
    {
      auto iter = GetImpl().m_Loaders.find(iLoaderName);
      if (iter != GetImpl().m_Loaders.end())
      {
        return iter->second.handleType;
      }

      return nullptr;
    }

    void AddLoader(ResourceLoader* iLoader, Rtti const& iRtti)
    {
      if (iLoader)
      {
        auto loaderName = iLoader->GetName();
        eXl_ASSERT(GetImpl().m_Loaders.count(loaderName) == 0);
        eXl_ASSERT(GetImpl().m_RttiToLoader.count(&iRtti) == 0);

        ResourceTypeEntry newType;
        newType.loader = iLoader;
        newType.rtti = &iRtti;
        newType.handleType = new ResourceHandleType(iRtti);
        TypeManager::RegisterType(newType.handleType);
        TypeManager::RegisterArrayType(new CoreArrayType<ResourceHandle<Resource>>(newType.handleType));

        GetImpl().m_Loaders.emplace(std::make_pair(loaderName, std::move(newType)));
        GetImpl().m_RttiToLoader.emplace(std::make_pair(&iRtti, loaderName));
      }
    }

    Vector<ResourceLoaderName> ListLoaders()
    {
      Vector<ResourceLoaderName> loaders;
      for (auto const& entry : GetImpl().m_Loaders)
      {
        loaders.push_back(entry.first);
      }

      return loaders;
    }

    ResourceLoader* GetLoader(ResourceLoaderName iLoaderName)
    {
      auto iter = GetImpl().m_Loaders.find(iLoaderName);
      return iter != GetImpl().m_Loaders.end() ? iter->second.loader : nullptr;
    }

    Resource* LoadExpectedType(Resource::UUID const& iUUID, const ResourceLoaderName& iExpectedLoader)
    {
      ResourceLoaderName actualResourceName;
      Resource* loadedResource = Load(iUUID, &actualResourceName);

      eXl_ASSERT_MSG_REPAIR_RET((!loadedResource || actualResourceName == iExpectedLoader), "Unexpected resource type", nullptr);
      
      return loadedResource;
    }

    Resource* LoadEntry(ResourceMetaData* iEntry)
    {
      Resource* loadedRsc = nullptr;
      auto foundLoader = GetImpl().m_Loaders.find(iEntry->m_Header.m_LoaderName);
      eXl_ASSERT(foundLoader != GetImpl().m_Loaders.end());
      if (auto reader = GetImpl().m_TextFileRead(iEntry->m_Path.c_str()))
      {
        loadedRsc = foundLoader->second.loader->Load(iEntry->m_Header, iEntry, *reader);
      }
      else
      {
        LOG_ERROR << "Could not open asset " << iEntry->m_Path << "\n";
      }
      return loadedRsc;
    }

    Resource* Load(Resource::UUID const& iUUID, ResourceLoaderName* oLoader)
    {
      Resource* loadedRsc = nullptr;
      auto foundEntry = GetImpl().m_UUIDToEntry.find(iUUID.uuid);
      if (foundEntry != GetImpl().m_UUIDToEntry.end())
      {
        ResourceMetaData* entry = foundEntry->second;
        if (entry->m_Rsc == nullptr)
        {
          loadedRsc = LoadEntry(entry);
          if (loadedRsc)
          {
            entry->m_Rsc = loadedRsc;
          }
        }
        else
        {
          loadedRsc = entry->m_Rsc;
        }
        if (oLoader)
        {
          *oLoader = entry->m_Header.m_LoaderName;
        }
      }
      else
      {
        LOG_ERROR << "Could not find asset for UUID" << iUUID.uuid_dwords[0] << "-" << 
          iUUID.uuid_dwords[1] << "-" << 
          iUUID.uuid_dwords[2] << "-" << 
          iUUID.uuid_dwords[3] << "-" << "\n";
      }
      return loadedRsc;
    }

    void UnloadUnusedResources()
    {
      for (auto entry : GetImpl().m_UUIDToEntry)
      {
        if (entry.second->m_Rsc && entry.second->m_Rsc->GetRefCount() == 0)
        {
          eXl_DELETE entry.second->m_Rsc;
          entry.second->m_Rsc = nullptr;
        }
      }
    }

    void Reset()
    {
#ifndef EXL_RSC_HAS_FILESYSTEM
      GetImpl().m_PathToEntry.clear();
#endif
      GetImpl().m_UUIDToEntry.clear();
    }

    Vector<Resource::Header> ListResources()
    {
      Vector<Resource::Header> oArray;
      oArray.reserve(GetImpl().m_UUIDToEntry.size());
      for (auto Entry : GetImpl().m_UUIDToEntry)
      {
        oArray.push_back(Entry.second->m_Header);
      }

      return oArray;
    }

    Resource::Header const* GetHeader(Resource::UUID const& iUUID)
    {
      auto iter = GetImpl().m_UUIDToEntry.find(iUUID.uuid);
      if (iter != GetImpl().m_UUIDToEntry.end())
      {
        return &iter->second->m_Header;
      }
      return nullptr;
    }

    String ExtractHeader(TextReader& iReader)
    {
      KString luaHeader(LuaScriptLoader::s_HeaderSection);
      String header;
      {
        bool isLuaHeader = true;
        EXPECT_STRING(luaHeader, isLuaHeader = false);

        if (isLuaHeader)
        {
          while (iReader.peek() != '\n' && iReader.good() && !iReader.eof())
          {
            header.push_back(iReader.get());
          }

          return header;
        }
      }

      EXPECT_CHAR('{');

      CLEAR_WHITESPACES;

      const String headerKey = "\"Header\"";
      EXPECT_STRING(headerKey, return header);
      CLEAR_WHITESPACES;
      EXPECT_CHAR(':');
      CLEAR_WHITESPACES;
      EXPECT_CHAR('{');
      CLEAR_WHITESPACES;
      header.append("{\n");
      int scopeCounter = 1;
      while (scopeCounter > 0 && iReader.good() && !iReader.eof())
      {
        char c = iReader.get();
        if (c == '{')
        {
          ++scopeCounter;
        }
        if (c == '}')
        {
          --scopeCounter;
        }
        header.push_back(c);
      }

      return header;
    }

    Err ProcessFile(char const* iPath, Resource::Header& oHeader)
    {
      std::unique_ptr<TextReader> reader = GetImpl().m_TextFileRead(iPath);
      if (reader)
      {
        String headerStr = ExtractHeader(*reader);
        StringViewReader strReader(iPath, headerStr.data(), headerStr.data() + headerStr.size());

        JSONUnstreamer unstreamer(&strReader);
        unstreamer.Begin();
        oHeader.Unstream(unstreamer);
        unstreamer.End();

        if (oHeader.m_ResourceId.IsValid())
        {
          if (GetImpl().m_Loaders.count(oHeader.m_LoaderName) != 0)
          {
            return Err::Success;
          }
          else
          {
            LOG_ERROR << "Unknown resource type " << oHeader.m_LoaderName.get() << " for asset " << iPath << "\n";
          }
        }
        else
        {
          LOG_ERROR << "Invalid asset " << iPath << "\n";
        }
      }
      else
      {
        LOG_ERROR << "Could not open asset " << iPath << "\n";
      }

      return Err::Failure;
    }

    ResourceMetaData* AddNewEntry(const char* iPath, Resource::Header const& iHeader)
    {
      ResourceMetaData* newEntry = eXl_NEW ResourceMetaData;
      newEntry->m_Header = iHeader;
#ifdef EXL_RSC_HAS_FILESYSTEM
      newEntry->m_Path = ToString(Filesystem::absolute(Filesystem::canonical(iPath)));
#else
      newEntry->m_Path = iPath;
#endif
      newEntry->m_Rsc = nullptr;

      auto alreadyExistingEntry = GetImpl().m_UUIDToEntry.find(iHeader.m_ResourceId.uuid);
#if !defined(EXL_IS_BAKED_PLATFORM) && defined(EXL_RSC_HAS_FILESYSTEM)
      if (alreadyExistingEntry != GetImpl().m_UUIDToEntry.end())
      {

        auto lastNew = Filesystem::last_write_time(iPath);
        Path existingPath(alreadyExistingEntry->second->m_Path.c_str());
        auto lastExisting = Filesystem::last_write_time(existingPath);

        ResourceMetaData* entryToRewrite = lastNew > lastExisting ? newEntry : alreadyExistingEntry->second;
        ResourceMetaData* oldEntry = lastNew > lastExisting ? newEntry : alreadyExistingEntry->second;

        entryToRewrite->m_Header.m_ResourceId.uuid = GetImpl().m_UUIDGen();

        if (alreadyExistingEntry->second == entryToRewrite)
        {
          alreadyExistingEntry->second = newEntry;
        }

        GetImpl().m_PathToEntry.insert(std::make_pair(entryToRewrite->m_Path, entryToRewrite));
        GetImpl().m_UUIDToEntry.insert(std::make_pair(entryToRewrite->m_Header.m_ResourceId.uuid, entryToRewrite));

        LOG_WARNING << "Duplicated resource ID, resource " << entryToRewrite->m_Path << " will be rewritten" << "\n";

        if (Load(entryToRewrite->m_Path.c_str()) != nullptr)
        {
          Err result = Save(entryToRewrite->m_Rsc);
          eXl_ASSERT(!!result);
        }
        else
        {
          eXl_ASSERT(false);
        }
      }
#else
      eXl_ASSERT_MSG_REPAIR_BEGIN(alreadyExistingEntry == GetImpl().m_UUIDToEntry.end(), "Duplicated entries in baked platform!!") {}
#endif
      else

      {
        GetImpl().m_PathToEntry.insert(std::make_pair(newEntry->m_Path, newEntry));
        GetImpl().m_UUIDToEntry.insert(std::make_pair(iHeader.m_ResourceId.uuid, newEntry));
      }

      return newEntry;
    }

    void BootstrapAssetsFromManifest(String const& iDir)
    {
      String manifestPath("eXlManifest");
      bool validDir = true;
#ifdef EXL_RSC_HAS_FILESYSTEM
      Path pathCheck(iDir.c_str());
      if (!(Filesystem::exists(pathCheck) && Filesystem::is_directory(pathCheck)))
      {
        validDir = false;
        return;
      }
      else
#else
      validDir = !iDir.empty();
      if(validDir)
#endif
      {
        manifestPath = iDir + "/" + manifestPath;
      }
      auto reader = GetImpl().m_TextFileRead(manifestPath.c_str());
      if (reader)
      {
        JSONUnstreamer unstreamer(reader.get());
        if (!unstreamer.Begin())
        {
          LOG_ERROR << "Invalid JSON while reading manifest " << manifestPath << "\n";
          return;
        }

        if (unstreamer.BeginSequence())
        {
          do
          {
            String path;
            unstreamer.ReadString(&path);

            if (validDir)
            {
              path = iDir + "/" + path;
            }

            LOG_INFO<< "Found file " << path << "\n";

            Resource::Header header;
            if (ProcessFile(path.c_str(), header))
            {
              AddNewEntry(path.c_str(), header);
            }

          } while (unstreamer.NextSequenceElement());
        }

        if (!unstreamer.End())
        {
          LOG_ERROR << "Unknown error while reading manifest " << manifestPath << "\n";
          return;
        }
      }
      else
      {
        LOG_ERROR << "Could not load manifest " << manifestPath << "\n";
      }
    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    void BootstrapDirectory(Path const& iPath, bool iRecursive)
    {
      if (!Filesystem::is_directory(iPath))
      {
        LOG_ERROR << "Path " << ToString(iPath) << " is not a directory" << "\n";
        return;
      }

      Vector<Path> directoriesToScan;
      directoriesToScan.push_back(iPath);
      while (!directoriesToScan.empty())
      {
        Path toScan = std::move(directoriesToScan.back());
        directoriesToScan.pop_back();
        std::error_code ec;
        for (Filesystem::directory_iterator iter(toScan, ec); iter != Filesystem::directory_iterator(); ++iter)
        {
          Path currentPath = iter->path();
          if (Filesystem::is_directory(currentPath) && iRecursive)
          {
            directoriesToScan.push_back(currentPath);
          }
          else
          {
            if (ToString(currentPath.extension()) == GetAssetExtension())
            {
              Resource::Header header;
              if (ProcessFile(ToString(currentPath).c_str(), header))
              {
                AddNewEntry(ToString(currentPath).c_str(), header);
              }
            }
          }
        }
      }
    }
    Resource* LoadExpectedType(Path const& iPath, const ResourceLoaderName& iExpectedLoader)
    {
      ResourceLoaderName actualResourceName;
      Resource* loadedResource = Load(ToString(iPath.string()).c_str(), &actualResourceName);

      eXl_ASSERT_MSG_REPAIR_RET((!loadedResource || actualResourceName == iExpectedLoader), "Unexpected resource type", nullptr);

      return loadedResource;
    }

    Resource* Load(const char* iPath, ResourceLoaderName* oLoader)
    {
      if (Filesystem::is_directory(iPath) || !Filesystem::exists(iPath))
      {
        LOG_ERROR << "Invalid Path to open" << iPath << "\n";
        return nullptr;
      }

      Resource* loadedRsc = nullptr;

      Path pathToLookFor = Filesystem::absolute(Filesystem::canonical(iPath));

      auto foundEntry = GetImpl().m_PathToEntry.find(ToString(pathToLookFor));
      if (foundEntry != GetImpl().m_PathToEntry.end())
      {
        ResourceMetaData* entry = foundEntry->second;
        if (entry->m_Rsc == nullptr)
        {
          loadedRsc = LoadEntry(entry);
          if (loadedRsc)
          {
            entry->m_Rsc = loadedRsc;
          }
        }
        else
        {
          loadedRsc = entry->m_Rsc;
        }
        if (oLoader)
        {
          *oLoader = entry->m_Header.m_LoaderName;
        }
      }
      else
      {
        Resource::Header header;
        if (ProcessFile(iPath, header))
        {
          auto foundEntry = GetImpl().m_UUIDToEntry.find(header.m_ResourceId.uuid);
          if (foundEntry != GetImpl().m_UUIDToEntry.end())
          {
            return Load(header.m_ResourceId, oLoader);
          }

          ResourceMetaData* entry = AddNewEntry(iPath, header);
          loadedRsc = LoadEntry(entry);
          if (loadedRsc)
          {
            entry->m_Rsc = loadedRsc;
          }
          if (oLoader)
          {
            *oLoader = entry->m_Header.m_LoaderName;
          }
        }
      }

      return loadedRsc;
    }

    Err SaveTo(Resource* iRsc, Path const& iPath)
    {
      if (!iRsc)
      {
        return Err::Failure;
      }

      if (SetPath(iRsc, iPath))
      {
        return Save(iRsc);
      }

      return Err::Failure;
    }

    Err Save(Resource* iRsc)
    {
      if (!iRsc)
      {
        return Err::Failure;
      }

      auto foundEntry = GetImpl().m_UUIDToEntry.find(iRsc->GetHeader().m_ResourceId.uuid);
      eXl_ASSERT_MSG_REPAIR_RET(foundEntry != GetImpl().m_UUIDToEntry.end(), "Unexpected not registered resource", Err::Failure);

      ResourceMetaData* metaData = foundEntry->second;

      {
        if (metaData->m_Path.empty())
        {
          LOG_ERROR << "Cannot save resource " << iRsc->GetHeader().m_ResourceName << " without providing a path" << "\n";
        }
        else
        {
          Path path(metaData->m_Path.c_str());

          bool firstTimeSaved = false;

          if (!Filesystem::exists(path))
          {
            firstTimeSaved = true;
          }
          else
          {
            auto alreadySavedResource = GetImpl().m_PathToEntry.find(metaData->m_Path);
            if (alreadySavedResource != GetImpl().m_PathToEntry.end() && alreadySavedResource->second != metaData)
            {
              LOG_ERROR << "Tried to save " << metaData->m_Header.m_ResourceName << " to " << ToString(path)
                << " but it the resource " << alreadySavedResource->second->m_Header.m_ResourceName << " is already saved there" << "\n";

              return Err::Failure;
            }
          }

          auto loader = GetImpl().m_Loaders.find(iRsc->GetHeader().m_LoaderName);
          eXl_ASSERT(loader != GetImpl().m_Loaders.end());

          Path tempPath = path;
          tempPath = tempPath.replace_extension("_tmp");

          {
            std::ofstream outputStream;
            outputStream.open(tempPath);

            StdOutWriter writer(outputStream);

            Err result = loader->second.loader->Save(iRsc, writer);
            if (!result)
            {
              LOG_ERROR << "Error while saving resource " << iRsc->GetHeader().m_ResourceName << "\n";
              outputStream.close();
              Filesystem::remove(tempPath);
              return Err::Failure;
            }
          }

          std::error_code err;
          if (!Filesystem::remove(path, err))
          {
            std::error_code errDummy;
            if (Filesystem::exists(path, errDummy))
            {
              LOG_ERROR << "Error while trying to replace resource at : " << ToString(path) << ":" << err.message() << "\n";
              return Err::Failure;
            }
          }

          err.clear();
          Filesystem::rename(tempPath, path, err);
          if (!err)
          {
            LOG_ERROR << "Error while trying to replace resource at : " << ToString(path) << ":" << err.message() << "\n";
            return Err::Failure;
          }

          if (firstTimeSaved)
          {
            std::error_code ec;
            Path sanitizedPath = Filesystem::absolute(Filesystem::canonical(path));
            path = sanitizedPath;
            GetImpl().m_PathToEntry.insert(std::make_pair(ToString(sanitizedPath), foundEntry->second));
          }

          return Err::Success;
        }
      }

      return Err::Failure;
    }

    Path GetPath(Resource::UUID const& iUUID)
    {
      auto iter = GetImpl().m_UUIDToEntry.find(iUUID.uuid);
      if (iter != GetImpl().m_UUIDToEntry.end())
      {
        return Path(iter->second->m_Path.c_str());
      }
      return Path();
    }

    Err SetPath(Resource* iRsc, Path const& iPath)
    {
      if (!iRsc)
      {
        return Err::Failure;
      }

      auto foundEntry = GetImpl().m_UUIDToEntry.find(iRsc->GetHeader().m_ResourceId.uuid);
      eXl_ASSERT_MSG_REPAIR_RET(foundEntry != GetImpl().m_UUIDToEntry.end(), "Unexpected not registered resource", Err::Failure);
      
      ResourceMetaData* metaData = foundEntry->second;

      bool bPathExists = Filesystem::exists(iPath);
      bool bIsDirectory = bPathExists && Filesystem::is_directory(iPath);
      
      if (metaData->m_Path.empty())
      {
        Path candidatePath = iPath;
        if (bIsDirectory)
        {
          String const& rscName = iRsc->GetName();
          if (rscName.empty())
          {
            LOG_ERROR << "Given empty resource name to save to" << ToString(iPath) << "\n";
            return Err::Failure;
          }
          candidatePath = iPath / rscName.c_str();
          candidatePath.replace_extension(GetAssetExtension().c_str());
          bPathExists = Filesystem::exists(candidatePath);
        }

        if (bPathExists)
        {
          LOG_ERROR << "Cannot save resource to overwrite" << ToString(candidatePath) << "\n";
          return Err::Failure;
        }

        metaData->m_Path = ToString(candidatePath);

        return Err::Success;
      }
      else
      {
        if (bPathExists)
        {
          std::error_code ec;
          Path sanitizedPath = Filesystem::absolute(Filesystem::canonical(iPath));
          if (!ec || ToString(sanitizedPath) != metaData->m_Path)
          {
            LOG_ERROR << "Cannot change saved resource " << iRsc->GetName() << " path to : " << ToString(iPath) << "\n";
            return Err::Failure;
          }
        }
        if(bIsDirectory)
        {
          LOG_ERROR << "Cannot change resource " << iRsc->GetName() << " path to a directory : " << ToString(iPath) << "\n";
          return Err::Failure;
        }
        else
        {
          if (iRsc->HasFlag(Resource::LockPathDirectory))
          {
            Path existingDir = Path(metaData->m_Path.c_str()).parent_path();
            Path newDir = iPath.parent_path();

            if (!Filesystem::exists(existingDir) || !Filesystem::exists(newDir))
            {
              LOG_ERROR << "Cannot change resource " << iRsc->GetName() << " path to non-existing directory : " << ToString(iPath) << "\n";
              return Err::Failure;
            }
            existingDir = Filesystem::absolute(Filesystem::canonical(existingDir));
            newDir = Filesystem::absolute(Filesystem::canonical(newDir));
            if (existingDir != newDir)
            {
              LOG_ERROR << "Cannot change resource " << iRsc->GetName() << " path to a different directory : " << ToString(iPath) << " because resource dir is locked" << "\n";
              return Err::Failure;
            }
          }
          metaData->m_Path = iPath.string().c_str();

          return Err::Success;
        }
      }
    }

    bool HasFile(Resource::UUID const& iUUID)
    {
      auto foundEntry = GetImpl().m_UUIDToEntry.find(iUUID.uuid);
      eXl_ASSERT_MSG_REPAIR_RET(foundEntry != GetImpl().m_UUIDToEntry.end(), "Unexpected not registered resource", false);

      ResourceMetaData* metaData = foundEntry->second;

      if (metaData->m_Path.empty())
      {
        return false;
      }

      auto alreadySavedResource = GetImpl().m_PathToEntry.find(metaData->m_Path);
      return alreadySavedResource != GetImpl().m_PathToEntry.end();
    }

    void Bake(Path const& iDest)
    {
      eXl_ASSERT_REPAIR_RET(Filesystem::exists(iDest) && Filesystem::is_directory(iDest), );

      UnorderedSet<String> fileNames;
      for (auto entry : GetImpl().m_UUIDToEntry)
      {
        ResourceLoaderName resourceType;
        Resource* rsc = Load(entry.second->m_Path.c_str(), &resourceType);
        if (rsc)
        {
          String pathStr;
          Resource::UUID idToHash;
          uint32_t counter = 0;
          do
          {
            size_t hash = counter;
            for (uint32_t dword : rsc->GetHeader().m_ResourceId.uuid_dwords)
            {
              boost::hash_combine(hash, dword);
            }

            pathStr = StringUtil::FromSizeT(hash).c_str();
            pathStr += GetAssetExtension().c_str();

            ++counter;
          } while (fileNames.count(pathStr) > 0);

          Path completePath = iDest / Path(pathStr.c_str());

          ResourceLoader& loader = *GetImpl().m_Loaders[resourceType].loader;
          std::ofstream outputStream;
          outputStream.open(completePath);

          StdOutWriter writer(outputStream);
          
          if (loader.NeedsBaking(rsc))
          {
            Resource* bakedRsc = loader.CreateBakedResource(rsc);
            Err result = loader.Save(bakedRsc, writer);
          }
          else
          {
            Err result = loader.Save(rsc, writer);
          }

          fileNames.insert(pathStr);
        }
      }

      Path manifestPath = iDest / "eXlManifest";

      std::ofstream outputStream;
      outputStream.open(manifestPath);

      JSONStreamer streamer(&outputStream);
      streamer.Begin();
      streamer.BeginSequence();
      for (auto const& file : fileNames)
      {
        streamer.Write(&file);
      }
      streamer.EndSequence();
      streamer.End();
    }
#endif
  }
}