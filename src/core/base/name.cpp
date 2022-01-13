/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/name.hpp>
#include <boost/optional.hpp>
#include <mutex>
#include <shared_mutex>

namespace eXl
{
#ifdef EXL_NAME_BOOST_IMPL

#if defined(EXL_SHARED_LIBRARY) && defined(EXL_NAME_EXPLICIT_INIT)
  boost::aligned_storage<sizeof(NameCoreHolder)> s_NameSingleton;
#endif

  void Name_Init()
  {
#if defined(EXL_SHARED_LIBRARY) && defined(EXL_NAME_EXPLICIT_INIT)
    new(s_NameSingleton.address()) NameCoreHolder;
    NameCore::init();
#endif
  }

  void Name_Destroy()
  {
#if defined(EXL_SHARED_LIBRARY) && defined(EXL_NAME_EXPLICIT_INIT)
    TNameCoreSingleton<NameCoreHolder>::get().~NameCoreHolder();
#endif
  }

#if defined(EXL_SHARED_LIBRARY) && defined(EXL_NAME_EXPLICIT_INIT)
  NameCoreHolder& TNameCoreSingleton<NameCoreHolder>::get()
  {
    return *reinterpret_cast<NameCoreHolder*>(s_NameSingleton.address());
  }
#endif

#else

  struct NameAllocHolder
  {

    NameAllocHolder()
    {
      AllocPage(0);
    }
    NameAllocHolder(NameAllocHolder const&) = delete;
    NameAllocHolder& operator = (NameAllocHolder const&) = delete;

    uint32_t AllocPage(size_t iSize)
    {
      size_t const defaultPageSize = 4096;
      uint32_t pageIdx = m_Pages.size() - 1;
      if (iSize > defaultPageSize / 2)
      {
        m_Pages.emplace_back(iSize);
        if (m_Pages.size() > 1)
        {
          --pageIdx;
          m_Pages[pageIdx].Swap(m_Pages[pageIdx + 1]);
        }
      }
      else
      {
        m_Pages.emplace_back(defaultPageSize);     
      }

      return pageIdx;
    }

    struct Page
    {
      Page(size_t iSize)
      {
        m_Alloc = (char*)eXl_ALLOC(iSize);
        m_Available = iSize;
        m_Cur = m_Alloc;
      }
      ~Page()
      {
        eXl_FREE(m_Alloc);
      }

      Page(Page&& iMoved)
      {
        m_Alloc = iMoved.m_Alloc;
        m_Available = iMoved.m_Available;
        m_Cur = iMoved.m_Cur;
        iMoved.m_Alloc = nullptr;
      }
      Page(Page const&) = delete;
      Page& operator=(Page const&) = delete;

      void Swap(Page& iOther)
      {
        Char* tempAlloc = iOther.m_Alloc;
        size_t tempAvailable = iOther.m_Available;
        Char* tempCur = iOther.m_Cur;

        iOther.m_Alloc = m_Alloc;
        iOther.m_Available = m_Available;
        iOther.m_Cur = m_Cur;

        m_Alloc = tempAlloc;
        m_Available = tempAvailable;
        m_Cur = tempCur;
      }

      Char* m_Alloc;
      size_t m_Available;
      Char* m_Cur;
    };

    using ReadLock = std::shared_lock<std::shared_mutex>;
    using WriteLock = std::unique_lock<std::shared_mutex>;

    KString Get(KString iStr)
    {
      {
        ReadLock lock(m_Mutex);
        auto iter = m_Strings.find(iStr);
        if (iter != m_Strings.end())
        {
          return *iter;
        }
      }

      WriteLock lock(m_Mutex);
      uint32_t const pageIdx = (m_Pages.back().m_Available < iStr.size() + 1) ?
        AllocPage(iStr.size() + 1) : m_Pages.size() - 1;

      Page& page = m_Pages[pageIdx];
      char* insertPos = page.m_Cur;
      KString newStr(insertPos, iStr.size());
      memcpy(insertPos, iStr.data(), iStr.size());
      insertPos += iStr.size();
      insertPos[0] = 0;
      insertPos++;

      auto insertRes = m_Strings.insert(newStr);
      if (insertRes.second)
      {
        page.m_Available -= iStr.size() + 1;
        page.m_Cur = insertPos;
      }
      
      return *(insertRes.first);
    }
    

    UnorderedSet<KString> m_Strings;
    Vector<Page> m_Pages;
    std::shared_mutex m_Mutex;
  };

  Optional<NameAllocHolder> s_Names;

  void Name_Init()
  {
    s_Names.emplace();
  }

  void Name_Destroy()
  {
    s_Names.reset();
  }

  Name::Name()
  {
    m_Str = KString();
  }

  Name::Name(String const& iStr)
  {
    m_Str = s_Names->Get(KString(iStr));
  }

  Name::Name(Char const* iStr) 
  {
    m_Str = s_Names->Get(KString(iStr, strlen(iStr)));
  }

  Name& Name::operator=(Char const* iStr)
  {
    m_Str = s_Names->Get(KString(iStr, strlen(iStr)));
    return *this;
  }

#endif
}