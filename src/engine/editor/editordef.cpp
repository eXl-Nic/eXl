#include "editordef.hpp"
#include <QLayout>
#include <QVBoxLayout>
#include <QImage>
#include <map>

#include <core/type/dynobject.hpp>
#include <core/type/tupletype.hpp>
#include <core/type/arraytype.hpp>
#include <core/image/image.hpp>

namespace eXl
{
  void DynObjToQVariant(ConstDynObject const& iObj, QVariant& oVariant)
  {
    Type const* objType = iObj.GetType();

    if(objType == TypeManager::GetType<unsigned int>())
    {
      oVariant = QVariant(*iObj.CastBuffer<unsigned int>());
    }
    else if(objType == TypeManager::GetType<int>())
    {
      oVariant = QVariant(*iObj.CastBuffer<int>());
    }
    else if(objType == TypeManager::GetType<unsigned char>() )
    {
      oVariant = QVariant(unsigned int(*iObj.CastBuffer<unsigned char>()));
    }
    else if(objType == TypeManager::GetType<float>())
    {
      oVariant = QVariant(*iObj.CastBuffer<float>());
    }
    else if(objType == TypeManager::GetType<String>())
    {
      oVariant = QString(iObj.CastBuffer<String>()->c_str());
    }
    else if (objType == TypeManager::GetType<Name>())
    {
      oVariant = QString(iObj.CastBuffer<Name>()->c_str());
    }
    else if(objType == TypeManager::GetType<bool>())
    {
      oVariant = QVariant(*iObj.CastBuffer<bool>());
    }
  }

  void DynObjFromQVariant(DynObject& oObj, QVariant const& iVariant)
  {
    Type const* objType = oObj.GetType();
    size_t typeId = objType->GetTypeId();

    if(objType == TypeManager::GetType<unsigned int>())
    {
      unsigned int localValue = iVariant.toUInt();
      TypeTraits::Copy<unsigned int>(oObj.GetBuffer(),&localValue);
    }
    else if(objType == TypeManager::GetType<int>())
    {
      int localValue = iVariant.toInt();
      TypeTraits::Copy<int>(oObj.GetBuffer(),&localValue);
    }
    else if(objType == TypeManager::GetType<unsigned char>() )
    {
      unsigned char localValue = iVariant.toUInt();
      TypeTraits::Copy<unsigned char>(oObj.GetBuffer(),&localValue);
    }
    else if(objType == TypeManager::GetType<float>())
    {
      float localValue = iVariant.toFloat();
      TypeTraits::Copy<float>(oObj.GetBuffer(),&localValue);
    }
    else if(objType == TypeManager::GetType<String>())
    {
      String localValue = iVariant.toString().toUtf8();
      TypeTraits::Copy<String>(oObj.GetBuffer(),&localValue);
    }
    else if (objType == TypeManager::GetType<Name>())
    {
      Name localValue(iVariant.toString().toUtf8());
      TypeTraits::Copy<Name>(oObj.GetBuffer(), &localValue);
    }
    else if(objType == TypeManager::GetType<bool>())
    {
      bool localValue = iVariant.toBool();
      TypeTraits::Copy<bool>(oObj.GetBuffer(),&localValue);
    }
    else
    {
      DynObject objCopy(iVariant.value<DynObject>());
      //eXl_ASSERT(dataType == objCopy.GetType(),"");

      objType->Assign(objCopy.GetType(), objCopy.GetBuffer(), oObj.GetBuffer());
    }
  }

  Err eXlImageToQImage(Image const& iImage, QImage& oImage)
  {
    if (iImage.GetFormat() == Image::Float)
    {
      return Err::Failure;
    }

    QImage::Format imageFormat;

    Optional<Image> converted;

    switch (iImage.GetComponents())
    {
    case Image::R:
      switch (iImage.GetFormat())
      {
      case Image::Char:
        imageFormat = QImage::Format_Grayscale8;
        break;
      case Image::Short:
        imageFormat = QImage::Format_Grayscale16;
        break;
      case Image::Int:
        return Err::Failure;
        break;
      }

    case Image::RG:
      return Err::Failure;
      break;

    case Image::RGB:
      switch (iImage.GetFormat())
      {
      case Image::Char:
        imageFormat = QImage::Format_RGB888;
        break;
      case Image::Short:
        imageFormat = QImage::Format_RGB16;
        break;
      case Image::Int:
        imageFormat = QImage::Format_RGB32;
        break;
      }
      break;
    case Image::RGBA:
      switch (iImage.GetFormat())
      {
      case Image::Char:
        imageFormat = QImage::Format_RGBA8888;
        break;
      case Image::Short:
        imageFormat = QImage::Format_RGBA64;
        break;
      case Image::Int:
        return Err::Failure;
        break;
      }
      break;
    case Image::BGR:
      switch (iImage.GetFormat())
      {
      case Image::Char:
        imageFormat = QImage::Format_BGR888;
        break;
      case Image::Short:
        return Err::Failure;
        break;
      case Image::Int:
        return Err::Failure;
        break;
      }
      break;
    case Image::BGRA:
      switch (iImage.GetFormat())
      {
      case Image::Char:
        imageFormat = QImage::Format_RGBA8888;
        converted.emplace(Image(nullptr, iImage.GetSize(), Image::RGBA, Image::Char, 1));
        for (uint32_t row = 0; row < iImage.GetSize().Y(); ++row)
        {
          char const* rowPtr = (char const*)iImage.GetRow(row);
          char* destRowPtr = (char*)converted->GetRow(row);
          for (uint32_t pixel = 0; pixel < iImage.GetSize().X(); ++pixel)
          {
            destRowPtr[0] = rowPtr[2];
            destRowPtr[1] = rowPtr[1];
            destRowPtr[2] = rowPtr[0];
            destRowPtr[3] = rowPtr[3];
            rowPtr += iImage.GetPixelSize();
            destRowPtr += iImage.GetPixelSize();
          }
        }
        break;
      case Image::Short:
        return Err::Failure;
        break;
      case Image::Int:
        return Err::Failure;
        break;
      }
      break;
    default:
      return Err::Failure;
      break;
    }

    Image const& imageSource = converted ? *converted : iImage;

    oImage = QImage((uchar*)imageSource.GetImageData(), imageSource.GetSize().X(), imageSource.GetSize().Y(), imageSource.GetRowStride(), imageFormat).copy();

    return Err::Success;
  }
}