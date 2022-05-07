/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/world.hpp>
#include <engine/common/data_tables/dense.hpp>
#include <engine/common/data_tables/sparse.hpp>

namespace eXl
{
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
      return m_Alloc.GarbageCollect(m_Alloc.m_View.GetWorld());
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
      if (IsSparseAlloc<Allocator>::value)
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

    GameDataView<T>& GetView()
    {
      return m_Alloc.m_View;
    }

  protected:
    Allocator m_Alloc;
  };

  template <typename T>
  using DenseGameDataStorage = GameDataStorage<T, DensePropertySheetAllocator<T>>;

  template <typename T>
  using SparseGameDataStorage = GameDataStorage<T, SparsePropertySheetAllocator<T>>;

}