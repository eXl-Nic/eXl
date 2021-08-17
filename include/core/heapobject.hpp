/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <core/coredef.hpp>

namespace eXl
{
  /**
     Base class of every heap-allocated object. Compatible with the memory manager.
  **********************************************************************/
  class HeapObject
  {
#if defined(_DEBUG) && defined(TRACE_LEAKS)
  public:
    inline void* operator new(size_t size)
    {
      return MemoryManager::Allocate(size,false);
    }
    inline void* operator new(size_t size,const char* file,unsigned int line,const char* iFun)
    {
      return MemoryManager::Allocate(size,file,line,iFun,false);
    }
    
    inline void* operator new(size_t size,void* ptr)
    {
      return ptr;
    }
    
    inline void operator delete(void* ptr)
    {
      MemoryManager::Free(ptr,false);
    }
    inline void operator delete(void* ptr, const char* file,unsigned int line,const char* iFun)
    {
      MemoryManager::Free(ptr,false);
    }
    
    inline void operator delete(void* ptr,void*)
    {
      MemoryManager::Free(ptr,false);
    }
    
    void* operator new[](size_t size){
      eXl_ASSERT_MSG(false,"Do not use");
    }
    
    void operator delete[](void* ptr){
      eXl_ASSERT_MSG(false,"Do not use");
    }
    
  protected:
    explicit HeapObject(){}
  private:
    
#endif
  };
}
