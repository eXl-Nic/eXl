/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/world.hpp>

namespace eXl
{
  MAKE_NAME(PropertySheetName);

  struct EXL_ENGINE_API DataAllocatorBase
  {
    DataAllocatorBase(Type const* iType, ObjectTable_Data& iObjects);
    virtual ~DataAllocatorBase() = default;

    virtual ObjectTableHandle_Base Alloc() = 0;
    virtual void Release(ObjectTableHandle_Base) = 0;

    virtual void GarbageCollect(World& iWorld) = 0;
    virtual void Clear();

    virtual uint32_t AllocateSlot(ObjectHandle iHandle) = 0;
    inline uint32_t GetSlot(ObjectHandle) const;
    virtual void EraseSlot(uint32_t) = 0;
    virtual ObjectTableHandle_Base GetDataFromSlot(uint32_t) = 0;
    virtual ObjectTableHandle_Base GetDataFromSlot(uint32_t) const = 0;

    UnorderedMap<ObjectHandle, uint32_t> m_ObjectToSlot;
    Vector<ObjectHandle> m_WorldObjects;
    Type const* m_Type;
    ObjectTable_Data& m_ObjectData;
    void* m_ViewPtr = nullptr;
  };

  // Dense -> ObjectTable data is never released.
  // Iterating over the object table data is done in the same order as the world objects.
  struct EXL_ENGINE_API DenseDataAllocator : public DataAllocatorBase
  {
    DenseDataAllocator(Type const* iType, ObjectTable_Data& iObjects);

    void GarbageCollect(World& iWorld) override;
    void Clear() override;

    uint32_t AllocateSlot(ObjectHandle iHandle) override;

    inline uint32_t AllocateSlot_Inl(ObjectHandle iHandle);
    void EraseSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) const override;

