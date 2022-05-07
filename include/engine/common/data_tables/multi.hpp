/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <engine/common/data_tables/dense.hpp>
#include <core/utils/positional.hpp>

namespace eXl
{
  template <typename T, uint32_t Index>
  struct MultiPropertySheetWrapper : T_DensePropertySheetAllocator<T>
  {
    static constexpr uint32_t s_Index = Index;
    MultiPropertySheetWrapper(World& iWorld, ObjectDataIndex& iIndex, DenseGameDataAllocator& iAlloc)
      : T_DensePropertySheetAllocator(iWorld, iIndex, nullptr, &m_View)
      , m_View(iWorld, iAlloc, this->m_ObjectsSpec)
    {}

    DenseGameDataView<T> m_View;
  };

  template<typename... ArgList >
  struct MultiPropertySheetBuilder;

  template<typename... ArgList >
  struct MultiPropertySheetBuilder<PositionalList<ArgList...>>
    : MultiPropertySheetBuilder<ArgList...>
  {
    MultiPropertySheetBuilder(World& iWorld, ObjectDataIndex& iIndex, DenseGameDataAllocator& iAlloc)
      : MultiPropertySheetBuilder<ArgList...>(iWorld, iIndex, iAlloc)
    {}
  };

  template<typename Arg, typename... ArgList>
  struct MultiPropertySheetBuilder<Arg, ArgList...>
    : MultiPropertySheetBuilder<ArgList...>
  {

    MultiPropertySheetBuilder(World& iWorld, ObjectDataIndex& iIndex, DenseGameDataAllocator& iAlloc)
      : MultiPropertySheetBuilder<ArgList...>(iWorld, iIndex, iAlloc)
      , m_Alloc(iWorld, iIndex, iAlloc)
    {}

    template <typename T, uint32_t iNum>
    MultiPropertySheetWrapper<typename std::enable_if<std::is_same<T, typename Arg::ArgType>::value
      && iNum == Arg::ArgPos, T>::type, iNum>& GetWrapper()
    {
      return m_Alloc;
    }

    template <typename T, uint32_t iNum>
    MultiPropertySheetWrapper<typename std::enable_if<!std::is_same<T, typename Arg::ArgType>::value
      || iNum != Arg::ArgPos, T>::type, iNum>& GetWrapper()
    {
      return MultiPropertySheetBuilder<ArgList...>::template GetWrapper<T, iNum>();
    }

    ObjectTableHandle_Base Alloc()
    {
      this->MultiPropertySheetBuilder<ArgList...>::Alloc();
      return m_Alloc.Alloc();
    }

    void Release(ObjectTableHandle_Base iHandle)
    {
      m_Alloc.Release(iHandle);
      this->MultiPropertySheetBuilder<ArgList...>::Release(iHandle);
    }

    void Clear()
    {
      m_Alloc.Clear();
      this->MultiPropertySheetBuilder<ArgList...>::Clear();
    }

    void GatherDataTable(ObjectTable_Data** oTables)
    {
      oTables[Arg::ArgPos] = &m_Alloc.m_ObjectsSpec.GetImplementation();
      MultiPropertySheetBuilder<ArgList...>::GatherDataTable(oTables);
    }

    MultiPropertySheetWrapper<typename Arg::ArgType, Arg::ArgPos> m_Alloc;
  };

  template<>
  struct MultiPropertySheetBuilder<>
  {
    MultiPropertySheetBuilder(World& iWorld, ObjectDataIndex& iIndex, DenseGameDataAllocator& iAlloc) {}
    ObjectTableHandle_Base Alloc()
    {
      return ObjectTableHandle_Base();
    }
    void Release(ObjectTableHandle_Base iHandle) {}
    void Clear() {}
    void GatherDataTable(ObjectTable_Data** oTables) {}
  };

  template <typename... Args>
  struct MultiDataStorage : protected DenseGameDataAllocator
  {
    using PositionalTypes = typename MakePositionalList<Args...>::type;

    MultiDataStorage(World& iWorld)
      : DenseGameDataAllocator(m_Index, nullptr)
      , m_Alloc(iWorld, m_Index, *this)
    {

    }
    
    template <uint32_t iNum>
    DenseGameDataView <typename Positional_Get<iNum, Args...>::type>& GetView()
    {
      return GetWrapper<iNum>().m_View;
    }

    template <uint32_t iNum>
    MultiPropertySheetWrapper<typename Positional_Get<iNum, Args...>::type, iNum>& GetWrapper()
    {
      return m_Alloc.GetWrapper<typename Positional_Get<iNum, Args...>::type, iNum>();
    }

    template <typename... Args>
    struct PositionalUnwrapper;

    template <typename... Args>
    struct PositionalUnwrapper<PositionalList<Args...>>
    {
      template <typename Function>
      static void Call(ObjectHandle iObject, Function const& iFun, ObjectTable_Data::Page** iPages, uint32_t iIdx)
      {
        return iFun(iObject, *(reinterpret_cast<typename Args::ArgType*>(iPages[Args::ArgPos]->m_Objects) + iIdx)...);
      }

