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
  // Dense -> ObjectTable data is never released.
  // Each object table entry is associated to a unique world object.
  struct EXL_ENGINE_API DenseGameDataAllocator : public GameDataAllocatorBase
  {
    DenseGameDataAllocator(ObjectDataIndex& iIndex, Type const* iType);

    void GarbageCollect(World& iWorld) override;
    void Clear() override;

    uint32_t AllocateSlot(ObjectHandle iHandle) override;

    inline uint32_t AllocateSlot_Inl(ObjectHandle iHandle);
    void EraseSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) override;
    ObjectTableHandle_Base GetDataFromSlot(uint32_t) const override;

    ObjectTableHandle_Base GetDataFromSlot_Inl(uint32_t);
    ObjectTableHandle_Base GetDataFromSlot_Inl(uint32_t) const;
    virtual ObjectTable_Data* GetObjectTable() { return nullptr; }
    ObjectTable_Data const* GetObjectTable() const
    { return const_cast<DenseGameDataAllocator*>(this)->GetObjectTable(); }
  };

  template <typename T>
  class DenseGameDataView : public GameDataView<T>
  {
  public:
    DenseGameDataView(World& iWorld, DenseGameDataAllocator& iAlloc, ObjectTable<T>& iObjectsSpec);

    T const* Get(ObjectHandle iObject) const override;
    T* Get(ObjectHandle iObject) override;
    T const* GetDataForDeletion(ObjectHandle iObject) override;
    T& GetOrCreate(ObjectHandle iObject) override;
    void Erase(ObjectHandle iObject) override;
    const DenseGameDataAllocator& GetAlloc() const override;
    DenseGameDataView<T>* GetDenseView() override { return this; }

    template <typename Functor>
    inline void Iterate(Functor const& iFn);
    template <typename Functor>
    inline void Iterate(Functor const& iFn) const;

  protected:
    DenseGameDataAllocator& m_Alloc;
    ObjectTable<T>& m_ObjectSpec;
  };

  template <typename T>
  struct T_DensePropertySheetAllocator : DenseGameDataAllocator
  {
    T_DensePropertySheetAllocator(World& iWorld, ObjectDataIndex& iIndex, Type const* iType, DenseGameDataView<T>* iViewPtr);
    ObjectTableHandle_Base Alloc() override;
    void Release(ObjectTableHandle_Base iHandle) override;
    void Clear() override;
    ObjectTable_Data* GetObjectTable() override { return &m_ObjectsSpec.GetImplementation(); }
    ObjectTable<T> m_ObjectsSpec;
  };

  template <typename T>
  struct DensePropertySheetAllocator : T_DensePropertySheetAllocator<T>
  {
    DensePropertySheetAllocator(World& iWorld, Type const* iType)
      : T_DensePropertySheetAllocator<T>(iWorld, m_Index, iType, &m_View)
      , m_View(iWorld, *this, this->m_ObjectsSpec)
    {}
    ObjectDataIndex m_Index;
    DenseGameDataView<T> m_View;
  };


#include "dense.inl"
}