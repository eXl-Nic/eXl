/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef EXL_ALLOCATOR_INCLUDED
#define EXL_ALLOCATOR_INCLUDED

#include <core/memorymanager.hpp>
#include <limits>
#include <memory>
#include <type_traits>

#ifdef EXL_NAMESPACE_ALLOC_INJECTION
#undef EXL_ALLOCATOR_NAME
#define EXL_ALLOCATOR_NAME(AllocName) eXl##AllocName
#else
#undef EXL_ALLOCATOR_NAME
#define EXL_ALLOCATOR_NAME(AllocName) AllocName
#endif

#ifndef EXL_NAMESPACE_ALLOC_INJECTION
namespace eXl
{
#endif

  template<typename T>
  class EXL_ALLOCATOR_NAME(Allocator)
#ifndef EXL_SHARED_LIBRARY
    : public std::allocator<T>
#endif
  {
  public:
    //    typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    typedef std::integral_constant<bool, true> propagate_on_container_move_assignment;
    typedef std::integral_constant<bool, true> is_always_equal;

  public:
    //    convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind
    {
      typedef EXL_ALLOCATOR_NAME(Allocator) <U> other;
    };

  public:
    inline EXL_ALLOCATOR_NAME(Allocator)() {}
    inline ~EXL_ALLOCATOR_NAME(Allocator)() {}
    inline EXL_ALLOCATOR_NAME(Allocator)(EXL_ALLOCATOR_NAME(Allocator) const&) {}
    template<typename U>
    inline EXL_ALLOCATOR_NAME(Allocator)(EXL_ALLOCATOR_NAME(Allocator) <U> const&) {}

    //    address
    inline pointer address(value_type& r) { return &r; }
    inline const_pointer address(value_type const& r) { return &r; }

    //    memory allocation
    inline pointer allocate(size_type cnt, const_pointer ptr = 0)
    {
#ifdef EXL_SHARED_LIBRARY
      return reinterpret_cast<pointer>(::eXl::MemoryManager::Allocate(sizeof(T) * cnt));
#else
      return std::allocator_traits<std::allocator<T>>::allocate(*this, cnt, ptr);
#endif
    }

    inline void deallocate(pointer p, size_type bytes)
    {
#ifdef EXL_SHARED_LIBRARY
      ::eXl::MemoryManager::Free(p, false);
#else
      return std::allocator_traits<std::allocator<T>>::deallocate(*this, p, bytes);
#endif
    }

    //    size
    inline size_type max_size() const {
      return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    //    construction/destruction
    template <typename... U>
    inline void construct(pointer p, U&&... u) { new(p) T(std::forward<U>(u)...); }
    inline void destroy(pointer p) { p->~T(); }

    friend void swap(EXL_ALLOCATOR_NAME(Allocator) &, EXL_ALLOCATOR_NAME(Allocator) &)
    {}
    friend bool operator==(const EXL_ALLOCATOR_NAME(Allocator) &, const EXL_ALLOCATOR_NAME(Allocator) &)
    {
      return true;
    }
    friend bool operator!=(const EXL_ALLOCATOR_NAME(Allocator) &, const EXL_ALLOCATOR_NAME(Allocator) &)
    {
      return false;
    }

  };    //    end of class Allocator 

  template<>
  class EXL_ALLOCATOR_NAME(Allocator) <void>
  {
  public:
    //    typedefs
    typedef void value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;

    typedef std::integral_constant<bool, true> propagate_on_container_move_assignment;
    typedef std::integral_constant<bool, true> is_always_equal;

  public:
    //    convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind
    {
      typedef EXL_ALLOCATOR_NAME(Allocator) <U> other;
    };

  public:
    inline EXL_ALLOCATOR_NAME(Allocator)() {}
    inline ~EXL_ALLOCATOR_NAME(Allocator)() {}
    inline EXL_ALLOCATOR_NAME(Allocator)(EXL_ALLOCATOR_NAME(Allocator) const&) {}
    template<typename U>
    inline EXL_ALLOCATOR_NAME(Allocator)(EXL_ALLOCATOR_NAME(Allocator) <U> const&) {}

    friend void swap(EXL_ALLOCATOR_NAME(Allocator) <void> &, EXL_ALLOCATOR_NAME(Allocator) <void> &)
    {}
    friend bool operator==(const EXL_ALLOCATOR_NAME(Allocator) <void> &, const EXL_ALLOCATOR_NAME(Allocator) <void> &)
    {
      return true;
    }
    friend bool operator!=(const EXL_ALLOCATOR_NAME(Allocator) <void> &, const EXL_ALLOCATOR_NAME(Allocator) <void> &)
    {
      return false;
    }
  };    //    end of class Allocator 
#ifndef EXL_NAMESPACE_ALLOC_INJECTION
}
#endif
  
#if 0
namespace eXl
{
  // Forgetful pool. Just keep allcoating more memory until Reset is called.
  // Use it for small-medium algorithm than need to run on a large number of objects.
  class EXL_CORE_API MemoryPool
  {
  public:

    MemoryPool(size_t iPageSize);
    ~MemoryPool();

    void* Alloc(size_t iSize);

    // Drop all pages usage to 0, retain memory.
    void Reset();

  protected:

    MemoryPool(MemoryPool const&) = delete;
    MemoryPool& operator=(MemoryPool const&) = delete;

    struct Page
    {
      char* m_Memory;
      size_t m_PageSize;
      uint32_t m_CurUsage;
    };

    Page* AllocPage(size_t iSize);

    Page* m_Pages;
    size_t m_NumPages;
    size_t m_FullPages;
    size_t m_PageCapacity;
    const size_t m_PageSize;
  };

  template<typename T>
  class PooledAllocator 
  {
  public : 
    //    typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

  public : 
    //    convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind 
    {
      typedef PooledAllocator<U> other;
    };

  public : 
    inline PooledAllocator(MemoryPool& iPool)
      : m_Pool(iPool)
    {}

    inline ~PooledAllocator() {}

    inline PooledAllocator(PooledAllocator const& iOther)
      : m_Pool(iOther.m_Pool)
    {}

    template<typename U>
    inline PooledAllocator(PooledAllocator<U> const& iOther) 
      : m_Pool(iOther.m_Pool)
    {}

    //    address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    //    memory allocation
    inline pointer allocate(size_type cnt, 
      typename std::allocator<void>::const_pointer = 0) 
    {
      return reinterpret_cast<pointer>(m_Pool.Alloc(sizeof (T) * cnt)); 
    }

    inline void deallocate(pointer p, size_type) 
    { 
      
    }

    //    size
    inline size_type max_size() const { 
      return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    //    construction/destruction
    inline void construct(pointer p, T&& t) { new(p) T(std::move(t)); }
    inline void construct(pointer p, const T& t) { new(p) T(t); }
    inline void destroy(pointer p) { p->~T(); }

    template<typename U>
    inline bool operator==(Allocator<U> const&) const { return true; }

    template<typename U>
    inline bool operator!=(Allocator<U> const& a) const { return !operator==(a); }

    MemoryPool& m_Pool;
  };
}
#endif

#endif