      static std::tuple<typename Args::ArgType&...> GetDummy()
      {
        return std::tuple<typename Args::ArgType&...>(Dummy<Args::ArgPos>()...);
      }

      static std::tuple<typename Args::ArgType&...> GetData(MultiPropertySheetBuilder<PositionalList<Args...>>& iAlloc, ObjectTableHandle_Base iHandle)
      {
        return std::tuple<typename Args::ArgType&...>(iAlloc.template GetWrapper<typename Args::ArgType, Args::ArgPos>().m_ObjectsSpec.Get(ObjectTableHandle<typename Args::ArgType>(iHandle))...);
      }
    };

    std::tuple<Args&...> GetOrCreate(ObjectHandle iObject)
    {
      World& world = m_Alloc.m_Alloc.m_View.GetWorld();
      eXl_ASSERT_REPAIR_BEGIN(world.IsObjectValid(iObject))
      {
        return PositionalUnwrapper<PositionalTypes>::GetDummy();
      }
      uint32_t slot = this->GetSlot(iObject);
      if (slot == -1)
      {
        slot = AllocateSlot_Inl(iObject);
      }

      return PositionalUnwrapper<PositionalTypes>::GetData(m_Alloc, reinterpret_cast<ObjectTableHandle_Base&>(slot));
    }

    Optional<std::tuple<Args&...>> Get(ObjectHandle iObject)
    {
      World& world = m_Alloc.m_Alloc.m_View.GetWorld();
      if(!world.IsObjectValid(iObject))
      {
        return {};
      }
      uint32_t slot = this->GetSlot(iObject);
      if (slot == -1)
      {
        return {};
      }

      return PositionalUnwrapper<PositionalTypes>::GetData(m_Alloc, reinterpret_cast<ObjectTableHandle_Base&>(slot));
    }

    Optional<std::tuple<Args const&...>> GetDataForDeletion(ObjectHandle iObject)
    {
      World& world = m_Alloc.m_Alloc.m_View.GetWorld();
      if (!this->m_World.IsObjectValid(iObject)
        && !this->m_World.IsObjectBeingDestroyed(iObject))
      {
        return {};
      }

      uint32_t slot = this->GetSlot(iObject);
      if (slot == -1)
      {
        return {};
      }

      return PositionalUnwrapper<PositionalTypes>::GetData(m_Alloc, reinterpret_cast<ObjectTableHandle_Base&>(slot));
    }

    void Erase(ObjectHandle iObject)
    {
      uint32_t slot = this->GetSlot(iObject);
      if (slot == -1)
      {
        return;
      }

      this->EraseSlot(slot);
    }

    template <typename Functor>
    void Iterate(Functor const& iFn)
    {
      World& world = m_Alloc.m_Alloc.m_View.GetWorld();
      constexpr size_t numTables = Positional_Size<Args...>::size;
      ObjectTable_Data* tables[numTables];
      m_Alloc.GatherDataTable(tables);

      ObjectTable_Data::Page* pages[numTables];
      for (uint32_t i = 0; i < numTables; ++i)
      {
        pages[i] = tables[i]->m_Pages;
      }
      for (uint32_t pageNum = 0; pageNum < tables[0]->m_NumPages; ++pageNum)
      {
        for (uint32_t obj = 0; obj < pages[0]->m_MaxId; ++obj)
        {
          uint32_t handle = pages[0]->m_Generation[obj];
          ObjectHandle object = m_Index.m_WorldObjects[obj];
          if (world.IsObjectValid(object)
            && (handle & ObjectConstants::s_IdMask) != ObjectConstants::s_InvalidId)
          {
            PositionalUnwrapper<PositionalTypes>::Call(object, iFn, pages, obj);
          }
        }

        for (uint32_t i = 0; i < numTables; ++i)
        {
          ++pages[i];
        }
      }
    }

    template <typename Functor>
    void Iterate(Functor const& iFn) const
    {
      const_cast<MultiDataStorage<Args...>*>(this)->Iterate([&iFn](ObjectHandle iObject, Args& iData...)
        {
          iFn(iObject, const_cast<Args const&>(iData)...);
        });
    }
  protected:

    template <uint32_t iNum>
    static typename Positional_Get<iNum, Args...>::type& Dummy()
    {
      static typename Positional_Get<iNum, Args...>::type s_Dummy;
      return s_Dummy;
    }

    ObjectTableHandle_Base Alloc() override
    {
      return m_Alloc.Alloc();
    }
    void Release(ObjectTableHandle_Base iHandle) override
    {
      m_Alloc.Release(iHandle);
    }
    void Clear() override
    {
      m_Alloc.Clear();
    }

    ObjectDataIndex m_Index;
    MultiPropertySheetBuilder<PositionalTypes> m_Alloc;
  };

#include "multi.inl"
}