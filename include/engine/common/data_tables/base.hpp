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
  class Type;

  struct EXL_ENGINE_API ObjectDataIndex
  {
    UnorderedMap<ObjectHandle, uint32_t> m_ObjectToSlot;
    Vector<ObjectHandle> m_WorldObjects;
  };

  struct EXL_ENGINE_API GameDataAllocatorBase
  {
    GameDataAllocatorBase(ObjectDataIndex& iIndex, Type const* iType);
    virtual ~GameDataAllocatorBase() = default;

    virtual ObjectTableHandle_Base Alloc() = 0;
    virtual void Release(ObjectTableHandle_Base) = 0;

    virtual void GarbageCollect(World& iWorld) = 0;
    virtual void Clear();

    virtual uint32_t AllocateSlot(ObjectHandle iHandle) = 0;
    inline uint32_t GetSlot(ObjectHandle) const;
    virtual void EraseSlot(uint32_t) = 0;
    virtual ObjectTableHandle_Base GetDataFromSlot(uint32_t) = 0;
    virtual ObjectTableHandle_Base GetDataFromSlot(uint32_t) const = 0;

    ObjectDataIndex& m_IndexRef;
    Type const* m_Type;
    void* m_ViewPtr = nullptr;
  };

  template <typename T>
  class DenseGameDataView;

  template <typename T>
  class SparseGameDataView;

  template <typename T>
  class StridedGameDataView;

  template <typename T>
  class GameDataView
  {
  public:
    GameDataView(World& iWorld)
      : m_World(iWorld)
    {}

    bool HasEntry(ObjectHandle iObject) const
    {
      return Get(iObject) != nullptr;
    }
    T const* GetConst(ObjectHandle iObject) const
    {
      return Get(iObject);
    }
    virtual T const* Get(ObjectHandle iObject) const = 0;
    virtual T* Get(ObjectHandle iObject) = 0;
    virtual T const* GetDataForDeletion(ObjectHandle iObject) = 0;
    virtual T& GetOrCreate(ObjectHandle iObject) = 0;
    virtual void Erase(ObjectHandle iObject) = 0;
    virtual const GameDataAllocatorBase& GetAlloc() const = 0;
    virtual DenseGameDataView<T>* GetDenseView() { return nullptr; }
    virtual SparseGameDataView<T>* GetSparseView() { return nullptr; }
    virtual StridedGameDataView<T>* GetStridedView() { return nullptr; }
    DenseGameDataView<T> const* GetDenseView() const;
    SparseGameDataView<T> const* GetSparseView() const;
    StridedGameDataView<T> const* GetStridedView() const;

    World& GetWorld() const { return m_World; }

    template <typename Functor>
    void Iterate(Functor const& iFn);
    template <typename Functor>
    void Iterate(Functor const& iFn) const;

  protected:
    World& m_World;
  };

  template<typename Alloc>
  struct IsSparseAlloc
  {
    static constexpr bool value = false;
  };

#include "base.inl"
}