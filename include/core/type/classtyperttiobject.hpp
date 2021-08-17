/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <core/type/classtype.hpp>
#include <core/coredef.hpp>
#include <core/rtti.hpp>

namespace eXl
{
#if 0
  namespace TypeManager
  {
    class ClassTypeRegImpl;
    EXL_CORE_API Err RegisterTransientClassForRtti(Rtti const& iRtti);
    namespace detail
    {
      EXL_CORE_API ClassType const* EndClassReg(ClassTypeRegImpl* iRegImpl);
    }
  }

  class ClassTypeRttiObject : public ClassType
  {
    friend class TypeManager::ClassTypeRegImpl;
    friend Err TypeManager::RegisterTransientClassForRtti(Rtti const& iRtti);
    friend ClassType const* TypeManager::detail::EndClassReg(TypeManager::ClassTypeRegImpl* iRegImpl);
    DECLARE_RTTI(ClassTypeRttiObject, ClassType);
  public:

    bool IsAbstract() const;

    virtual Err Build(void*& ioObj, void const* iDesc) const;

    virtual Err RetrieveDesc_Uninit(void const* iObj, void* oDesc) const;

    virtual void* Copy(void const* iObject) const;

    virtual void Destroy(void* iObj) const;

    inline ClassTypeRttiObject const* GetParentRttiClass() const 
    {return reinterpret_cast<ClassTypeRttiObject const*>(m_ParentRttiClass);}

  protected:

    void ComputeStreamingStruct();

    typedef RttiObject* (*ObjectFactory)();

    ClassTypeRttiObject(TypeName const& iName, Rtti const& iClassRtti, ClassType const* iParentRttiClass, ObjectFactory iFactory);

    ObjectFactory m_Factory;
    unsigned int m_Offset;
  };
#endif
}
