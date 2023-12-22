/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/memorymanager.hpp>
#include <new>
#include <map>
#include <mutex>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/global_fun.hpp>
#include <core/coredef.hpp>
#include <core/log.hpp>
#include <core/utils/capturestack.hpp>

#if defined(EXL_TRACE_LEAKS)

#define DEBUG_ALLOC

#endif

namespace eXl
{
#if defined(EXL_TRACE_LEAKS)
  EXL_CORE_API bool s_CaptureStacks = true;
#endif
  struct AllocStack
  {
    Stack m_Stack;
    DECLARE_RefC;

    void OnNullRefC() const
    {}
  
  public:

    bool HasReferences() const
    {
      return m_RefCount > 0;
    }
  };

  IMPLEMENT_RefCCustom(AllocStack);

  size_t hash_value(Stack const& iStack)
  {
    return iStack.m_Hash;
  }

  size_t hash_value(IntrusivePtr<AllocStack> const& iStack)
  {
    return iStack->m_Stack.m_Hash;
  }


  Stack const& RetrieveStack(IntrusivePtr<AllocStack> const& iPtr)
  {
    return iPtr->m_Stack;
  }

  using AllocStackMap = boost::multi_index::multi_index_container< IntrusivePtr<AllocStack>,
    boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<boost::multi_index::global_fun<IntrusivePtr<AllocStack> const&, Stack const&, &RetrieveStack>>,
    boost::multi_index::random_access<>>
    , RawAllocator<IntrusivePtr<AllocStack>>>;
  typedef AllocStackMap::nth_index<0>::type StackMap_by_stack;

  struct MemRec
  {
    size_t size;
    unsigned int numElems;
    unsigned int line;
    const char* file;
    const char* func;

    IntrusivePtr<AllocStack> allocStack;
    void* ptr;
  };

  typedef boost::multi_index::multi_index_container<MemRec,
    boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<boost::multi_index::member<MemRec,void*,&MemRec::ptr> > > 
  , RawAllocator<MemRec>> MemMap;
  typedef MemMap::nth_index<0>::type MemMap_by_ptr;

  static MemMap& GetMemMap()
  {
    static MemMap s_Map;

    return s_Map;
  }

  static AllocStackMap& GetStackMap()
  {
    static AllocStackMap s_Map;

    return s_Map;
  }

  static std::recursive_mutex& GetMemLock()
  {
    static std::recursive_mutex s_MemLock;

    return s_MemLock;
  }

  IntrusivePtr<AllocStack> CaptureMemStack()
  {
    AllocStack stack;
    stack.m_Stack = CaptureStack();
    
    auto& stackMap = GetStackMap();

    auto iter = stackMap.find(stack.m_Stack);
    if (iter == stackMap.end())
    {
      AllocStack* newStack = new(malloc(sizeof(AllocStack))) AllocStack;
      newStack->m_Stack = std::move(stack.m_Stack);
      return *stackMap.insert(IntrusivePtr<AllocStack>(newStack)).first;
    }
    else
    {
      return *iter;
    }
  }

  static void*(*s_AllocFn)(size_t) = nullptr;
  static void(*s_FreeFn)(void*) = nullptr;

  void MemoryManager::SetAllocFn(void*(*AllocFn)(size_t))
  {
    if(AllocFn)
    {
      s_AllocFn = AllocFn;
    }
  }

  void MemoryManager::SetFreeFn(void(*FreeFn)(void*))
  {
    if(FreeFn)
    {
      s_FreeFn = FreeFn;
    }
  }

  void* MemoryManager::Allocate(size_t size,size_t numElems)
  {
    if(!s_AllocFn)
      s_AllocFn = &malloc;
    void* res=s_AllocFn(size);
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(res!=nullptr,"Out of Memory");
    MemRec newRec;
    newRec.size=size;
    newRec.line=0;
    newRec.file=nullptr;
    newRec.func=nullptr;
    newRec.ptr=res;
    newRec.numElems=numElems;
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    if (s_CaptureStacks)
    {
      newRec.allocStack = CaptureMemStack();
    }
    GetMemMap().insert(newRec);
#endif
    return res;
  }
  void* MemoryManager::Allocate(size_t size,const char* file,unsigned int line,const char* iFun,size_t numElems)
  {
    if(!s_AllocFn)
      s_AllocFn = &malloc;
    void* res=s_AllocFn(size);
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(res!=nullptr,"Out of Memory");
    MemRec newRec;
    newRec.size=size;
    newRec.line=line;
    newRec.file=file;
    newRec.func=iFun;
    newRec.ptr=res;
    newRec.numElems=numElems;
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    GetMemMap().insert(newRec);
#endif
    return res;
  }

  void* MemoryManager::Allocate_Ext(size_t size,const char* file,unsigned int line,const char* iFun,size_t numElems,void *(*iAlloc)(size_t , int ))
  {
    void* res=iAlloc(size,16);
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(res!=nullptr,"Out of Memory");
    MemRec newRec;
    newRec.size=size;
    newRec.line=line;
    newRec.file=file;
    newRec.func=iFun;
    newRec.ptr=res;
    newRec.numElems=numElems;
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    GetMemMap().insert(newRec);
#endif
    return res;
  }
  
  void MemoryManager::Free(void* ptr,bool array)
  {
    if(!s_FreeFn)
      s_FreeFn = &free;
    if (ptr == nullptr)
    {
      return;
    }
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr")).c_str());
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    MemMap_by_ptr::iterator iter= GetMemMap().get<0>().find(ptr);
    eXl_ASSERT_MSG_REPAIR_RET(iter!= GetMemMap().end(),"Trying to free unmanaged memory", void());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor for memory allocated in ")+iter->file+" at "+StringUtil::FromInt(iter->line)).c_str());
    GetMemMap().erase(iter);
#endif
    s_FreeFn(ptr);
  }

