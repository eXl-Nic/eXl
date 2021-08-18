/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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