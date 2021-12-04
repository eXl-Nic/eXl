/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/gfx/tileset.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/image/imagestreamer.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <core/log.hpp>

#include <core/type/typemanager.hpp>
#include <core/type/enumtype.hpp>
#include <core/type/arraytype.hpp>
#include <core/type/tupletypestruct.hpp>

#include <core/stream/jsonstreamer.hpp>
#include <fstream>

#include "../dummysprites.hpp"

#define BUFFERSIZE 256
#include <b64/encode.h>
#include <b64/decode.h>
#undef BUFFERSIZE

namespace eXl
{
  IMPLEMENT_RTTI(Tileset)

  Err Tile::Stream(Streamer& iStreamer) const
  {
    return GetType()->Stream(this, &iStreamer);
  }

  Err Tile::Unstream(Unstreamer& iStreamer)
  {
    void* readBuffer = this;
    return GetType()->Unstream(readBuffer, &iStreamer);
  }

  class TilesetLoader : public ResourceLoader
  {

  public:

    static TilesetLoader& Get()
    {
      static TilesetLoader s_This;
      return s_This;
    }

    TilesetLoader() 
      : ResourceLoader(Tileset::StaticLoaderName(), 1)
    {
      
    }

    static Type const* GetTileType()
    {
      static TupleType const* s_Type = []
      {
        //Type const* animEnum = TypeManager::BeginEnumTypeRegistration("AnimationType")
        //  .AddValue("None")
        //  .AddValue("Loop")
        //  .AddValue("Pingpong")
        //  .EndRegistration();
        return TypeManager::BeginNativeTypeRegistration<Tile>("Tile")
          .AddField("ImageName", &Tile::m_ImageName)
          .AddField("FrameDuration", &Tile::m_FrameDuration)
          .AddField("Size", &Tile::m_Size)
          //.AddCustomField("AnimationType", &Tile::m_AnimType, animEnum)
          .AddField("AnimationType", &Tile::m_AnimType)
          .AddCustomField("Frames", &Tile::m_Frames, TypeManager::GetArrayType<Vector2i>())
          .AddField("Offset", &Tile::m_Offset)
          .AddField("Scale", &Tile::m_Scale)
          .EndRegistration();
      }();

      return s_Type;
    }

#ifndef EXL_IS_BAKED_PLATFORM
    Tileset* Create(Path const& iDir, String const& iName) const
    {
      ResourceMetaData* metaData = CreateNewMetaData(iName);

      Path rscPath = iDir / Path(iName.c_str());
      rscPath.replace_extension(ResourceManager::GetAssetExtension().c_str());

      Tileset* newTileset = eXl_NEW Tileset(*metaData);
      newTileset->SetFlags(Resource::LockPathDirectory);
      if (ResourceManager::SetPath(newTileset, rscPath))
      {
        return newTileset;
      }

      return nullptr;
    }
#endif

    Tileset* Create_Impl(ResourceMetaData* iMetaData) const override
    {
      Tileset* tileset = eXl_NEW Tileset(*iMetaData);

      return tileset;
    }

    bool NeedsBaking(Resource* iRsc) const override
    { 
      return true; 
    }

    Tileset* CreateBakedResource(Resource* iRsc) const override
    { 
      Tileset* tilesetToBake = Tileset::DynamicCast(iRsc);
      Tileset* bakedTileset = eXl_NEW Tileset(*CreateBakedMetaData(iRsc->GetMetaData()));

      bakedTileset->m_Tiles = tilesetToBake->m_Tiles;
      bakedTileset->PostLoad();

      return bakedTileset;
    }

  private:
    Type const* m_TileType;
  };

  void Tileset::Init()
  {
    ResourceManager::AddLoader(&TilesetLoader::Get(), Tileset::StaticRtti());
  }

  Type const* Tile::GetType()
  {
    return TilesetLoader::GetTileType();
  }

