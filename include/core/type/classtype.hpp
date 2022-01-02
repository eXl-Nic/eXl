/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/type.hpp>

namespace eXl
{
  class Rtti;
  class Type;
  class TupleType;

  class EXL_CORE_API ClassType : public Type
  {
  public:
    typedef ClassType TheRttiClass;
    typedef RttiObject ParentRttiClass;
    static Rtti const& StaticRtti();
    static Type const* GetType();
    const Rtti& GetRtti() const override;
    static ClassType* DynamicCast(::eXl::RttiObject* ptr);
    static const ClassType* DynamicCast(const ::eXl::RttiObject* ptr);
        
  public:

    ClassType(TypeName const& iName, Rtti const& iClassRtti, ClassType const* iParentRttiClass);

    ~ClassType();

    inline Rtti const& GetClassRtti() const {return m_ClassRtti;}
    inline ClassType const* GetParentRttiClass() const {return m_ParentRttiClass;}

  protected:
    Rtti const&      m_ClassRtti;
    ClassType const* m_ParentRttiClass;
    TupleType const* m_StreamingStruct;
  };
}