  void MemoryManager::Free(void* ptr,const char* file,unsigned int line,const char* iFun,bool array)
  {
    if(!s_FreeFn)
      s_FreeFn = &free;
    if (ptr == nullptr)
    {
      return;
    }
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    MemMap_by_ptr::iterator iter= GetMemMap().get<0>().find(ptr);
    eXl_ASSERT_MSG_REPAIR_RET(iter != GetMemMap().end(), eXl_FORMAT("Trying to free unmanaged memory in %s at %i", file, line), void());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor called for memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    GetMemMap().erase(iter);
#endif
    s_FreeFn(ptr);
  }

  void MemoryManager::Free_Ext(void* ptr,const char* file,unsigned int line,const char* iFun,bool array,void (*iFree)(void*))
  {
    if (ptr == nullptr)
    {
      return;
    }
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    MemMap_by_ptr::iterator iter= GetMemMap().get<0>().find(ptr);
    eXl_ASSERT_MSG_REPAIR_RET(iter != GetMemMap().end(), eXl_FORMAT("Trying to free unmanaged memory in %s at %i", file, line), void());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor called for memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    GetMemMap().erase(iter);
#endif
    iFree(ptr);
    
  }

  struct CompStr
  {
    bool operator()(char const* const& iStr1, char const* const& iStr2)const
    {
      return strcmp(iStr1,iStr2) < 0;
    }
  };

  void MemoryManager::ReportLeaks()
  {
    char buff[4096];
    char* buffer = &buff[0];
    size_t totalLeak=0;

    auto const& stackMap = GetStackMap();
    std::vector<DebugString, RawAllocator<DebugString>> stackDumps;
    stackDumps.resize(stackMap.size());
    for (auto stackIter = stackMap.begin(); stackIter != stackMap.end(); ++stackIter)
    {
      if ((*stackIter)->HasReferences())
      {
        uint32_t idx = stackMap.project<1>(stackIter) - stackMap.get<1>().begin();
        stackDumps[idx] = DumpStackInformation((*stackIter)->m_Stack);
      }
    }

    std::map<char const* ,unsigned int,CompStr, RawAllocator<std::pair<char const* const, unsigned int>>> m_SetPos;
    {
      MemMap::iterator iter = GetMemMap().begin();
      MemMap::iterator iterEnd = GetMemMap().end();

      for(;iter!=iterEnd;iter++)
      {
        totalLeak+=iter->size;
        if(iter->file!=nullptr)
        {
          snprintf(buffer, 4096,("Leak in file %s in function %s at line %i of size %zi"),iter->file,iter->func,iter->line,iter->size);
          std::map<char const* ,unsigned int,CompStr>::iterator iter = m_SetPos.find(buffer);
          if(iter == m_SetPos.end())
          {
            size_t strLen = strlen(buffer);
            char* strCopy = (char*)malloc(strLen + 1);
            memcpy(strCopy,buffer, strLen + 1);
            strCopy[strLen] = 0;
            std::pair<std::map<char const* ,unsigned int,CompStr>::iterator,bool> res = m_SetPos.insert(std::make_pair(strCopy,0));
            iter = res.first;
          }
          iter->second++;
        }
        else if (iter->allocStack)
        {
          auto stackIter = stackMap.find(iter->allocStack->m_Stack);
          if (stackIter != stackMap.end())
          {
            uint32_t idx = stackMap.project<1>(stackIter) - stackMap.get<1>().begin();
            if (idx < stackDumps.size())
            {
              char const* strPtr = stackDumps[idx].c_str();
              std::map<char const*, unsigned int, CompStr>::iterator iter = m_SetPos.find(strPtr);
              if (iter == m_SetPos.end())
              {
                std::pair<std::map<char const*, unsigned int, CompStr>::iterator, bool> res = m_SetPos.insert(std::make_pair(strPtr, 0));
                iter = res.first;
              }
              iter->second++;
            }
          }
        }
        else
        {
          //sprintf(buffer,"Leak at %p of size %i",iter->ptr,iter->size);
          LOG_INFO<<"Leak at "<<iter->ptr<<" of size "<<static_cast<uint32_t>(iter->size)<<"\n";
        } 
      }
    }
    std::map<char const* ,unsigned int,CompStr>::iterator iter = m_SetPos.begin();
    std::map<char const* ,unsigned int,CompStr>::iterator iterEnd = m_SetPos.end();
    for(;iter!=iterEnd;iter++)
    {
      if (iter->second > 1)
      {
        //totalLeak+=iter->size;
        LOG_INFO << iter->first << " X " << iter->second << "\n";
        //free((void*)iter->first);
      }
    }
    LOG_INFO<<"Total memory leaks : "<< static_cast<uint32_t>(totalLeak)<<"\n";
  }
  
  unsigned int MemoryManager::GetNum(void* iPtr,size_t& oStride,const char* file,unsigned int line,const char* iFun)
  {
    std::unique_lock<std::recursive_mutex> mapLock(GetMemLock());
    MemMap_by_ptr::iterator iter= GetMemMap().get<0>().find(iPtr);
    //eXl_ASSERT_MSG(iter!=m_Map.end(),(String("Trying to free unmanaged memory in ")+StringUtil::FromASCII(file)+" at "+StringUtil::FromInt(line)).c_str());
    //eXl_ASSERT_MSG(iter->numElems>0,(String("Wrong dtor called memory in ")+StringUtil::FromASCII(file)+" at "+StringUtil::FromInt(line)).c_str());
    oStride = iter->size/iter->numElems;
    return iter->numElems;
  }

}
