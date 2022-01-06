/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>

namespace eXl
{
  template <class T>
  inline void IntrusivePtrAddRef(T const* Obj){Obj->AddRef();}

  template <class T>
  inline void IntrusivePtrRelease(T const* Obj){Obj->Release();}

  template <class T>
  class IntrusivePtr
  {
  public:

    typedef T element_type;

    IntrusivePtr(): m_Ptr( 0 )
    {
    }

    IntrusivePtr(std::nullptr_t) : m_Ptr(0)
    {
    }

    explicit IntrusivePtr( T * p, bool add_ref = true )
      : m_Ptr( p )
    {
      if (m_Ptr != nullptr && add_ref)
      {
        IntrusivePtrAddRef(m_Ptr);
      }
    }

    template <class U>
    IntrusivePtr( IntrusivePtr<U> const & iOther )
    : m_Ptr( 0 )
    {
      T* ptr = iOther.get();
      if( ptr != 0 )
      {
        m_Ptr = ptr;
        IntrusivePtrAddRef( m_Ptr );
      }
    }

    IntrusivePtr(IntrusivePtr const & iOther): m_Ptr( iOther.m_Ptr )
    {
      if( m_Ptr != 0 )
        IntrusivePtrAddRef( m_Ptr );
    }

    IntrusivePtr(IntrusivePtr&& iOther): m_Ptr( iOther.m_Ptr )
    {
      iOther.m_Ptr = nullptr;
    }

    ~IntrusivePtr()
    {
      if( m_Ptr != 0 ) 
        IntrusivePtrRelease( m_Ptr );
    }

    template<class U> 
    IntrusivePtr & operator=(IntrusivePtr<U> const & iOther)
    {
      T* ptr = iOther.get();
      set(ptr);
      return *this;
    }

    IntrusivePtr & operator=(IntrusivePtr const & iOther)
    {
      set(iOther.m_Ptr);
      return *this;
    }

    IntrusivePtr & operator=(IntrusivePtr&& iOther)
    {
      m_Ptr = iOther.m_Ptr;
      iOther.m_Ptr = nullptr;
      return *this;
    }

    IntrusivePtr & operator=(T * iOther)
    {
      set(iOther);
      return *this;
    }

    bool operator==(IntrusivePtr const & iOther)const
    {
      return iOther.m_Ptr == m_Ptr;
    }

    bool operator==(T * iOther)const
    {
      return iOther == m_Ptr;
    }

    bool operator!=(IntrusivePtr const & iOther)const
    {
      return iOther.m_Ptr != m_Ptr;
    }

    bool operator!=(T * iOther)const
    {
      return iOther != m_Ptr;
    }

    bool operator<(IntrusivePtr const & iOther)const
    {
      return iOther.m_Ptr < m_Ptr;
    }

    bool operator<(T * iOther)const
    {
      return iOther < m_Ptr;
    }

    explicit operator bool() const
    {
      return m_Ptr != nullptr;
    }

    void reset()
    {
      set((T*)(0));
    }

    T * get() const
    {
      return m_Ptr;
    }

    inline T & operator*() const;

    inline T * operator->() const;

private:

    void set(T* iPtr)
    {
      if(iPtr != 0)
        IntrusivePtrAddRef(iPtr);
      if(m_Ptr != 0)
        IntrusivePtrRelease(m_Ptr);
      m_Ptr = iPtr;
    }

    T * m_Ptr;
  };

  template <typename T, typename... Args>
  IntrusivePtr<T> MakeRefCounted(Args&&... iArgs)
  {
    return IntrusivePtr<T>(eXl_NEW T(std::forward<Args>(iArgs)...));
  }


  template <typename T>
  inline size_t hash_value(IntrusivePtr<T> const& iPtr)
  {
    return (ptrdiff_t)(iPtr.get());
  }
}

#include "coredef.hpp"

namespace eXl
{
  template <class T>
  inline T & IntrusivePtr<T>::operator*() const
  {
    eXl_ASSERT( m_Ptr != 0);
    return *m_Ptr;
  }

  template <class T>
  inline T * IntrusivePtr<T>::operator->() const
  {
    eXl_ASSERT( m_Ptr != 0);
    return m_Ptr;
  }
}
