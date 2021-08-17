/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/heapobject.hpp>
#include <core/corelibexp.hpp>
#include <core/type/classtype.hpp>


namespace eXl
{
  class ClassType;
  class Streamer;
  class Unstreamer;
  /**
     Object that have Runtime type identification.
     Handles single-inheritance class trees and allows DynamicCast
  **********************************************************************/
  
  class EXL_CORE_API Rtti
  {
  public:
    Rtti(KString name, const Rtti* father);

    bool IsKindOf(const Rtti& Type) const;

    inline const KString& GetName()const{return ClassName;}

    inline const Rtti* GetFather() const {return fatherRtti;}

    //inline unsigned int GetId()const{return id;}

    //inline unsigned int GetDepth()const{return depth;}

    inline bool operator ==(const Rtti& rtti)const {return &rtti==this;}

    inline bool operator !=(const Rtti& rtti)const {return &rtti!=this;}

    //const Rtti* CommonAncestor(const Rtti& rtti)const;

    //void InitRtti();
    
    //static std::vector<Rtti*>& GetRttiList(){return RttiList;}

  private:
    Rtti(const Rtti&);
    Rtti& operator=(const Rtti&);
    //unsigned int ComputeDepth() const;
    
    KString ClassName;
    const Rtti* fatherRtti;
  };

#ifdef NO_RTTI_CHECK

#define DYNAMIC_CAST(a) DynamicCast(a)

#define DECLARE_RTTI(ClassName,TheParentRttiClass)                         \
  public:                                                           \
  typedef ClassName TheRttiClass;                                       \
  typedef TheParentRttiClass ParentRttiClass;                               \
  const Rtti& GetRtti() const override{return TheRttiClass::StaticRtti();}   \
  static inline const TheRttiClass* DynamicCast(const RttiObject* ptr){	\
    if(ptr==nullptr) return nullptr;                                      \
    return (TheRttiClass*)ptr;                                          \
  }                                                                 \
  static inline TheRttiClass* DynamicCast(RttiObject* ptr){             \
    if(ptr==nullptr) return nullptr;                                      \
    return (TheRttiClass*)ptr;                                          \
  }                                                                 \
  static const Rtti& StaticRtti();                                  \
  static ::eXl::Type const* GetType();
  
#else

#ifdef RTTI_THROW

#define DYNAMIC_CAST(a) DynamicCast(a,__FILE__,__LINE__)

#define DECLARE_RTTI(ClassName,TheParentRttiClass)                             \
  public:                                                               \
  typedef ClassName TheRttiClass;                                           \
  typedef TheParentRttiClass ParentRttiClass;                                   \
  const ::eXl::Rtti& GetRtti() const override{return TheRttiClass::StaticRtti();}        \
  static inline const TheRttiClass* DynamicCast(const ::eXl::RttiObject* ptr){     \
    if(ptr==nullptr || !ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti()))        \
      throw eXl::Exception(eXl::Exception::BadCast,String("Bad cast from")+ ptr->GetRtti().GetName() +" to "+String(#ClassName)); \
    return ((const TheRttiClass*)(ptr));                                    \
  }                                                                     \
  static inline const TheRttiClass* DynamicCast(const ::eXl::RttiObject* ptr,const char* iFile,unsigned int iLine){ \
    if(ptr==nullptr || !ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti()))        \
      throw eXl::Exception(eXl::Exception::BadCast,String("Bad cast from")+ ptr->GetRtti().GetName() +" to "+String(#ClassName),iFile,iLine); \
    return ((const TheRttiClass*)(ptr));                                    \
  }                                                                     \
  static inline TheRttiClass* DynamicCast(::eXl::RttiObject* ptr){                 \
    if(ptr==nullptr || !ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti()))        \
      throw eXl::Exception(eXl::Exception::BadCast,String("Bad cast from")+ ptr->GetRtti().GetName() +" to "+String(#ClassName)); \
    return ((TheRttiClass*)(ptr));                                          \
  }                                                                     \
  static inline TheRttiClass* DynamicCast(::eXl::RttiObject* ptr,const char* iFile,unsigned int iLine){ \
    if(ptr==nullptr || !ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti()))        \
      throw eXl::Exception(eXl::Exception::BadCast,String("Bad cast from")+ ptr->GetRtti().GetName() +" to "+String(#ClassName),iFile,iLine); \
    return ((TheRttiClass*)(ptr));                                          \
  }                                                                     \
  static ::eXl::Rtti const& StaticRtti();                               \
  static ::eXl::Type const* GetType();
  
#else
  
#define DYNAMIC_CAST(a) DynamicCast(a)
  
#define DECLARE_RTTI(ClassName,TheParentRttiClass)                          \
  public:                                                               \
  typedef ClassName TheRttiClass;                                           \
  typedef TheParentRttiClass ParentRttiClass;                                   \
  const ::eXl::Rtti& GetRtti() const override{return TheRttiClass::StaticRtti();} \
  static inline TheRttiClass* DynamicCast(::eXl::RttiObject* ptr){          \
    if(ptr==nullptr) return nullptr;                                          \
    return (ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti())?(TheRttiClass*)(ptr):nullptr); \
  }                                                                     \
  static inline const TheRttiClass* DynamicCast(const ::eXl::RttiObject* ptr){     \
    if(ptr==nullptr) return nullptr;                                          \
    return (ptr->GetRtti().IsKindOf(TheRttiClass::StaticRtti())?(const TheRttiClass*)(ptr):nullptr); \
  }                                                                     \
  static ::eXl::Rtti const& StaticRtti(); \
  static ::eXl::Type const* GetType();
  
#endif
#endif

#define DECLARE_RTTI_OBJECT(ClassName,ParentRttiClass)          \
  public:                                                   \
  static RttiObject* CreateInstance();                      \
  virtual ClassName* Duplicate() const;                     \
  DECLARE_RTTI(ClassName,ParentRttiClass)
  /*static ClassName* CopyInstance(ClassName* iOther);        \*/
  
  

  
#define IMPLEMENT_RTTI(ClassName)            \
  const ::eXl::Rtti& ClassName::StaticRtti() \
  { \
    static ::eXl::Rtti s_Rtti(#ClassName,&ClassName::ParentRttiClass::StaticRtti()); \
    return s_Rtti; \
  } \
  ::eXl::Type const* ClassName::GetType() \
  { \
    static ::eXl::ClassType s_Type(#ClassName, ClassName::StaticRtti(), ::eXl::ClassType::DynamicCast(ClassName::ParentRttiClass::GetType())); \
    return &s_Type; \
  }


#define IMPLEMENT_RTTI_OBJECT(ClassName)                    \
  IMPLEMENT_RTTI(ClassName)                                 \
  RttiObject* ClassName::CreateInstance()                   \
  {                                                         \
    return eXl_NEW ClassName;                               \
  }                                                         \
  ClassName* ClassName::Duplicate() const                   \
  {                                                         \
    return eXl_NEW ClassName(*this);                        \
  }
  /*ClassName* ClassName::CopyInstance(ClassName* iOther)   \
  {                                                         \
    if(iOther)                                              \
      return iOther->Duplicate;                             \
    else                                                    \
      return nullptr;                                          \
  }                                                         \
                                                            \
                                                            */
}
