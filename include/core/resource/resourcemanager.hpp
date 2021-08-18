/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/resource/resource.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/path.hpp>

#include <core/stream/textreader.hpp>
#include <functional>

namespace eXl
{
  namespace ResourceManager
  {
    using TextFileReadFactory = std::function<std::unique_ptr<TextReader>(char const* iPath)>;

    EXL_CORE_API void SetTextFileReadFactory(TextFileReadFactory);

#ifdef EXL_RSC_HAS_FILESYSTEM
    template <typename T>
    T* Load(Path const& iPath)
    {
      return static_cast<T*>(LoadExpectedType(iPath, T::StaticLoaderName()));
    }
#endif
    template <typename T>
    T* Load(Resource::UUID const& iUUID)
    {
      return static_cast<T*>(LoadExpectedType(iUUID, T::StaticLoaderName()));
    }
    
    EXL_CORE_API void AddManifest(RttiObject const& iManifest);
    EXL_CORE_API void RemoveManifest(RttiObject const& iManifest);
    EXL_CORE_API void RemoveManifest(Rtti const& iManifest);

    EXL_CORE_API void PopulateManifests(Stream_Base& iStreamer);

    EXL_CORE_API String const& GetAssetExtension();

    EXL_CORE_API ResourceLoaderName GetLoaderFromRtti(Rtti const& iRtti);
    EXL_CORE_API Type const* GetHandleType(Rtti const& iRtti);
    EXL_CORE_API Type const* GetHandleType(ResourceLoaderName iLoaderName);
    EXL_CORE_API void AddLoader(ResourceLoader* iLoader, Rtti const& iRtti);
    EXL_CORE_API Vector<ResourceLoaderName> ListLoaders();
    EXL_CORE_API ResourceLoader* GetLoader(ResourceLoaderName iLoaderName);

    EXL_CORE_API Resource* LoadExpectedType(Resource::UUID const& iUUID, const ResourceLoaderName& iExpectedLoader);
    EXL_CORE_API Resource* Load(Resource::UUID const& iUUID, ResourceLoaderName* oLoader = nullptr);

    EXL_CORE_API Vector<Resource::Header> ListResources();
    EXL_CORE_API Resource::Header const* GetHeader(Resource::UUID const& iUUID);
    EXL_CORE_API bool HasFile(Resource::UUID const& iUUID);

    EXL_CORE_API void UnloadUnusedResources();
    EXL_CORE_API void Reset();

    EXL_CORE_API void BootstrapAssetsFromManifest(String const& iDir);

#ifdef EXL_RSC_HAS_FILESYSTEM
    EXL_CORE_API void BootstrapDirectory(Path const& iPath, bool iRecursive);
    EXL_CORE_API Resource* LoadExpectedType(Path const& iPath, const ResourceLoaderName& iExpectedLoader);
    EXL_CORE_API Resource* Load(const char* iPath, ResourceLoaderName* oLoader = nullptr);
    EXL_CORE_API Path GetPath(Resource::UUID const& iUUID);
    EXL_CORE_API Err SetPath(Resource* iRsc, Path const& iPath);
    EXL_CORE_API Err SaveTo(Resource* iRsc, Path const& iPath);
    EXL_CORE_API Err Save(Resource* iRsc);
    EXL_CORE_API void Bake(Path const& iDest);
#endif
  }
}