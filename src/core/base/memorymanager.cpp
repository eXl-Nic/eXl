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
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <core/coredef.hpp>
#include <core/log.hpp>

#if defined(_DEBUG) && defined(TRACE_LEAKS)

//#define DEBUG_ALLOC

#endif

namespace eXl
{
  struct MemRec
  {
    size_t size;
    unsigned int numElems;
    unsigned int line;
    const char* file;
    const char* func;
    void* ptr;
  };

  typedef boost::multi_index::multi_index_container<MemRec,
    boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<boost::multi_index::member<MemRec,void*,&MemRec::ptr> > > > MemMap;
  typedef MemMap::nth_index<0>::type MemMap_by_ptr;

  static MemMap m_Map;
  static std::mutex memLock;

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
    std::unique_lock<std::mutex> mapLock(memLock);
    m_Map.insert(newRec);
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
    std::unique_lock<std::mutex> mapLock(memLock);
    m_Map.insert(newRec);
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
    std::unique_lock<std::mutex> mapLock(memLock);
    m_Map.insert(newRec);
#endif
    return res;
  }
  
  void MemoryManager::Free(void* ptr,bool array)
  {
    if(!s_FreeFn)
      s_FreeFn = &free;
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr")).c_str());
    std::unique_lock<std::mutex> mapLock(memLock);
    MemMap_by_ptr::iterator iter=m_Map.get<0>().find(ptr);
    //eXl_ASSERT_MSG(iter!=m_Map.end(),(String("Trying to free unmanaged memory")).c_str());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor for memory allocated in ")+iter->file+" at "+StringUtil::FromInt(iter->line)).c_str());
    m_Map.erase(iter);
#endif
    s_FreeFn(ptr);
  }

  void MemoryManager::Free(void* ptr,const char* file,unsigned int line,const char* iFun,bool array)
  {
    if(!s_FreeFn)
      s_FreeFn = &free;
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    std::unique_lock<std::mutex> mapLock(memLock);
    MemMap_by_ptr::iterator iter=m_Map.get<0>().find(ptr);
    //eXl_ASSERT_MSG(iter!=m_Map.end(),(String("Trying to free unmanaged memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor called for memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    m_Map.erase(iter);
#endif
    s_FreeFn(ptr);
  }

  void MemoryManager::Free_Ext(void* ptr,const char* file,unsigned int line,const char* iFun,bool array,void (*iFree)(void*))
  {
#ifdef DEBUG_ALLOC
    //eXl_ASSERT_MSG(ptr!=nullptr,(String("Trying to free nullptr in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    std::unique_lock<std::mutex> mapLock(memLock);
    MemMap_by_ptr::iterator iter=m_Map.get<0>().find(ptr);
    //eXl_ASSERT_MSG(iter!=m_Map.end(),(String("Trying to free unmanaged memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    //eXl_ASSERT_MSG((!array && iter->numElems==0) || (array && iter->numElems>0),(String("Wrong dtor called for memory in ")+file+" at "+StringUtil::FromInt(line)).c_str());
    m_Map.erase(iter);
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
    return;

    std::vector<char> buff(4096);
    char* buffer = &buff[0];
    size_t totalLeak=0;
    std::map<char const* ,unsigned int,CompStr> m_SetPos;
    {
      MemMap::iterator iter = m_Map.begin();
      MemMap::iterator iterEnd = m_Map.end();

      for(;iter!=iterEnd;iter++)
      {
        totalLeak+=iter->size;
        if(iter->file!=nullptr)
        {
          snprintf(buffer, buff.size(),("Leak in file %s in function %s at line %i of size %zi"),iter->file,iter->func,iter->line,iter->size);
          std::map<char const* ,unsigned int,CompStr>::iterator iter = m_SetPos.find(buffer);
          if(iter == m_SetPos.end())
          {
            size_t strLen = strlen(buffer);
            char* strCopy = new char[strLen + 1];
            memcpy(strCopy,buffer, strLen + 1);
            strCopy[strLen] = 0;
            std::pair<std::map<char const* ,unsigned int,CompStr>::iterator,bool> res = m_SetPos.insert(std::make_pair(strCopy,0));
            iter = res.first;
          }
          iter->second++;
        }
        else
        {
          //sprintf(buffer,"Leak at %p of size %i",iter->ptr,iter->size);
          LOG_INFO<<"Leak at "<<iter->ptr<<" of size "<<iter->size<<"\n";
        } 
      }
    }
    std::map<char const* ,unsigned int,CompStr>::iterator iter = m_SetPos.begin();
    std::map<char const* ,unsigned int,CompStr>::iterator iterEnd = m_SetPos.end();
    for(;iter!=iterEnd;iter++)
    {
      //totalLeak+=iter->size;
      LOG_INFO<<iter->first << " X "<< iter->second<<"\n";
    }
    LOG_INFO<<"Total memory leaks : "<<totalLeak<<"\n";
  }
  
  unsigned int MemoryManager::GetNum(void* iPtr,size_t& oStride,const char* file,unsigned int line,const char* iFun)
  {
    std::unique_lock<std::mutex> mapLock(memLock); 
    MemMap_by_ptr::iterator iter=m_Map.get<0>().find(iPtr);
    //eXl_ASSERT_MSG(iter!=m_Map.end(),(String("Trying to free unmanaged memory in ")+StringUtil::FromASCII(file)+" at "+StringUtil::FromInt(line)).c_str());
    //eXl_ASSERT_MSG(iter->numElems>0,(String("Wrong dtor called memory in ")+StringUtil::FromASCII(file)+" at "+StringUtil::FromInt(line)).c_str());
    oStride = iter->size/iter->numElems;
    return iter->numElems;
  }

}
