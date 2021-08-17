#pragma once

#include <QVariant>
#include <core/resource/resource.hpp>
#include <core/type/dynobject.hpp>


namespace eXl
{
  class Resource;
  class DynObject;
  class ConstDynObject;
  class Rtti;
  class Type;
  class Image;
}
class QLayout;
class QImage;
class DynObjectEditor;
class ResourceEditor;

struct ArrayItem
{
  uint32_t index;
};

Q_DECLARE_METATYPE(eXl::Resource::UUID);
Q_DECLARE_METATYPE(eXl::ResourceLoaderName);
Q_DECLARE_METATYPE(eXl::ConstDynObject*);
Q_DECLARE_METATYPE(eXl::ConstDynObject const*);
Q_DECLARE_METATYPE(eXl::DynObject);
Q_DECLARE_METATYPE(ArrayItem);

namespace eXl
{
  void DynObjToQVariant(ConstDynObject const& iObj, QVariant& oVariant);
  void DynObjFromQVariant(DynObject& iObj, QVariant const& oVariant);

  Err eXlImageToQImage(Image const& iImage, QImage& oImage);
}

