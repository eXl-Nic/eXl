/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/containers.hpp>
#include <core/heapobject.hpp>
#include <core/idgenerator.hpp>

namespace eXl
{

  namespace ObjectConstants
  {
    const uint32_t s_GenerationSize = 6;
    const uint32_t s_GenerationShift = 32 - s_GenerationSize;
    const uint32_t s_GenerationMask = ((1<<s_GenerationSize) - 1) << s_GenerationShift;
    const uint32_t s_IdMask = ~(s_GenerationMask);
    const uint32_t s_InvalidId = s_IdMask;
    // Loop generation when reaching max. Also mark never used slots.
    const uint32_t s_MaxGeneration = ((1<<s_GenerationSize) - 1) << s_GenerationShift;

    uint32_t GetGeneration(uint32_t iId);

    uint32_t IncreaseGeneration(uint32_t iGen);

    uint32_t ComputeNextHandle(uint32_t iCurHandle, uint32_t iId);

    uint32_t ReleaseHandle(uint32_t iCurHandle);
  };

  class ObjectTableHandle_Base
  {
  public:
    ObjectTableHandle_Base()
      : m_IdAndGen(ObjectConstants::s_InvalidId) {}

    bool IsAssigned() const { return GetId() != ObjectConstants::s_InvalidId; }

    uint32_t GetId() const { return m_IdAndGen & ObjectConstants::s_IdMask; }
    uint32_t GetGeneration() const { return m_IdAndGen & ObjectConstants::s_GenerationMask; }

    bool operator== (ObjectTableHandle_Base const& iOther) const { return m_IdAndGen == iOther.m_IdAndGen; }
    bool operator!= (ObjectTableHandle_Base const& iOther) const { return m_IdAndGen != iOther.m_IdAndGen; }
    bool operator < (ObjectTableHandle_Base const& iOther) const { return m_IdAndGen <  iOther.m_IdAndGen; }

  protected:
    friend struct ObjectTable_Data;

    ObjectTableHandle_Base(uint32_t iId) : m_IdAndGen(iId) {}

    uint32_t m_IdAndGen;
  };

  template <typename T>
  class ObjectTable;

  template <typename T>
  struct ObjectTableHandle : public ObjectTableHandle_Base
  {
  public:
    ObjectTableHandle() {}
    explicit ObjectTableHandle(ObjectTableHandle_Base iGenericHandle) : ObjectTableHandle_Base(iGenericHandle) {}

  protected:
    ObjectTableHandle(uint32_t iId) : ObjectTableHandle_Base(iId) {}

    friend ObjectTable<T>;
    using ObjectTableHandle_Base::m_IdAndGen;
  };

  struct EXL_ENGINE_API ObjectTable_Data
  {
    static constexpr uint32_t s_PageSize = 1024;

    struct Page : public HeapObject
    {
      void Init(size_t iObjectSize, size_t iAlignment);

      ~Page();

      uint32_t* m_Generation;
      void* m_Objects;

      void* m_Buffer;
      uint32_t m_Used;
      uint32_t m_MaxId;
    };

    ObjectTable_Data(size_t iObjectSize, size_t iAlignment);

    void* Alloc(ObjectTableHandle_Base& oHandle);

    void* TryGet(ObjectTableHandle_Base iHandle) const;
#ifdef _DEBUG
    void* Get(ObjectTableHandle_Base iHandle) const;
#else
    void* Get(ObjectTableHandle_Base iHandle) const
    {
      uint32_t globId = iHandle.GetId();
      Page& page = m_Pages[globId / s_PageSize];
      uint32_t locId = globId % s_PageSize;
      return reinterpret_cast<char*>(page.m_Objects) + locId * m_ObjectOffset;
    }
#endif

    void Release(ObjectTableHandle_Base iHandle, void(*deleter)(void*));

    bool IsValid(ObjectTableHandle_Base iHandle) const;

    void Reset(void(*deleter)(void*));

    Page* m_Pages = nullptr;
    size_t m_ObjectSize;
    size_t m_Alignment;
    size_t m_ObjectOffset;