  void Tileset::PostLoad()
  {
#ifndef EXL_IS_BAKED_PLATFORM
    Path rscPath = ResourceManager::GetPath(GetHeader().m_ResourceId);
    Path rscDir = rscPath.parent_path();
    for (auto const& tile : m_Tiles)
    {
      ImageName imgName = tile.second.m_ImageName;
      if (imgName != Tile::EmptyName())
      {
        if (m_ImagePathCache.count(tile.second.m_ImageName) == 0)
        {
          Path imagePath = rscDir / Path(imgName.get().c_str());
          m_ImagePathCache.insert(std::make_pair(imgName, imagePath));
        }
      }
    }
#endif
  }

#ifndef EXL_IS_BAKED_PLATFORM

  Path Tileset::GetSrcImagePath(ImageName iImage) const
  {
    auto iterPath = m_ImagePathCache.find(iImage);
    if (iterPath == m_ImagePathCache.end())
    {
      return Path();
    }

    return iterPath->second;
  }

  boost::optional<ImageName> Tileset::ImageNameFromImagePath(Path const& iImagePath)
  {
    Path resourcePath = ResourceManager::GetPath(GetHeader().m_ResourceId);

    if (iImagePath.string() == Tile::EmptyName().get())
    {
      return {};
    }

    if (Filesystem::exists(iImagePath) && !Filesystem::is_directory(iImagePath))
    {
      Path containingDir = Filesystem::absolute(Filesystem::canonical(iImagePath.parent_path()));
      Path remainingPath = iImagePath.filename();
      Path resourceDir = resourcePath.parent_path();

      while (containingDir != resourceDir && containingDir.has_parent_path())
      {
        remainingPath = containingDir.filename() / remainingPath;
        containingDir = containingDir.parent_path();
      }

      if (containingDir == resourceDir)
      {
        return ImageName(remainingPath.string().c_str());
      }
    }

    return {};
  }

  Tileset* Tileset::Create(Path const& iDir, String const& iName)
  {
    return TilesetLoader::Get().Create(iDir, iName);
  }
#endif

  ResourceLoaderName Tileset::StaticLoaderName()
  {
    return ResourceLoaderName("Tileset");
  }

  Tileset::Tileset(ResourceMetaData& iMetaData)
    : Resource(iMetaData)
  {

  }
  Err Tileset::AddTile(TileName iName, Tile const& iTile)
  {
    if (iTile.m_ImageName != Tile::EmptyName())
    {
#ifndef EXL_IS_BAKED_PLATFORM
      if (m_ImagePathCache.count(iTile.m_ImageName) == 0)
      {
        Path rscPath = ResourceManager::GetPath(GetHeader().m_ResourceId);
        Path rscDir = rscPath.parent_path();

        Path imagePath = rscDir / Path(iTile.m_ImageName.get().c_str());
        if (!Filesystem::exists(imagePath) || Filesystem::is_directory(imagePath))
        {
          LOG_ERROR << "Could not find an image at " << imagePath.string() << "\n";
          return Err::Error;
        }

        m_ImagePathCache.insert(std::make_pair(iTile.m_ImageName, imagePath));
      }
#else
      if (m_Images.count(iTile.m_ImageName) == 0)
      {
        return Err::Error;
      }
#endif
    }

    m_Tiles.insert_or_assign(iName, iTile);

    return Err::Success;
  }


  void Tileset::RemoveTile(TileName iName)
  {
    m_Tiles.erase(iName);
  }

  Image const* Tileset::GetImage(ImageName iImage) const
  {
    if (iImage == Tile::EmptyName())
    {
      return nullptr;
    }

    auto iter = const_cast<Tileset*>(this)->m_Images.find(iImage);

		if(iter == m_Images.end() || iter->second == nullptr)
		{
#ifndef EXL_IS_BAKED_PLATFORM
      auto iterPath = m_ImagePathCache.find(iImage);
      if (iterPath == m_ImagePathCache.end())
      {
        LOG_WARNING << "Tried to use unknown image " << iImage.get() << " with tileset " << GetName() << "\n";
        return nullptr;
      }
#ifdef EXL_IMAGESTREAMER_ENABLED
			if (Image* newImg = ImageStreamer::Load(iterPath->second.string().c_str()))
			{
        iter = const_cast<Tileset*>(this)->m_Images.emplace(std::make_pair(iImage, std::unique_ptr<Image>())).first;
        iter->second.reset(newImg);
      }
			else
#endif
			{
				LOG_ERROR << "Could not load image at path " << iterPath->second.string() << "\n";
			}
#endif
		}

    if (iter == m_Images.end())
    {
      return nullptr;
    }
    return iter->second.get();
  }

