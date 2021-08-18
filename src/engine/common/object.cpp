/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/object.hpp>

namespace eXl
{
  static const uint32_t s_PageSize = 1024;

  ObjectTable_Data::ObjectTable_Data(size_t iObjectSize, size_t iAlignment)
    : m_ObjectSize(iObjectSize)
    , m_Alignment(iAlignment)
  {
    size_t testSize = 2 * iObjectSize;
    size_t testSizeOut = testSize;
    void* testPtr = (uint8_t*)iObjectSize;
    std::align(iAlignment, iObjectSize, testPtr, testSize);
    size_t additionalReq = testSize - testSizeOut;
    m_ObjectOffset = iObjectSize + additionalReq;
  }

  void* ObjectTable_Data::Alloc(ObjectTableHandle_Base& oHandle)
  {
    uint32_t globPosition = m_Ids.Get();

    uint32_t numReqPages = (globPosition / s_PageSize) + 1;

    if (numReqPages > m_NumPages)
    {
      Page* newPages = (Page*)eXl_ALLOC(numReqPages * sizeof(Page));
      for (uint32_t page = 0; page < m_NumPages; ++page)
      {
        newPages[page] = m_Pages[page];
      }
      for (uint32_t page = m_NumPages; page < numReqPages; ++page)
      {
        newPages[page].Init(m_ObjectOffset, m_Alignment);
      }
      eXl_FREE(m_Pages);
      m_Pages = newPages;
      m_NumPages = numReqPages;
    }

    Page& page = m_Pages[globPosition / s_PageSize];
    uint32_t locId = globPosition % s_PageSize;

    uint32_t handleValue = ObjectConstants::ComputeNextHandle(page.m_Generation[locId], globPosition);
    page.m_Generation[locId] = handleValue;
    oHandle.m_IdAndGen = handleValue;

    page.m_MaxId = page.m_MaxId > locId ? page.m_MaxId : locId + 1;
    ++page.m_Used;
    ++m_TotObject;

    return reinterpret_cast<char*>(page.m_Objects) + locId * m_ObjectOffset;
  }

  void* ObjectTable_Data::TryGet(ObjectTableHandle_Base iHandle) const
  {
    if (IsValid(iHandle))
    {
      uint32_t globId = iHandle.GetId();

      Page& page = m_Pages[globId / s_PageSize];

      uint32_t locId = globId % s_PageSize;

      uint32_t& gen = page.m_Generation[locId];
      if (gen == iHandle.m_IdAndGen)
      {
        return reinterpret_cast<char*>(page.m_Objects) + locId * m_ObjectOffset;
      }
    }

    return nullptr;
  }

  void* ObjectTable_Data::Get(ObjectTableHandle_Base iHandle) const
  {
    uint32_t globId = iHandle.GetId();

    Page& page = m_Pages[globId / s_PageSize];

    uint32_t locId = globId % s_PageSize;

    return reinterpret_cast<char*>(page.m_Objects) + locId * m_ObjectOffset;
  }

  void ObjectTable_Data::Release(ObjectTableHandle_Base iHandle, void(*deleter)(void*))
  {
    if(iHandle.IsAssigned())
    {
      uint32_t globId = iHandle.GetId();
      //eXl_ASSERT(globId < m_NumPages * s_PageSize);

      Page& page = m_Pages[globId / s_PageSize];

      uint32_t locId = globId % s_PageSize;
      uint32_t& gen = page.m_Generation[locId];

      if (gen != iHandle.m_IdAndGen)
      {
        return ;
      }

      deleter(reinterpret_cast<char*>(page.m_Objects) + locId * m_ObjectOffset);

      gen = ObjectConstants::ReleaseHandle(gen);

      m_Ids.Return(iHandle.GetId());

      --page.m_Used;
      --m_TotObject;

      if (locId == page.m_MaxId - 1)
      {
        for (int i = page.m_MaxId - 2; i >= 0; --i)
        {
          if ((page.m_Generation[i] & ObjectConstants::s_IdMask) != ObjectConstants::s_InvalidId)
          {
            page.m_MaxId = i + 1;
            break;
          }
        }
      }
    }
  }

  bool ObjectTable_Data::IsValid(ObjectTableHandle_Base iHandle) const
  {
    if (!iHandle.IsAssigned())
    {
      return false;
    }
    //eXl_ASSERT(iHandle.GetId() < m_NumPages * s_PageSize);

    Page& page = m_Pages[iHandle.GetId() / s_PageSize];
    uint32_t gen = page.m_Generation[iHandle.GetId() % s_PageSize];

    return gen == iHandle.m_IdAndGen;
  }

  void ObjectTable_Data::Reset(void(*deleter)(void*))
  {
    m_Ids = IdGenerator();
    for (uint32_t pageIdx = 0; pageIdx < m_NumPages; ++pageIdx)
    {
      Page& page = m_Pages[pageIdx];
      for (uint32_t i = 0; i<s_PageSize; ++i)
      {
        if ((page.m_Generation[i] & ObjectConstants::s_IdMask) != ObjectConstants::s_InvalidId)
        {
          deleter(reinterpret_cast<char*>(page.m_Objects) + i * m_ObjectOffset);
        }
      }
      m_Pages[pageIdx].~Page();
    }
    eXl_FREE(m_Pages);
    m_Pages = nullptr;
    m_NumPages = 0;
    m_TotObject = 0;
  }

  void ObjectTable_Data::Page::Init(size_t iObjectSize, size_t iAlignment)
  {
    m_Buffer = eXl_ALLOC(s_PageSize * (sizeof(uint32_t) + iObjectSize) + iAlignment);
    m_Generation = (uint32_t*)m_Buffer;
    m_Objects = m_Generation + s_PageSize;
    size_t totSize = iObjectSize * s_PageSize + iAlignment;
    std::align(iAlignment, iObjectSize, m_Objects, totSize);
    eXl_ASSERT(m_Objects && totSize >= iObjectSize * s_PageSize);
    m_Used = 0;
    m_MaxId = 0;

    for (uint32_t i = 0; i<s_PageSize; ++i)
    {
      m_Generation[i] = ObjectConstants::s_InvalidId;
    }
  }

  ObjectTable_Data::Page::~Page()
  {
    eXl_FREE(m_Buffer);
  }

}