    uint32_t m_TotObject = 0;
    uint32_t m_NumPages = 0;
    IdGenerator m_Ids;
  };

  template <typename T>
  class ObjectTable
  {
  public:

    using Handle = ObjectTableHandle<T>;

    ObjectTable();

    ~ObjectTable();

    Handle Alloc();

    T* TryGet(Handle iHandle) const;

    //PREREQ : IsValid(iHandle)
    T& Get(Handle iHandle) const;

    void Release(Handle iHandle);

    bool IsValid(Handle iHandle) const;

    void Reset();

    template <typename Functor>
    inline void Iterate(Functor&& iFun) const;

    ObjectTable_Data& GetImplementation() { return m_Data; }

  private:
    static void DeleteObj(void* iObj)
    {
      reinterpret_cast<T*>(iObj)->~T();
    }

    ObjectTable_Data m_Data;
  };

  namespace ObjectConstants
  {
    inline uint32_t GetGeneration(uint32_t iId)
    {
      return iId >> ObjectConstants::s_GenerationShift;
    }

    inline uint32_t IncreaseGeneration(uint32_t iGen)
    {
      ++iGen;
      if(iGen >= ObjectConstants::s_MaxGeneration)
      {
        iGen = 0;
      }
      return iGen;
    }

    inline uint32_t ComputeNextHandle(uint32_t iCurHandle, uint32_t iId)
    {
      return (GetGeneration(iCurHandle) << ObjectConstants::s_GenerationShift) | iId;
    }

    inline uint32_t ReleaseHandle(uint32_t iCurHandle)
    {
      return (IncreaseGeneration(GetGeneration(iCurHandle)) << ObjectConstants::s_GenerationShift) | ObjectConstants::s_InvalidId;
    }
  }


  template<typename T>
  ObjectTable<T>::ObjectTable()
    : m_Data(sizeof(T), alignof(T))
  {

  }

  template<typename T>
  ObjectTable<T>::~ObjectTable()
  {
    Reset();
  }

  template<typename T>
  typename ObjectTable<T>::Handle ObjectTable<T>::Alloc()
  {
    Handle outHandle;
    void* dataBuffer = m_Data.Alloc(*reinterpret_cast<ObjectTableHandle_Base*>(&outHandle));

    new(dataBuffer) T;

    return outHandle;
  }

  template<typename T>
  T* ObjectTable<T>::TryGet(Handle iHandle) const
  {
    return reinterpret_cast<T*>(m_Data.TryGet(iHandle));
  }

  template<typename T>
  T& ObjectTable<T>::Get(Handle iHandle) const
  {
    return *reinterpret_cast<T*>(m_Data.Get(iHandle));
  }

  template<typename T>
  void ObjectTable<T>::Release(Handle iHandle)
  {
    m_Data.Release(iHandle, &ObjectTable<T>::DeleteObj);
  }

  template<typename T>
  bool ObjectTable<T>::IsValid(Handle iHandle) const
  {
    return m_Data.IsValid(iHandle);
  }

  template<typename T>
  void ObjectTable<T>::Reset()
  {
    m_Data.Reset(&ObjectTable<T>::DeleteObj);
  }

  template<typename T>
  template <typename Functor>
  inline void ObjectTable<T>::Iterate(Functor&& iFun) const
  {
    for (uint32_t pageNum = 0; pageNum < m_Data.m_NumPages; ++pageNum)
    {
      ObjectTable_Data::Page& page = m_Data.m_Pages[pageNum];
      for (uint32_t obj = 0; obj < page.m_MaxId; ++obj)
      {
        uint32_t handle = page.m_Generation[obj];
        if ((handle & ObjectConstants::s_IdMask) != ObjectConstants::s_InvalidId)
        {
          iFun(reinterpret_cast<T*>(page.m_Objects)[obj], Handle(handle));
        }
      }
    }
  }

  inline std::size_t hash_value(eXl::ObjectTableHandle_Base val)
  {
    return val.GetId();
  }

  template <typename T>
  inline std::size_t hash_value(typename eXl::ObjectTable<T>::Handle val)
  {
    return val.GetId();
  }
}