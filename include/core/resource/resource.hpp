/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/heapobject.hpp>
#include <core/rtti.hpp>
#include <core/name.hpp>
#include <core/refcobject.hpp>
#include <boost/uuid/uuid.hpp>

#ifdef __ANDROID__
#define EXL_IS_BAKED_PLATFORM
#endif

namespace eXl
{
  class Streamer;
  class Unstreamer;

  //struct ResourceLoaderTag {};
  //using ResourceLoaderName = Name;
  MAKE_NAME(ResourceLoaderName);

  class ResourceLoader;
  template <typename T, typename BaseLoader>
  class TResourceLoader;

  struct ResourceMetaData;

  class EXL_CORE_API Resource : public RttiObject
  {
    DECLARE_RTTI(Resource, RttiObject)
    DECLARE_RefC;
  public:

    struct EXL_CORE_API UUID
    {
      UUID()
      {
        Clear();
      }

      UUID(std::initializer_list<uint32_t> iList)
      {
        std::copy(iList.begin(), iList.end(), uuid_dwords);
      }

      void Clear()
      {
        memset(uuid_dwords, 0, sizeof(uuid_dwords));
      }

      bool IsValid() const
      {
        return uuid_dwords[0] != 0 && uuid_dwords[1] != 0 &&
          uuid_dwords[2] != 0 && uuid_dwords[3] != 0;
      }

      union
      {
        uint32_t uuid_dwords[4];
        boost::uuids::uuid uuid;
      };

      bool operator==(Resource::UUID const& iRhs) const
      {
        return uuid == iRhs.uuid;
      }

      bool operator!=(Resource::UUID const& iRhs) const
      {
        return !(*this == iRhs);
      }
      String ToString() const;
      Err Stream(Streamer& iStreamer) const;
      Err Unstream(Unstreamer& iStreamer);
    };

    enum Flags
    {
      LockPathDirectory = 1 << 0,
      BakedResource = 1 << 1,
    };

    struct Header
    {
      UUID m_ResourceId;
      ResourceLoaderName m_LoaderName;
      String m_ResourceName;
      uint32_t m_ResourceHash;
      uint32_t m_LoaderVersion;
      uint32_t m_Flags = 0;

      Err Stream(Streamer& iStreamer) const;
      Err Unstream(Unstreamer& iStreamer);
    };

    virtual ~Resource();

    Header const& GetHeader() const;
    String const& GetName() const { return GetHeader().m_ResourceName; }
    bool HasFlag(Flags iFlag) const { return (GetHeader().m_Flags & iFlag) != 0; };
    ResourceMetaData& GetMetaData() { return m_MetaData; }

    Err Stream(Streamer& iStreamer) const;
    Err Unstream(Unstreamer& iStreamer);

    virtual Err Stream_Data(Streamer& iStreamer) const = 0;
    virtual Err Unstream_Data(Unstreamer& iStreamer) = 0;

    virtual uint32_t ComputeHash() = 0;

    uint32_t GetRefCount() const { return m_RefCount; }

   protected:

    virtual void PostLoad();

    void OnNullRefC() const {}

    void SetFlags(uint32_t iFlag);
    void ClearFlags(uint32_t iFlag);

    Resource(ResourceMetaData&);

    ResourceMetaData& m_MetaData;
  private:
    Resource(Resource const&) = delete;
    Resource& operator=(Resource const&) = delete;
  };

  namespace ResourceManager
  {
    EXL_CORE_API Resource* LoadExpectedType(Resource::UUID const& iUUID, const ResourceLoaderName& iExpectedLoader);
    EXL_CORE_API Resource* Load(Resource::UUID const& iUUID, ResourceLoaderName*);
  }

  template <typename T>
  struct ResourceLoadingHandler
  {
    static T* Load(Resource::UUID const& iUUID)
    {
      return static_cast<T*>(ResourceManager::LoadExpectedType(iUUID, T::StaticLoaderName()));
    }
  };
  
  template <>
  struct ResourceLoadingHandler<Resource>
  {
    static Resource* Load(Resource::UUID const& iUUID)
    {
      return ResourceManager::Load(iUUID, nullptr);
    }
  };

  template <typename T>
  class ResourceHandle
  {
  public:

    bool IsLoaded() const { return m_Resource != nullptr; }

    void Set(T const* iResource)
    {
      if (iResource)
      {
        m_ResourceUUID = iResource->GetHeader().m_ResourceId;
        m_Resource = iResource;
      }
      else
      {
        m_ResourceUUID.Clear();
        m_Resource = nullptr;
      }
    }

    T const* Get() const { return m_Resource.get(); }
    T const* GetOrLoad() const 
    {
      Load();
      return m_Resource.get(); 
    }

    Resource::UUID const& GetUUID() const { return m_ResourceUUID; }

    Err Load() const
    {
      if (!m_Resource)
      {
        m_Resource = ResourceLoadingHandler<T>::Load(m_ResourceUUID);
      }

      return m_Resource ? Err::Success : Err::Failure;
    }

    void ClearPtr()
    {
      m_Resource.reset();
    }

    void SetUUID(Resource::UUID const& iUUID)
    {
      ClearPtr();
      m_ResourceUUID = iUUID;
    }

    Err Stream(Streamer& iStreamer) const
    {
      return iStreamer.Write(&m_ResourceUUID);
    }

    Err Unstream(Unstreamer& iStreamer)
    {
      Resource::UUID id;
      Err res = iStreamer.Read(&id);
      if (res)
      {
        SetUUID(id);
      }
      return res;
    }

    bool operator ==(ResourceHandle<T> const& iOther) const
    {
      return GetUUID() == iOther.GetUUID();
    }

    bool operator !=(ResourceHandle<T> const& iOther) const
    {
      return GetUUID() != iOther.GetUUID();
    }

  protected:
    Resource::UUID m_ResourceUUID;
    mutable IntrusivePtr<T const> m_Resource;
  };

  inline std::size_t hash_value(Resource::UUID const& Id) 
  {
    return hash_value(Id.uuid);
  }

  template <typename T>
  inline std::size_t hash_value(ResourceHandle<T> const& Handle)
  {
    return hash_value(Handle.GetUUID());
  }
}