  Vector2i Tileset::GetImageSize(ImageName iImage) const
  {
    if(Image const* image = GetImage(iImage))
    {
      return Vector2i(image->GetSize().X(), image->GetSize().Y());
    }

    return Vector2i::ZERO;
  }

#ifdef EXL_WITH_OGL
  IntrusivePtr<OGLTexture> Tileset::GetTexture(ImageName iImage) const
  {
    auto iter = m_Textures.find(iImage);

    if (iter == m_Textures.end())
    {
			Image const* img = GetImage(iImage);
      if (img == nullptr)
      {
        return IntrusivePtr<OGLTexture>();
      }

      //Image dummy = DummySprites::BitmapToImage(DummySprites::dummyChar, img->GetSize());
      //img = &dummy;
			if (OGLTexture* newTex = OGLTextureLoader::CreateFromImage(img, true))
			{
        LOG_INFO << "Created texture for image " << img << " of size (" << img->GetSize().X() << ", " << img->GetSize().Y() << ")\n";
				iter = const_cast<Tileset*>(this)->m_Textures.insert(std::make_pair(iImage, newTex)).first;
			}
			else
			{
				LOG_ERROR << "Could not create texture for image " << iImage.get() << "\n";
			}
		}

    if (iter == m_Textures.end())
    {
      return iter->second;
    }

    return iter->second;
  }
#endif

