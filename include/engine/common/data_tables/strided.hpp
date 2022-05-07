/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/data_tables/dense.hpp>

namespace eXl
{
  template <typename T>
  class StridedGameDataView : public GameDataView<T>
  {
  public:
    StridedGameDataView(World& iWorld, DenseDataAllocator const& iAlloc, size_t iOffset)
      : GameDataView(iWorld)
      , m_Alloc(iAlloc)
      , m_Offset(iOffset)
    {}

    template <typename Functor>
    void Iterate(Functor const& iFn) const
    {
      const_cast<StridedGameDataView<T>*>(this)->Iterate([&iFn](ObjectHandle iObject, T& iData)
        {
          iFn(iObject, const_cast<T const&>(iData));
        });
    }

    template <typename Functor>
    void Iterate(Functor const& iFn)
    {
      for (uint32_t pageNum = 0; pageNum < m_Alloc.m_ObjectData.m_NumPages; ++pageNum)
      {
        ObjectTable_Data::Page& page = m_Alloc.m_ObjectData.m_Pages[pageNum];
        for (uint32_t obj = 0; obj < page.m_MaxId; ++obj)
        {
          uint32_t handle = page.m_Generation[obj];
          ObjectHandle object = m_Alloc.m_WorldObjects[obj];
          if (this->m_World.IsObjectValid(object)
            && (handle & ObjectConstants::s_IdMask) != ObjectConstants::s_InvalidId)
          {
            uint8_t* ptr = (((uint8_t*)page.m_Objects) + m_Alloc.m_ObjectData.m_ObjectSize) + m_Offset;
            iFun(object, *reinterpret_cast<T*>(ptr));
          }
        }
      }
    }

  protected:
    DenseDataAllocator const& m_Alloc;
    size_t m_Offset;
  };




  template <typename T, typename Field, Field T::* ObjectField>
  class StridedGameDataViewAdaptor : public StridedGameDataView<Field>
  {
  public:
    StridedGameDataViewAdaptor(DenseGameDataView<T>& iView)
      : StridedGameDataView(iView.GetWorld(), iView.GetAlloc(), CastPointerToMember())
      , m_View(iView)
    {

    }

    static size_t CastPointerToMember()
    {
      union
      {
        size_t res;
        Field T::* ptr;
      } obj;

      obj.ptr = ObjectField;
      return obj.res;
    }

    Field const* Get(ObjectHandle iObject) const override
    {
      T* const obj = m_View.Get(iObject);
      return obj != nullptr ? &(obj->*ObjectField) : nullptr;
    }
    Field* Get(ObjectHandle iObject) override
    {
      T* obj = m_View.Get(iObject);
      return obj != nullptr ? &(obj->*ObjectField) : nullptr;
    }
    Field const* GetDataForDeletion(ObjectHandle iObject) override
    {
      T const* obj = m_View.GetDataForDeletion(iObject);
      return obj != nullptr ? &(obj->*ObjectField) : nullptr;
    }
    Field& GetOrCreate(ObjectHandle iObject) override
    {
      return m_View.GetOrCreate(iObject).*ObjectField;
    }
    void Erase(ObjectHandle iObject) override
    {
      m_View.Erase(iObject);
    }
    const DataAllocatorBase& GetAlloc() const override
    {
      return m_View.GetAlloc();
    }
  protected:
    DenseGameDataView<T>& m_View;
  };

  template <typename T>
  StridedGameDataView<T> const* GameDataView<T>::GetStridedView() const
  {
    return const_cast<GameDataView<T>*>(this)->GetStridedView();
  }

#include "strided.inl"
}