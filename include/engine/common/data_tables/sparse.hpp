/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/data_tables/base.hpp>

namespace eXl
{
  // Sparse -> ObjectTable data is released, and can even be shared.
  // A copy-on modify pattern is implemented in order to share data from archetypes.
  struct EXL_ENGINE_API SparseGameDataAllocator : public GameDataAllocatorBase
  {
    SparseGameDataAllocator(ObjectDataIndex& iIndex, Type const* iType, ObjectTable_Data& iObjects);
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

    ObjectTable_Data& m_ObjectData;
  };

  template <typename T>
  class SparseGameDataView : public GameDataView<T>
  {
  public:
    SparseGameDataView(World& iWorld, SparseGameDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec);

    T const* Get(ObjectHandle iObject) const override;
    T* Get(ObjectHandle iObject) override;
    T const* GetDataForDeletion(ObjectHandle iObject) override;
    T& GetOrCreate(ObjectHandle iObject) override;
    void Erase(ObjectHandle iObject) override;
    const SparseGameDataAllocator& GetAlloc() const override;
    SparseGameDataView<T>* GetSparseView() override { return this; }

    template <typename Functor>
    inline void Iterate(Functor const& iFn);
    template <typename Functor>
    inline void Iterate(Functor const& iFn) const;

  protected:
    SparseGameDataAllocator& m_Alloc;
    ObjectTable<T>& m_ObjectSpec;
  };

  template <typename T>
  struct SparsePropertySheetAllocator : SparseGameDataAllocator
  {
    SparsePropertySheetAllocator(World& iWorld, Type const* iType);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    void Clear() override;
    ObjectDataIndex m_Index;
    ObjectTable<T> m_ObjectsSpec;
    SparseGameDataView<T> m_View;
  };

  template<typename T>
  struct IsSparseAlloc<SparsePropertySheetAllocator<T>>
  {
    static constexpr bool value = true;
  };

#include "sparse.inl"
}