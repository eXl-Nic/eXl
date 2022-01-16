/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/transforms.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(Transforms);

  Transforms::Transforms()
  {
    m_IdToPosition.reserve(s_PageSize);
    m_Stack.reserve(256);
    m_ModifQueue.reserve(16);
  }

  Transforms::~Transforms()
  {
    for(auto& page : m_Pages)
    {
      eXl_FREE(page.m_Buffer);
    }
  }

  Err Transforms::AddTransform(ObjectHandle iObj, Optional<Matrix4f> const& iTrans)
  {
    if(!GetWorld().IsObjectValid(iObj))
    {
      return Err::Failure;
    }

    if(m_Pages.empty() || m_Pages.back().m_Used == s_PageSize)
    {
      m_Pages.emplace_back(TransformPage());
    }

    uint32_t extId = iObj.GetId();
    while(m_IdToPosition.size() < extId + 1)
    {
      m_IdToPosition.push_back(s_InvalidPos);
    }

    TransformPage& page = m_Pages.back();

    uint32_t globPosition = (m_Pages.size() - 1) * s_PageSize + page.m_Used;
    if(iTrans)
    {
      page.m_WorldTransform[page.m_Used] = page.m_LocalTransform[page.m_Used] = *iTrans;
    }
    else
    {
      page.m_WorldTransform[page.m_Used].MakeIdentity();
      page.m_LocalTransform[page.m_Used].MakeIdentity();
    }
    new(page.m_Owner + page.m_Used) ObjectHandle(iObj);
    page.m_Timestamps[page.m_Used] = m_TimeStamp | s_NeedUpdateMask;
    page.m_GlobId[page.m_Used] = globPosition;
    page.m_Hierarchy[page.m_Used].Init();
    
    ++page.m_Used;
    ++m_TotAlloc;
    m_IdToPosition[extId] = globPosition;
    ComponentManager::CreateComponent(iObj);
    return Err::Success;
  }

  void Transforms::Entry::Copy(Entry& iOther)
  {
    LocalTransform() = iOther.LocalTransform();
    WorldTransform() = iOther.WorldTransform();
    Owner() = iOther.Owner();
    Timestamp() = iOther.Timestamp();
    GlobId() = iOther.GlobId();
    Hierarchy() = iOther.Hierarchy();
  }

  void Transforms::Entry::Destroy()
  {
    m_Page->m_Owner[m_LocalPos].~ObjectHandle();
  }

  void Transforms::Detach_Impl(Entry& Parent, Entry& Child)
  {
    HierarchyInfo& parentInfo = Parent.Hierarchy();
    HierarchyInfo& info = Child.Hierarchy();

    if (parentInfo.first == Child.GlobId())
    {
      parentInfo.first = info.next;
    }
    
    if (info.next != s_InvalidPos)
    {
      Entry nextEntry = GetEntry(info.next);
      if (info.prev != s_InvalidPos)
      {
        Entry prevEntry = GetEntry(info.prev);
        nextEntry.Hierarchy().prev = prevEntry.GlobId();
        prevEntry.Hierarchy().next = nextEntry.GlobId();
      }
      else
      {
        nextEntry.Hierarchy().prev = s_InvalidPos;
      }
    }
    else if (info.prev != s_InvalidPos)
    {
      Entry prevEntry = GetEntry(Child.Hierarchy().prev);
      prevEntry.Hierarchy().next = s_InvalidPos;
    }
    info.parent = s_InvalidPos;
    info.next = s_InvalidPos;
    info.prev = s_InvalidPos;
  }

  Transforms::Entry Transforms::GetEntry(uint32_t iGlobPos) const
  {
    Entry h;
    h.m_Page = const_cast<TransformPage*>(&m_Pages[iGlobPos / s_PageSize]);
    h.m_LocalPos = iGlobPos % s_PageSize;

    return h;
  }

  void Transforms::Touch(Entry& iEntry)
  {
    m_Stack.push_back(iEntry.GlobId());
    while (!m_Stack.empty())
    {
      uint32_t curEntry = m_Stack.back();
      m_Stack.pop_back();
      Entry trans = GetEntry(curEntry);
      trans.Timestamp() = m_TimeStamp | s_NeedUpdateMask;

      if (curEntry < m_DirtyStart)
      {
        Swap(curEntry, m_DirtyStart - 1);
        --m_DirtyStart;
        curEntry = m_DirtyStart;
        trans = GetEntry(curEntry);
      }

      uint32_t curChild = trans.Hierarchy().first;
      while (curChild != s_InvalidPos)
      {
        m_Stack.push_back(curChild);
        curChild = GetEntry(curChild).Hierarchy().next;
      }
    }
  }

  void Transforms::UpdateTransform(ObjectHandle iObj, Matrix4f const& iTransform)
  {
    if (!GetWorld().IsObjectValid(iObj))
    {
      return;
    }

    uint32_t extId = iObj.GetId();
    if(extId < m_IdToPosition.size() && m_IdToPosition[extId] != s_InvalidPos)
    {
      uint32_t globPosition = m_IdToPosition[extId];
      Entry updatedEntry = GetEntry(globPosition);
      updatedEntry.LocalTransform() = iTransform;
      Touch(updatedEntry);
    }
  }

  void Transforms::DeleteComponent(ObjectHandle iObj)
  {
    uint32_t extId = iObj.GetId();
    if(extId < m_IdToPosition.size() && m_IdToPosition[extId] != s_InvalidPos)
    {
      uint32_t globPosition = m_IdToPosition[extId];
      Entry deletedEntry = GetEntry(globPosition);
      {
        Touch(deletedEntry);
        globPosition = m_IdToPosition[extId];
        deletedEntry = GetEntry(globPosition);

        uint32_t parentPos = deletedEntry.Hierarchy().parent;
        if (parentPos != s_InvalidPos)
        {
          Entry parentEntry = GetEntry(parentPos);
          Detach_Impl(parentEntry, deletedEntry);
        }
        uint32_t child = deletedEntry.Hierarchy().first;
        while (child != s_InvalidPos)
        {
          Entry childEntry = GetEntry(child);
          uint32_t next = childEntry.Hierarchy().next;
          
          Detach_Impl(deletedEntry, childEntry);
          child = next;
        }
      }
      
      auto& lastPage = m_Pages.back();

      if (deletedEntry.m_Page != &lastPage || (deletedEntry.m_LocalPos + 1) != lastPage.m_Used)
      {
        Swap(globPosition, m_TotAlloc - 1);
      }
      Entry lastEntry = GetEntry(m_TotAlloc - 1);
      lastEntry.Destroy();

      lastPage.m_Used--;
      if(lastPage.m_Used == 0)
      {
        eXl_FREE(m_Pages.back().m_Buffer);
        m_Pages.pop_back();
      }
      --m_TotAlloc;
      m_IdToPosition[extId] = s_InvalidPos;

      ComponentManager::DeleteComponent(iObj);
    }
  }

  void Transforms::Attach(ObjectHandle iChild, ObjectHandle iParent, AttachType iAttach)
  {
    if (!GetWorld().IsObjectValid(iChild)
      || !GetWorld().IsObjectValid(iParent))
    {
      return;
    }

    uint32_t extIdChild = iChild.GetId();
    uint32_t extIdParent = iParent.GetId();

    if (extIdChild < m_IdToPosition.size() && m_IdToPosition[extIdChild] != s_InvalidPos
      && extIdParent < m_IdToPosition.size() && m_IdToPosition[extIdParent] != s_InvalidPos)
    {
      uint32_t posChild = m_IdToPosition[extIdChild];
      uint32_t posParent = m_IdToPosition[extIdParent];

      Entry parent = GetEntry(posParent);
      Entry child = GetEntry(posChild);
      if (child.Hierarchy().parent != s_InvalidPos)
      {
        Detach_Impl(parent, child);
      }

      HierarchyInfo& parentInfo = parent.Hierarchy();
      HierarchyInfo& info = child.Hierarchy();
      
      info.parent = posParent;
      info.attach = iAttach;
      info.next = parentInfo.first;
      if (info.next != s_InvalidPos)
      {
        Entry sibling = GetEntry(info.next);
        sibling.Hierarchy().prev = posChild;
      }
      parentInfo.first = posChild;
      Touch(child);
    }
  }

  void Transforms::Detach(ObjectHandle iChild)
  {
    if (!GetWorld().IsObjectValid(iChild))
    {
      return;
    }

    uint32_t extIdChild = iChild.GetId();
    if (extIdChild < m_IdToPosition.size() && m_IdToPosition[extIdChild] != s_InvalidPos)
    {
      Entry childEntry = GetEntry(m_IdToPosition[extIdChild]);
      uint32_t parentGlobPos = childEntry.Hierarchy().parent;
      if (parentGlobPos != s_InvalidPos)
      {
        Entry parentEntry = GetEntry(parentGlobPos);
        Detach_Impl(parentEntry, childEntry);
        Touch(childEntry);
      }
    }
  }

  bool Transforms::HasTransform(ObjectHandle iObj) const
  {
    if (GetWorld().IsObjectValid(iObj))
    {
      uint32_t extId = iObj.GetId();
      if(extId < m_IdToPosition.size() && m_IdToPosition[extId] != s_InvalidPos)
      {
        return true;
      }
    }
    return false;
  }

  Matrix4f const& Transforms::GetLocalTransform(ObjectHandle iObj) const
  {
    if (GetWorld().IsObjectValid(iObj))
    {
      uint32_t extId = iObj.GetId();
      if(extId < m_IdToPosition.size() && m_IdToPosition[extId] != s_InvalidPos)
      {
        return GetEntry(m_IdToPosition[extId]).LocalTransform();
      }
    }
    //else
    //{
    //  eXl_ASSERT(false);
    //}

    static const Matrix4f dummyMatrix = [] 
    { 
      Matrix4f mat; 
      mat.MakeIdentity(); 
      return mat;
    }();

    return dummyMatrix;
  }

  void Transforms::UpdateTrans(Entry& iEntry)
  {
    if (iEntry.Hierarchy().parent == s_InvalidPos)
    {
      iEntry.WorldTransform() = iEntry.LocalTransform();
      iEntry.Timestamp() &= s_TimestampMask;

      return;
    }

    m_Stack.push_back(iEntry.GlobId());
    uint32_t toInspect = m_Stack.back();
    while (true)
    {
      Entry curEntry = GetEntry(toInspect);
      uint32_t parentPos = curEntry.Hierarchy().parent;
      if (parentPos == s_InvalidPos)
      {
        break;
      }
      Entry parentEntry = GetEntry(parentPos);
      if (parentEntry.Timestamp() & s_NeedUpdateMask)
      {
        toInspect = parentPos;
        m_Stack.push_back(parentPos);
      }
      else
      {
        break;
      }
    }

    while(!m_Stack.empty())
    {
      uint32_t toUpdate = m_Stack.back();
      m_Stack.pop_back();

      Entry entryToUpdate = GetEntry(toUpdate);
      HierarchyInfo hierarchy = entryToUpdate.Hierarchy();
      uint32_t parentPos = hierarchy.parent;
      AttachType attach = /*PositionRotation;*/ (AttachType)hierarchy.attach;
      Matrix4f& worldTransform = entryToUpdate.WorldTransform();

      if (parentPos != s_InvalidPos)
      {
        Entry parentEntry = GetEntry(parentPos);
        Matrix4f& parentWorldTransform = parentEntry.WorldTransform();
        if (attach == PositionRotation)
        {
          worldTransform = parentWorldTransform * entryToUpdate.LocalTransform();
        }
        else
        {
          worldTransform = entryToUpdate.LocalTransform();
          worldTransform.m_Data[12] += parentWorldTransform.m_Data[12];
          worldTransform.m_Data[13] += parentWorldTransform.m_Data[13];
          worldTransform.m_Data[14] += parentWorldTransform.m_Data[14];
        }
      }
      else
      {
        entryToUpdate.WorldTransform() = entryToUpdate.LocalTransform();
      }

      entryToUpdate.Timestamp() &= s_TimestampMask;
    }
  }

  void Transforms::IterationUpdate(Entry& iEntry) const
  {
    uint32_t entryParent = iEntry.Hierarchy().parent;
    if (entryParent != s_InvalidPos && entryParent > iEntry.GlobId())
    {
      const_cast<Transforms*>(this)->Swap(entryParent, iEntry.GlobId());
    }
    const_cast<Transforms*>(this)->UpdateTrans(iEntry);
  }

  Matrix4f const& Transforms::GetWorldTransform(ObjectHandle iObj)
  {
    if (GetWorld().IsObjectValid(iObj))
    {
      uint32_t extId = iObj.GetId();
      if (extId < m_IdToPosition.size() && m_IdToPosition[extId] != s_InvalidPos)
      {
        uint32_t globPosition = m_IdToPosition[extId];
        Entry curEntry = GetEntry(globPosition);
        if ((curEntry.Timestamp() & s_NeedUpdateMask) == 0)
        {
          return curEntry.WorldTransform();
        }
        else
        {
          UpdateTrans(curEntry);
          return curEntry.WorldTransform();

        }
        //return GetEntry(m_IdToPosition[extId]).LocalTransform();
      }
    }
    //else
    //{
    //  eXl_ASSERT(false);
    //}

    static const Matrix4f dummyMatrix = []
    {
      Matrix4f mat;
      mat.MakeIdentity();
      return mat;
    }();

    return dummyMatrix;
  }

  //void Transforms::Update()
  //{
  //  if (m_DirtyStart < m_TotAlloc)
  //  {
  //    uint32_t curPageIdx = m_DirtyStart / s_PageSize;
  //    Entry curEntry;
  //    curEntry.m_Page = &m_Pages[curPageIdx];
  //    curEntry.m_LocalPos = m_DirtyStart % s_PageSize;
  //    for (; curEntry.m_Page < &m_Pages.back() + 1; ++curEntry.m_Page)
  //    {
  //      for (; curEntry.m_LocalPos < curEntry.m_Page->m_Used; ++curEntry.m_LocalPos)
  //      {
  //        if (curEntry.Timestamp() & s_NeedUpdateMask)
  //        {
  //          Update(curEntry);
  //        }
  //      }
  //      ++curPageIdx;
  //      curEntry.m_LocalPos = 0;
  //    }
  //  }
  //}

  void Transforms::NextFrame()
  {
    ++m_TimeStamp;
    if(m_DirtyStart < m_TotAlloc)
    {
      uint32_t curPos = m_DirtyStart;
      uint32_t curPageIdx = curPos / s_PageSize;
      Entry curTrans;
      curTrans.m_Page = &m_Pages[curPageIdx];
      curTrans.m_LocalPos = curPos % s_PageSize;
      for(; curTrans.m_Page < &m_Pages.back() + 1; ++curTrans.m_Page)
      {
        for(; curTrans.m_LocalPos < curTrans.m_Page->m_Used; ++curTrans.m_LocalPos)
        {
          if(curTrans.Timestamp() + s_OutDirtyDelay < m_TimeStamp)
          {
            if(m_DirtyStart != curPos)
            {
              Swap(curPos, m_DirtyStart);
            }
            ++m_DirtyStart;
          }
          ++curPos;
        }
        ++curPageIdx;
        ++curTrans.m_Page;
        curTrans.m_LocalPos = 0;
      }
    }
  }

  void Transforms::PushModif(uint32_t* iDest, uint32_t iValue)
  {
    PendingModif modif = { iDest, iValue };
    m_ModifQueue.push_back(modif);
  }

  void Transforms::ApplyModif()
  {
    for (auto& modif : m_ModifQueue)
    {
      modif.Apply();
    }

    m_ModifQueue.clear();
  }

  void Transforms::Remap(HierarchyInfo const& iInfo, uint32_t iOld, uint32_t iNew)
  {
    if (iInfo.parent != s_InvalidPos)
    {
      Entry parent = GetEntry(iInfo.parent);
      if (parent.Hierarchy().first == iOld)
      {
        PushModif(&parent.Hierarchy().first, iNew);
      }
      if (iInfo.prev != s_InvalidPos)
      {
        PushModif(&GetEntry(iInfo.prev).Hierarchy().next, iNew);
      }
      if (iInfo.next != s_InvalidPos)
      {
        PushModif(&GetEntry(iInfo.next).Hierarchy().prev, iNew);
      }
    }

    uint32_t curChild = iInfo.first;
    while(curChild != s_InvalidPos)
    {
      Entry child = GetEntry(curChild);
      PushModif(&child.Hierarchy().parentField, iNew);
      curChild = child.Hierarchy().next;
    }
  }

  void Transforms::Swap(uint32_t iOrigPos, uint32_t iDestPos)
  {
    if(iOrigPos != iDestPos)
    {
      Entry origEntry = GetEntry(iOrigPos);
      Entry destEntry = GetEntry(iDestPos);

      Remap(origEntry.Hierarchy(), iOrigPos, iDestPos);
      Remap(destEntry.Hierarchy(), iDestPos, iOrigPos);

      ApplyModif();

      Matrix4f saveDestLTransform = destEntry.LocalTransform();
      Matrix4f saveDestWTransform = destEntry.WorldTransform();
      ObjectHandle saveDestOwner = destEntry.Owner();
      uint32_t saveDestTimestamp = destEntry.Timestamp();
      HierarchyInfo saveHierarchyInfo = destEntry.Hierarchy();

      destEntry.Copy(origEntry);

      origEntry.LocalTransform() = saveDestLTransform;
      origEntry.WorldTransform() = saveDestWTransform;
      origEntry.Owner() = saveDestOwner;
      origEntry.Timestamp() = saveDestTimestamp;
      origEntry.Hierarchy() = saveHierarchyInfo;

      origEntry.GlobId() = iOrigPos;
      destEntry.GlobId() = iDestPos;

      m_IdToPosition[destEntry.Owner().GetId()] = iDestPos;
      m_IdToPosition[origEntry.Owner().GetId()] = iOrigPos;
    }
  }

  Transforms::TransformPage::TransformPage()
  {
    static size_t s_pageBufferSize = (2 * sizeof(Matrix4f) + sizeof(ObjectHandle) + 2*sizeof(uint32_t) + sizeof(HierarchyInfo)) * s_PageSize;

    m_Buffer = eXl_ALLOC(s_pageBufferSize);

    m_LocalTransform = reinterpret_cast<Matrix4f*>(m_Buffer);
    m_WorldTransform = reinterpret_cast<Matrix4f*>(m_LocalTransform + s_PageSize);
    m_Owner = reinterpret_cast<ObjectHandle*>(m_WorldTransform + s_PageSize);
    m_Timestamps = reinterpret_cast<uint32_t*>(m_Owner + s_PageSize);
    m_GlobId = reinterpret_cast<uint32_t*>(m_Timestamps + s_PageSize);
    m_Hierarchy = reinterpret_cast<HierarchyInfo*>(m_GlobId + s_PageSize);
  }
}
