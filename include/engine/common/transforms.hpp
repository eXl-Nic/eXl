/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/coredef.hpp>
#include <math/matrix4.hpp>
#include <core/idgenerator.hpp>
#include <core/type/typetraits.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  class EXL_ENGINE_API Transforms : public ComponentManager
  {
    DECLARE_RTTI(Transforms, ComponentManager);
  public:

    enum AttachType
    {
      Position,
      PositionRotation
    };

    Transforms();
    ~Transforms();

    Err AddTransform(ObjectHandle iObj, Optional<Matrix4f> const& iTrans = {});
    void UpdateTransform(ObjectHandle, Matrix4f const& iTransform);
    void DeleteComponent(ObjectHandle) override;

    void Attach(ObjectHandle iChild, ObjectHandle iParent, AttachType iAttach = PositionRotation);
    void Detach(ObjectHandle iChild);

    bool HasTransform(ObjectHandle) const;
    Matrix4f const& GetLocalTransform(ObjectHandle) const;

    // Caution : can do some computation if the transform is out of date
    Matrix4f const& GetWorldTransform(ObjectHandle);

    //void Update();
    void NextFrame();

    template <typename T>
    void IterateOverDirtyTransforms(T const& iFn) const
    {
      if(m_DirtyStart < m_TotAlloc)
      {
        uint32_t curPageIdx = m_DirtyStart / s_PageSize;
        Entry curEntry;
        curEntry.m_Page = const_cast<TransformPage*>(&m_Pages[curPageIdx]);
        curEntry.m_LocalPos = m_DirtyStart % s_PageSize;
        for(; curEntry.m_Page < &m_Pages.back() + 1; ++curEntry.m_Page)
        {
          for(; curEntry.m_LocalPos < curEntry.m_Page->m_Used; ++curEntry.m_LocalPos)
          {
            if (curEntry.Timestamp() & s_NeedUpdateMask)
            {
              IterationUpdate(curEntry);
            }
            if(curEntry.Timestamp() == m_TimeStamp)
            {
              iFn(curEntry.WorldTransform(), curEntry.Owner());
            }
          }
          ++curPageIdx;
          curEntry.m_LocalPos = 0;
        }
      }
    }

  private:

    static constexpr uint32_t s_NeedUpdateMask = 0x80000000;
    static constexpr uint32_t s_TimestampMask = ~0x80000000;

    void Swap(uint32_t iOrigPos, uint32_t iDestPos);

    static const uint32_t s_PageSize = 1024;
    static const uint32_t s_OutDirtyDelay = 5;

    static constexpr uint32_t s_InvalidPos = 0x7FFFFFFF;
    static constexpr uint32_t s_AttachMask = 0x80000000;

    struct HierarchyInfo
    {
      void Init()
      {
        parent = first = next = prev = s_InvalidPos;
      }
      union
      {
        struct
        {
          uint32_t parent : 31;
          uint32_t attach : 1;
        };
        uint32_t parentField;
      };
      uint32_t first;
      uint32_t next;
      uint32_t prev;
    };

    struct TransformPage : public HeapObject
    {
      TransformPage();

      uint32_t m_Used = 0;
      void* m_Buffer;

      Matrix4f* m_WorldTransform;
      Matrix4f* m_LocalTransform;
      ObjectHandle* m_Owner;
      uint32_t* m_Timestamps;
      uint32_t* m_GlobId;
      HierarchyInfo* m_Hierarchy;
    };

    struct Entry
    {
      inline Matrix4f& WorldTransform();
      inline Matrix4f& LocalTransform();
      inline ObjectHandle& Owner();
      inline uint32_t& Timestamp();
      inline uint32_t& GlobId();
      inline HierarchyInfo& Hierarchy();

      void Copy(Entry& iOther);

      void Destroy();

      TransformPage* m_Page;
      uint32_t m_LocalPos;
    };

    Entry GetEntry(uint32_t) const;
    void Detach_Impl(Entry& iParent, Entry& iChild);
    void Remap(HierarchyInfo const& iInfo, uint32_t iOld, uint32_t iNew);
    void IterationUpdate(Entry& iEntry) const;
    void UpdateTrans(Entry& iEntry);
    void Touch(Entry& iEntry);

    Vector<TransformPage> m_Pages;
    Vector<uint32_t> m_IdToPosition;
    Vector<uint32_t> m_Stack; 

    struct PendingModif
    {
      uint32_t* dest;
      uint32_t value;
      void Apply()
      {
        *dest = (*dest & s_AttachMask) | (value & ~s_AttachMask);
      }
    };
    Vector<PendingModif> m_ModifQueue;

    void PushModif(uint32_t* iDest, uint32_t iValue);
    void ApplyModif();

    uint32_t m_TimeStamp = 0;
    uint32_t m_DirtyStart = 0;
    uint32_t m_TotAlloc = 0;
  };

  Matrix4f& Transforms::Entry::WorldTransform()
  {
    return m_Page->m_WorldTransform[m_LocalPos];
  }
  Matrix4f& Transforms::Entry::LocalTransform()
  {
    return m_Page->m_LocalTransform[m_LocalPos];
  }
  ObjectHandle& Transforms::Entry::Owner()
  {
    return m_Page->m_Owner[m_LocalPos];
  }
  uint32_t& Transforms::Entry::Timestamp()
  {
    return m_Page->m_Timestamps[m_LocalPos];
  }
  uint32_t& Transforms::Entry::GlobId()
  {
    return m_Page->m_GlobId[m_LocalPos];
  }
  Transforms::HierarchyInfo& Transforms::Entry::Hierarchy()
  {
    return m_Page->m_Hierarchy[m_LocalPos];
  }

  EXL_REFLECT_ENUM(Transforms::AttachType, eXl__Transforms__AttachType, EXL_ENGINE_API);
}