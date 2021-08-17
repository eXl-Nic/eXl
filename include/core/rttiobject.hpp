#pragma once

#include <core/corelibexp.hpp>
#include <core/heapobject.hpp>

namespace eXl
{
  class Type;
  class Rtti;
  class EXL_CORE_API RttiObject : public HeapObject
  {
  public:
    typedef RttiObject TheRttiClass;
    typedef RttiObject TheRttiParentClass;
    virtual const Rtti& GetRtti() const;
    static Type const* GetType();

    static inline const RttiObject* DynamicCast(const RttiObject* ptr)
    {
      if (ptr == nullptr) return nullptr;
      return ptr;
    }
    static inline RttiObject* DynamicCast(RttiObject* ptr)
    {
      if (ptr == nullptr) return nullptr;
      return ptr;
    }
    static const Rtti& StaticRtti();

    virtual ~RttiObject();
  protected:
    RttiObject();
  };

  //typedef IntrusivePtr<RttiObjectRefC> RttiOPtr;
}