    ObjectTableHandle_Base GetDataFromSlot_Inl(uint32_t);
    ObjectTableHandle_Base GetDataFromSlot_Inl(uint32_t) const;
  };

  // Sparse -> ObjectTable data is released, and can even be shared.
  struct EXL_ENGINE_API SparseDataAllocator : public DataAllocatorBase
  {
    SparseDataAllocator(Type const* iType, ObjectTable_Data& iObjects);
    void GarbageCollect(World& iWorld) override;
    void Clear() override;

    uint32_t AllocateSlot(ObjectHandle iHandle) override;

    inline uint32_t AllocateSlot_Inl(ObjectHandle iHandle);
    void EraseSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) const override;

    ObjectTableHandle_Base GetDataFromSlot_Inl(uint32_t) const;

    Vector<ObjectTableHandle_Base> m_ObjectHandles;
    Vector<ObjectTableHandle_Base> m_ArchetypeHandle;
  };

  template <typename T>
  class DenseGameDataView;

  template <typename T>
  class SparseGameDataView;

  template <typename T>
  class GameDataView
  {
  public:
    GameDataView(World& iWorld, ObjectTable<T>& iObjectsSpec)
      : m_World(iWorld)
      , m_ObjectSpec(iObjectsSpec)
    {}

    virtual T const* Get(ObjectHandle iObject) const = 0;
    virtual T* Get(ObjectHandle iObject) = 0;
    virtual T const* GetDataForDeletion(ObjectHandle iObject) = 0;
    virtual T& GetOrCreate(ObjectHandle iObject) = 0;
    virtual void Erase(ObjectHandle iObject) = 0;
    virtual const DataAllocatorBase& GetAlloc() const = 0;
    virtual DenseGameDataView<T>* GetDenseView() { return nullptr; }
    virtual SparseGameDataView<T>* GetSparseView() { return nullptr; }
    DenseGameDataView<T> const* GetDenseView() const;
    SparseGameDataView<T> const* GetSparseView() const;

    template <typename Functor>
    void Iterate(Functor const& iFn);
    template <typename Functor>
    void Iterate(Functor const& iFn) const;

  protected:
    World& m_World;
    ObjectTable<T>& m_ObjectSpec;
  };

  template <typename T>
  class DenseGameDataView : public GameDataView<T>
  {
  public:
    DenseGameDataView(World& iWorld, DenseDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec);

    T const* Get(ObjectHandle iObject) const override;
    T* Get(ObjectHandle iObject) override;
    T const* GetDataForDeletion(ObjectHandle iObject) override;
    T& GetOrCreate(ObjectHandle iObject) override;
    void Erase(ObjectHandle iObject) override;
    const DataAllocatorBase& GetAlloc() const override;
    DenseGameDataView<T>* GetDenseView() override { return this; }

    template <typename Functor>
    inline void Iterate(Functor const& iFn);
    template <typename Functor>
    inline void Iterate(Functor const& iFn) const;

  protected:
    DenseDataAllocator& m_Alloc;
  };

  template <typename T>
  class SparseGameDataView : public GameDataView<T>
  {
  public:
    SparseGameDataView(World& iWorld, SparseDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec);

    T const* Get(ObjectHandle iObject) const override;
    T* Get(ObjectHandle iObject) override;
    T const* GetDataForDeletion(ObjectHandle iObject) override;
    T& GetOrCreate(ObjectHandle iObject) override;
    void Erase(ObjectHandle iObject) override;
    const DataAllocatorBase& GetAlloc() const override;
    SparseGameDataView<T>* GetSparseView() override { return this; }

    template <typename Functor>
    inline void Iterate(Functor const& iFn);
    template <typename Functor>
    inline void Iterate(Functor const& iFn) const;

  protected:
    SparseDataAllocator& m_Alloc;
  };

  template<typename Alloc>
  struct IsSparseAlloc 
  {
    static constexpr bool value = false;
  };

  template <typename T>
  struct DensePropertySheetAllocator : DenseDataAllocator
  {
    DensePropertySheetAllocator(World& iWorld, Type const* iType);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    void Clear() override;
    ObjectTable<T> m_ObjectsSpec;
    DenseGameDataView<T> m_View;
  };

  template <typename T>
  struct SparsePropertySheetAllocator : SparseDataAllocator
  {
    SparsePropertySheetAllocator(World& iWorld, Type const* iType);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    void Clear() override;
    ObjectTable<T> m_ObjectsSpec;
    SparseGameDataView<T> m_View;
  };

  template<typename T>
  struct IsSparseAlloc<SparsePropertySheetAllocator<T>>
  {
    static constexpr bool value = true;
  };
  
  template <typename T, typename Allocator>
  class GameDataStorage
  {
  public:
    GameDataStorage(World& iWorld)
      : m_Alloc(iWorld, nullptr)
    {}

    T const* Get(ObjectHandle iObject) const
    {
      return m_Alloc.m_View.Get(iObject);
    }
    T* Get(ObjectHandle iObject)
    {
      return m_Alloc.m_View.Get(iObject);
    }
    T const* GetDataForDeletion(ObjectHandle iObject)
    {
      return m_Alloc.m_View.GetDataForDeletion(iObject);
    }
    T& GetOrCreate(ObjectHandle iObject)
    {
      return m_Alloc.m_View.GetOrCreate(iObject);
    }
    void Erase(ObjectHandle iObject)
    {
      return m_Alloc.m_View.Erase(iObject);
    }
    void GarbageCollect()
    {
      return m_Alloc.GarbageCollect();
    }

    template <typename Functor>
    inline void Iterate(Functor const& iFn)
    {
      if (IsSparseAlloc<Allocator>::value)
      {
        reinterpret_cast<SparseGameDataView<T>&>(m_Alloc.m_View).SparseGameDataView<T>::Iterate(iFn);
      }
      else
      {
        reinterpret_cast<DenseGameDataView<T>&>(m_Alloc.m_View).DenseGameDataView<T>::Iterate(iFn);
      }
    }

    template <typename Functor>
    inline void Iterate(Functor const& iFn) const
    {
      if (IsSparseAlloc<Alloc>::value)
      {
        static_cast<SparseGameDataView<T> const&>(m_Alloc.m_View).SparseGameDataView<T>::Iterate(iFn);
      }
      else
      {
        static_cast<DenseGameDataView<T> const&>(m_Alloc.m_View).DenseGameDataView<T>::Iterate(iFn);
      }
    }

    void Clear()
    {
      m_Alloc.Clear();
    }

  protected:
    Allocator m_Alloc;
  };

  template <typename T>
  using DenseGameDataStorage = GameDataStorage<T, DensePropertySheetAllocator<T>>;

  template <typename T>
  using SparseGameDataStorage = GameDataStorage<T, SparsePropertySheetAllocator<T>>;

#include "gamedata.inl"
}