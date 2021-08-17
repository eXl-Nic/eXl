#pragma once

#ifndef EXL_DISABLE_GLOBAL_ALLOC

#if WIN32

inline void* operator new(size_t iSize)
{
  return eXl::MemoryManager::Allocate(iSize);
}

inline void operator delete(void* iPtr)
{
  return eXl::MemoryManager::Free(iPtr, false);
}
#else
EXL_CORE_API void* operator new(size_t iSize);
EXL_CORE_API void operator delete(void* iPtr) noexcept;
#endif

#endif