/**

  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WEAKREF_INCLUDED
#define WEAKREF_INCLUDED

#include <core/heapobject.hpp>
#include <core/refcobject.hpp>

#include <boost/type_traits.hpp>

#define IMPLEMENT_WEAKREFLINK_NOTIFY(ClassName) \
  void WeakRefLink##ClassName::AddRef() const   \
  {                                             \
    AtomicIncrement(m_RefCount);                \
  }                                             \
  void WeakRefLink##ClassName::Release() const  \
  {                                             \
    AtomicDecrement(m_RefCount);                \
    if(RefCIsOne())                             \
      m_Ptr->OnNullWeakRef();                   \
    else if(m_RefCount == 0)                    \
      eXl_DELETE this;                          \
  }

#define ADD_WEAKREF(ClassName, ExportDecl)                         \
public:                                                            \
  class ExportDecl WeakRefLinkType : public WeakRefLink<ClassName> \
  {                                                                \
    DECLARE_RefC;                                                  \
  public:                                                          \
    inline WeakRefLinkType(ClassName const* iPtr)                  \
      :m_Ptr(const_cast<ClassName*>(iPtr))                         \
      {}                                                           \
                                                                   \
    inline bool RefCIsOne() const                                  \
    {                                                              \
      if(m_RefCount == 1)                                          \
      {                                                            \
          return true;                                             \
      }                                                            \
      return false;                                                \
    }                                                              \
                                                                   \
    inline void Reset(ClassName* iObj = nullptr)                   \
    {                                                              \
      m_Ptr = iObj;                                                \
    }                                                              \
    static void Set(WeakRefLinkType*& iPtr, ClassName const* iObject); \
                                                                   \
    inline ClassName* Get(){return m_Ptr;}                         \
  protected:                                                       \
    ClassName* m_Ptr;                                              \
  };                                                               \
  void InvalidateWeakRefs();                                       \
private:                                                           \
  WeakRefLinkType* GetWeakRef() const;                             \
  mutable struct WeakRefHolder                                     \
  {                                                                \
    ~WeakRefHolder();                                              \
    mutable IntrusivePtr<WeakRefLinkType> m_LinkPtr;               \
  } m_Holder
 

#define IMPLEMENT_WEAKREF(ClassName)                             \
IMPLEMENT_RefC(ClassName::WeakRefLinkType)                       \
void ClassName::WeakRefLinkType::Set(ClassName::WeakRefLinkType*& iPtr, ClassName const* iObject) \
{                                                                \
  if(iObject != NULL)                                            \
    iObject->GetWeakRef()->AddRef();                             \
  if(iPtr != NULL)                                               \
    iPtr->Release();                                             \
  if(iObject != NULL)                                            \
    iPtr = iObject->GetWeakRef();                                \
  else                                                           \
    iPtr = NULL;                                                 \
}                                                                \
ClassName::WeakRefHolder::~WeakRefHolder()              \
{                                                       \
  if(m_LinkPtr != nullptr)                              \
  {                                                     \
    m_LinkPtr->Reset(nullptr);                          \
  }                                                     \
}                                                       \
ClassName::WeakRefLinkType* ClassName::GetWeakRef() const          \
{                                                       \
  if(m_Holder.m_LinkPtr == nullptr)                     \
  {                                                     \
    m_Holder.m_LinkPtr = eXl_NEW WeakRefLinkType(this); \
  }                                                     \
  return m_Holder.m_LinkPtr.get();                      \
}                                                       \
void ClassName::InvalidateWeakRefs()                    \
{                                                       \
  m_Holder.m_LinkPtr->Reset();                          \
  m_Holder.m_LinkPtr->Release();                        \
  m_Holder.m_LinkPtr= nullptr;                          \
}

namespace eXl
{
  template <class T>
  class Handle;

  template<class T>
  class WeakRefLink : public HeapObject
  {
  };

  template <class T>
  size_t hash_value(Handle<T> const&);

  template <class T>
  class Handle
  {
    template <class U>
    friend size_t hash_value(Handle<U> const&);
  public:

    Handle();
    
    Handle(T* iObj);
  
    Handle(Handle&& r);

    Handle(Handle const& r);
    
    template<class Y> 
    Handle(Handle<Y> const& r);
    
    Handle& operator=(Handle const& r);
    
    template<class Y>
    Handle& operator=(Handle<Y> const& r);

    Handle& operator=(T*);

    ~Handle();

    void Reset();

    void ResetNoRelease();

    void SetNoAddRef(void* iPtr);

    void Set(T*);

    T* Get()const;

    T* GetNoRelease()const;

    bool operator==(const Handle& iHandle)const;

    bool operator<(const Handle& iHandle)const;
  private:
    typedef WeakRefLink< typename boost::remove_const<T>::type > WeakRefType;
    mutable WeakRefType* m_LinkPtr;
  };

  template <class T>
  inline size_t hash_value(Handle<T> const& iHandle)
  {
    return (ptrdiff_t)iHandle.m_LinkPtr;
  }

#include "handle.inl"
}

#endif