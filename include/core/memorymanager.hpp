/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/corelibexp.hpp>

#include <cstdlib>


/*#define ConstructN(Type,Num,iPtr)			\
  do{							\
  Type* temp = iPtr;					\
  for(unsigned int i=0;i<count;i++)new(temp++) T;	\
  }while(false)*/

#define FreeArrayT(Type,iPtr,File,Line,Fun)                             \
  do{                                                                   \
    unsigned char* tempPtr = (unsigned char*)iPtr;                      \
    size_t stride;                                                      \
  char buffer[64];													\
  /*sprintf(buffer,"%i",Line);*/											\
    eXl_ASSERT_MSG(tempPtr!=nullptr,(String("Trying to free nullptr in "))+File+" at "+String(buffer)).c_str(); \
    unsigned int num = MemoryManager::GetNum(iPtr,stride,File,Line,Fun); \
    for(unsigned int i=0;i<num;i++)                                     \
    {                                                                   \
      ((Type*)tempPtr)->~Type();                                        \
      tempPtr+=stride/sizeof(unsigned char);                            \
    }                                                                   \
    MemoryManager::Free(iPtr,File,Line,Fun,true);                       \
  }while(false); 
  
namespace eXl
{  
  /**
     Class registering every allocation done with eXl_ALLOC,eXl_NEW to trace memory leaks.
  **********************************************************************/
  class EXL_CORE_API MemoryManager
  {
  public:
    
    template<class T>
      static T* ConstructN(void* iPtr,unsigned int count)
    {
      T* temp = (T*)iPtr;
      for(unsigned int i=0;i<count;i++){
        new(temp)T;
        temp++;
      }
      return (T*)iPtr;
    }
    
    /*template<class T>
      static void FreeArray(T* iPtr)
      {
      T* tempPtr = iPtr;
      unsigned int num = GetNum(iPtr,sizeof(T));
      if(num==0)return;
      for(unsigned int i=0;i<num;i++)
      {
      tempPtr->~T();
      tempPtr++;
      }
      Free(iPtr);
      }*/

    static void SetAllocFn(void*(*AllocFn)(size_t));
    static void SetFreeFn(void(*FreeFn)(void*));
    
    static void* Allocate(size_t size,size_t numElem=0);
    
    static void* Allocate(size_t size,const char* file,unsigned int line,const char* iFun,size_t numElem=0);

    static void* Allocate_Ext(size_t size,const char* file,unsigned int line,const char* iFun,size_t numElem,void *(*iAlloc)(size_t , int ));
    
    static void Free(void* ptr,bool array);

    static void Free(void* ptr,const char* file,unsigned int line,const char* iFun,bool array);

    static void Free_Ext(void* ptr,const char* file,unsigned int line,const char* iFun,bool array,void (*iFree)(void*));
    
    static void ReportLeaks();
    
    static unsigned int GetNum(void* iPtr,size_t& oStride,const char* file,unsigned int line,const char* iFun);
    
  private:
    
  };
}
