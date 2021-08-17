#include <core/allocator.hpp>
#include <core/coredef.hpp>

namespace eXl
{
#if 0
  MemoryPool::MemoryPool(size_t iPageSize)
    : m_PageSize(iPageSize)
    , m_Pages(nullptr)
    , m_NumPages(0)
    , m_PageCapacity(0)
    , m_FullPages(0)
  {
  }

  MemoryPool::~MemoryPool()
  {
    for(Page* page = m_Pages; page < m_Pages + m_NumPages; ++page)
    {
      MemoryManager::Free(page->m_Memory, false);
    }
    MemoryManager::Free(m_Pages, false);
  }

  void* MemoryPool::Alloc(size_t iSize)
  {
    for(Page* page = m_Pages; page < m_Pages + m_NumPages; ++page)
    {
      if(page->m_CurUsage + iSize < page->m_PageSize)
      {
        size_t allocPos = page->m_CurUsage;

        if(m_FullPages > 0
        && page - m_Pages < m_FullPages
        && page->m_CurUsage >= 0.95 * page->m_PageSize)
        {
          std::swap(*page, m_Pages[m_FullPages - 1]);
          --m_FullPages;
        }

        page->m_CurUsage += iSize;
        return page->m_Memory + allocPos;
      }
    }

    //Alloc a new page for the new alloc.
    Page* newPage = AllocPage(iSize);
    newPage->m_CurUsage += iSize;
    return newPage->m_Memory;
  }

  MemoryPool::Page* MemoryPool::AllocPage(size_t iSize)
  {
    if(m_NumPages >= m_PageCapacity)
    {
      size_t newPageCapacity = m_PageCapacity * 1.5;
      if(newPageCapacity <= m_NumPages)
      {
        newPageCapacity = m_NumPages + 1;
      }

      Page* newPageArray = (Page*)MemoryManager::Allocate(sizeof(Page) * newPageCapacity);
      memcpy(newPageArray, m_Pages, m_NumPages * sizeof(Page));
      MemoryManager::Free(m_Pages, false);
      m_Pages = newPageArray;
      m_PageCapacity = newPageCapacity;
    }

    uint32_t newPageIdx;
    if(iSize < m_PageSize)
    {
      newPageIdx = m_FullPages;
      if(m_FullPages < m_NumPages)
      {
        std::swap(m_Pages[m_FullPages], m_Pages[m_NumPages]);
      }
      ++m_FullPages;
    }
    else
    {
      newPageIdx = m_NumPages;
    }
    ++m_NumPages;

    Page* newPage = m_Pages + newPageIdx;
    newPage->m_CurUsage = 0;
    newPage->m_PageSize = iSize > m_PageSize ? iSize : m_PageSize;
    newPage->m_Memory = (char*)MemoryManager::Allocate(newPage->m_PageSize);

    return newPage;
  }

  void MemoryPool::Reset()
  {
    for(Page* page = m_Pages; page < m_Pages + m_NumPages; ++page)
    {
      page->m_CurUsage = 0;
    }
    m_FullPages = m_NumPages;
  }
#endif
}