  Err Tileset::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<Tileset*>(this)->Serialize(Serializer(iStreamer));
  }

  Err Tileset::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  namespace
  {
    uint32_t NextPOT(uint32_t iValue)
    {
      uint32_t val = 1;
      while (val < iValue)
      {
        val *= 2;
      }

      return val;
    }
  }

  Err Tileset::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Tiles");
    iStreamer.HandleSequence(m_Tiles, 
      [](UnorderedMap<TileName, Tile>& oMap, Unstreamer& iStreamer)
    {
      String tempStr;
      Tile tempTile;
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.Read(&tempStr);
      iStreamer.PopKey();
      iStreamer.PushKey("TileData");
      iStreamer.Read(&tempTile);
      iStreamer.PopKey();
      iStreamer.EndStruct();

      oMap.insert(std::make_pair(TileName(tempStr), std::move(tempTile)));
    },
      [](UnorderedMap<TileName, Tile>::value_type const& iEntry, Streamer& iStreamer)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.Write(&iEntry.first);
      iStreamer.PopKey();
      iStreamer.PushKey("TileData");
      iStreamer.Write(&iEntry.second);
      iStreamer.PopKey();
      iStreamer.EndStruct();
    });
    iStreamer.PopKey();

    if (GetHeader().m_Flags & BakedResource)
    {
#ifndef EXL_IS_BAKED_PLATFORM
      if (iStreamer.IsWriting())
      {
        for (auto& entry : m_ImagePathCache)
        {
          m_Images.insert(std::make_pair(entry.first, nullptr));
        }
      }
#endif

      iStreamer.PushKey("Images");
      iStreamer.HandleSequence(m_Images,
        [this](UnorderedMap<ImageName, std::unique_ptr<Image>>& oMap, Unstreamer& iStreamer)
      {
        String tempStr;
        iStreamer.BeginStruct();
        iStreamer.PushKey("Name");
        iStreamer.Read(&tempStr);
        iStreamer.PopKey();

        iStreamer.PushKey("Data");

#ifdef RAW_IMAGE

        iStreamer.BeginStruct();

        iStreamer.PushKey("Size");
        Vector2i size;
        iStreamer.Read(&size);
        iStreamer.PopKey();

        int components;

        iStreamer.PushKey("Components");
        iStreamer.ReadInt(&components);
        iStreamer.PopKey();

        int format;

        iStreamer.PushKey("Format");
        iStreamer.ReadInt(&format);
        iStreamer.PopKey();

        iStreamer.PushKey("Pixels");

        String base64String;
        iStreamer.ReadString(&base64String);

        LOG_INFO << "Image : " << tempStr << "Data : " << base64String;

        Vector<char> imageData;
        imageData.reserve(base64String.size());

        base64::base64_decodestate state;
        base64::base64_init_decodestate(&state);
        
        size_t const decodeBufferSize = 256;
        char decodeBuffer[decodeBufferSize];

        char const* dataIter = base64String.data();
        char const* endIter = dataIter + base64String.size();
        size_t const inputSize = 128;
        do
        {
          size_t readSize = dataIter + inputSize < endIter ? inputSize : endIter - dataIter;
          size_t outSize = base64::base64_decode_block(dataIter, readSize, decodeBuffer, &state);

          imageData.insert(imageData.end(), decodeBuffer, decodeBuffer + outSize);

          dataIter += readSize;

        } while (dataIter != endIter);

        Image::Size imageSize(size.X(), size.Y());
        std::unique_ptr<Image> imagePtr;
        {
          imagePtr = std::make_unique<Image>(imageData.data(), imageSize, (Image::Components)components, (Image::Format)format, 4);
        }

        //std::unique_ptr<Image> imagePtr(ImageStreamer::Load((uint8_t*)imageData.data(), imageData.size()));
        //Image::Size imageSize = imagePtr ? imagePtr->GetSize() : Image::Size(0, 0);

        //if (imageSize.X() == 0 || imageSize.Y() == 0)
        //{
        //  LOG_ERROR << "Image " << tempStr << " zero size!!";
        //}

        oMap.insert(std::make_pair(ImageName(tempStr), std::move(imagePtr)));

        iStreamer.PopKey();
        iStreamer.EndStruct();
#else
        String base64String;
        iStreamer.ReadString(&base64String);

        Vector<char> imageData;
        imageData.reserve(base64String.size());

        base64::base64_decodestate state;
        base64::base64_init_decodestate(&state);

        size_t const decodeBufferSize = 256;
        char decodeBuffer[decodeBufferSize];

        char const* dataIter = base64String.data();
        char const* endIter = dataIter + base64String.size();
        size_t const inputSize = 128;
        do
        {
          size_t readSize = dataIter + inputSize < endIter ? inputSize : endIter - dataIter;
          size_t outSize = base64::base64_decode_block(dataIter, readSize, decodeBuffer, &state);

          imageData.insert(imageData.end(), decodeBuffer, decodeBuffer + outSize);

          dataIter += readSize;

        } while (dataIter != endIter);
#ifdef EXL_IMAGESTREAMER_ENABLED
        std::unique_ptr<Image> imagePtr(ImageStreamer::Load((uint8_t*)imageData.data(), imageData.size()));
        Image::Size imageSize = imagePtr ? imagePtr->GetSize() : Image::Size(0, 0);

        if (imageSize.X() == 0 || imageSize.Y() == 0)
        {
          LOG_ERROR << "Image " << tempStr << " zero size!!";
        }

        oMap.insert(std::make_pair(ImageName(tempStr), std::move(imagePtr)));
#endif
#endif
        iStreamer.PopKey();
        iStreamer.EndStruct();

      },
        [this](UnorderedMap<ImageName, std::unique_ptr<Image>>::value_type const& iEntry, Streamer& iStreamer)
      {
#ifndef EXL_IS_BAKED_PLATFORM

        iStreamer.BeginStruct();
        iStreamer.PushKey("Name");
        iStreamer.Write(&iEntry.first);
        iStreamer.PopKey();

        iStreamer.PushKey("Data");

#ifdef RAW_IMAGE

        iStreamer.BeginStruct();

        Image const* image = GetImage(iEntry.first);

        iStreamer.PushKey("Size");
        Vector2i size(image->GetSize().X(), image->GetSize().Y());
        iStreamer.Write(&size);
        iStreamer.PopKey();

        int components = image->GetComponents();

        iStreamer.PushKey("Components");
        iStreamer.WriteInt(&components);
        iStreamer.PopKey();

        int format = image->GetFormat();

        iStreamer.PushKey("Format");
        iStreamer.WriteInt(&format);
        iStreamer.PopKey();

        iStreamer.PushKey("Pixels");

        base64::base64_encodestate state;
        base64::base64_init_encodestate(&state);
        
        Vector<char> tempStr;

        char const* pixelsData = (char const*)image->GetImageData();

        size_t const inputBufferSize = 128;
        size_t const encodeBufferSize = 256;
        char encodeBuffer[encodeBufferSize];

        for (int i = 0; i < size.Y(); ++i)
        {
          char const* pixelRow = pixelsData + i * image->GetRowStride();

          size_t rowSizeInBytes = size.X() * image->GetPixelSize();
          uint32_t numWrites = rowSizeInBytes / inputBufferSize;
          numWrites += rowSizeInBytes % inputBufferSize == 0 ? 0 : 1;
          for(uint32_t block = 0; block < numWrites; ++block)
          {
            uint32_t readSize = block == numWrites - 1 && rowSizeInBytes % inputBufferSize != 0
              ? rowSizeInBytes % inputBufferSize
              : inputBufferSize;

            size_t encodeSize = base64::base64_encode_block(pixelRow + block * inputBufferSize, readSize, encodeBuffer, &state);
            tempStr.insert(tempStr.end(), encodeBuffer, encodeBuffer + encodeSize);
          }
        }
        size_t encodeSize = base64::base64_encode_blockend(encodeBuffer, &state);
        tempStr.insert(tempStr.end(), encodeBuffer, encodeBuffer + encodeSize);

        tempStr.push_back(0);
        iStreamer.WriteString(tempStr.data());
        iStreamer.PopKey();
        iStreamer.EndStruct();
#endif
        Path imgPath = GetSrcImagePath(iEntry.first);

        std::ifstream fileStream(imgPath.c_str(), std::ios::binary);
        if (fileStream.is_open())
        {
          size_t const inputBufferSize = 128;
          char inputBuffer[inputBufferSize];
          size_t const encodeBufferSize = 256;
          char encodeBuffer[encodeBufferSize];

          base64::base64_encodestate state;
          base64::base64_init_encodestate(&state);

          Vector<char> tempStr;

          fileStream.read(inputBuffer, inputBufferSize);
          size_t readSize = fileStream.gcount();
          while (readSize > 0)
          {
            size_t encodeSize = base64::base64_encode_block(inputBuffer, readSize, encodeBuffer, &state);
            tempStr.insert(tempStr.end(), encodeBuffer, encodeBuffer + encodeSize);

            fileStream.read(inputBuffer, inputBufferSize);
            readSize = fileStream.gcount();
          }
          size_t encodeSize = base64::base64_encode_blockend(encodeBuffer, &state);
          tempStr.insert(tempStr.end(), encodeBuffer, encodeBuffer + encodeSize);

          tempStr.push_back(0);
          iStreamer.WriteString(tempStr.data());
        }

        iStreamer.PopKey();
        iStreamer.EndStruct();
#endif
      });
      iStreamer.PopKey();
    }
    iStreamer.EndStruct();

    return Err::Success;
  }

  //Err Tileset::Unstream_Data(Unstreamer& iStreamer)
  //{
  //  String tempStr;
  //  iStreamer.BeginStruct();
  //
  //  auto tileType = TilesetLoader::Get().GetTileType();
  //
  //  iStreamer.PushKey("Tiles");
  //  if (iStreamer.BeginSequence())
  //  {
  //    do
  //    {
  //
  //
  //    } while (iStreamer.NextSequenceElement());
  //  }
  //  iStreamer.PopKey();
  //
  //  iStreamer.EndStruct();
  //
  //  return Err::Success;
  //}

  uint32_t Tileset::ComputeHash()
  {
    return 0;
  }
}