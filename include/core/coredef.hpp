/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

// +++ disable warnings +++

#ifdef _MSC_VER

#pragma warning(disable : 4595)

// float to double
#pragma warning(disable : 4244 4305)

// size_t to uint32
#pragma warning(disable : 4267)

// forcing value to bool true or false
#pragma warning(disable : 4800)

// undefined template function during instantiation
#pragma warning(disable : 4661)

// --- disable warnings ---
// 
// +++ warnings as errors +++

// BGL does not like it...
// Returning reference to local variable or temporary.
//#pragma warning(error : 4172)

// Not all control path return a value
#pragma warning(error : 4715)
 
// --- warnings as errors ---
#endif

#include <core/corelibexp.hpp>
#include <core/memorymanager.hpp>
#include <core/string.hpp>

#include <cstdlib>
#include <cstddef>
#include <memory>
#include <optional>

#ifdef WIN32
#pragma comment(linker, "/nodefaultlib:libc.lib")
#pragma comment(linker, "/nodefaultlib:libcd.lib")
#endif

#include <core/corenew.hpp>

#define eXl_TO_STR_(s) #s
#define eXl_TO_STR(s) eXl_TO_STR_(s)
#define eXl_CONCAT(s,r) s##r
#define eXl_FORMAT(s, ...) (::eXl::StringUtil::Format(s, __VA_ARGS__).c_str())
#if !defined(_MSC_VER)
#define eXl_TEMPLATE_EXTERN extern
#else
#define eXl_TEMPLATE_EXTERN
#endif

namespace eXl
{

  class EXL_CORE_API Exception //: public std::exception
  {
  public:
    static const unsigned int AssertType;
    static const unsigned int UnexpectedType;
    static const unsigned int TestingFailureType;
    static const unsigned int BadCast;

    Exception(unsigned int type);
    Exception(unsigned int type,const String& iMsg);
    Exception(unsigned int type,const String& iMsg,const char* iFile,unsigned int line);
    ~Exception()throw(){}
    
    inline bool IsAssert()const
    {return (flags & AssertType) != 0;}
    inline bool IsUnexpected()const
    {return (flags & UnexpectedType) != 0;}
    inline bool IsTestFail()const
    {return (flags & TestingFailureType) != 0;}
    inline bool IsBadCast()const
    {return (flags & BadCast) != 0;}

    const char* what() const throw();

  protected:
    unsigned int flags;
    String msg;
    const String file;
    const unsigned int line;
  };

  struct Err
  {
    enum Code
    {
      Success,
      Failure,    //Operation failed, not fatal
      Error,      //Operation failed because of an error, should be coupled with an assertion
      Unexpected, //Unexpected error
      Undefined   //Operation not yet implemented
    };

    Err(Code err)
      : m_code(err)
    {

    }

    bool operator == (Code const iOtherCode)
    {
      return m_code == iOtherCode;
    }

    bool operator != (Code const iOtherCode)
    {
      return m_code != iOtherCode;
    }

    explicit operator bool()
    {
      return m_code == Success;
    }

    Code m_code;
  };
 
  EXL_CORE_API void InstallThreadHandler();
  EXL_CORE_API void AssertionError(const Char* expression, const Char* msg,const char* file, unsigned int line,bool repair);
  EXL_CORE_API void Unexpected(const Char* msg,const char* file, unsigned int line,bool hardware);
  EXL_CORE_API void Undefined(const Char* msg,const char* file, unsigned int line);

  inline bool TestAssertion(bool iExp, const Char* exp, const Char* msg, const char* file, unsigned int line, bool repair)
  {
    if(!iExp)
    {
      AssertionError(exp, msg, file, line, repair);
    }
    return iExp;
  }

  typedef unsigned int CompRes;
  const CompRes CompEqual     = 0;
  const CompRes CompDifferent = 1;
  const CompRes CompLesser    = 2;
  const CompRes CompGreater   = 4;
}


#define RETURN_SUCCESS do{return ::eXl::Err::Success;}while(false)

#define RETURN_FAILURE do{return ::eXl::Err::Failure;}while(false)

#define RETURN_ERROR do{return ::eXl::Err::Error;}while(false)

#define RETURN_ERROR_MSG(msg)                                           \
  do{::eXl::AssertionError(nullptr, msg, __FILE__, __LINE__, true);return ::eXl::Err::Error;}while(false)

#define RETURN_FATAL_ERROR_MSG(msg)                                     \
  do{::eXl::AssertionError(nullptr, msg, __FILE__, __LINE__, false);return ::eXl::Err::Error;}while(false)

#define RETURN_UNEXPECTED(msg)                                          \
  do{::eXl::Unexpected(nullptr, msg, __FILE__, __LINE__);return ::eXl::Err::Unexpected; }while(false)

/*#define RETURN_UNDEFINED_MSG(msg)					\
do{eXl::Undefined(msg,__FILE__,__LINE__);return RC_Undefined; }while(false)

#define RETURN_UNDEFINED \
do{eXl::Undefined(nullptr,__FILE__,__LINE__);return RC_Undefined; }while(false)*/

#define RETURN_IF_FAIL(exp)                     \
  do                                            \
  {                                             \
    ::eXl::Err exp_evalres = (exp);             \
    if(exp_evalres!=::eXl::Err::Success)        \
      return ;                                  \
  }while(false)

#define RETURNVALUE_IF_FAIL(exp,val)            \
  do                                            \
  {                                             \
    ::eXl::Err exp_evalres = (exp);             \
    if(exp_evalres!=::eXl::Err::Success)        \
      return val;                               \
  }while(false)

#define RETURNFAIL_IF_FAIL(exp) RETURNVALUE_IF_FAIL(exp,::eXl::Err::Failure)

#define TOSTRING(exp) #exp


#define eXl_ASSERT(exp)                                                 \
  do{                                                                   \
    if(!(exp)) ::eXl::AssertionError(#exp, nullptr,__FILE__,__LINE__,false); \
  }while(false)

#define eXl_FAIL_MSG(msg) \
do \
{ \
  ::eXl::AssertionError(nullptr , msg, __FILE__, __LINE__, false); \
}while(false) \

#define eXl_FAIL_MSG_RET(msg, retval) \
do \
{ \
  ::eXl::AssertionError(nullptr , msg, __FILE__, __LINE__, false); \
  return retval; \
}while(false) \

#define eXl_ASSERT_MSG(exp, msg)                                         \
  do{                                                                   \
    if(!(exp))::eXl::AssertionError(#exp, msg, __FILE__, __LINE__, false); \
  }while(false)

#define eXl_ASSERT_REPAIR_BEGIN(exp)                                    \
  if(!::eXl::TestAssertion(exp, #exp, "Assertion failed", __FILE__, __LINE__, true))

#define eXl_ASSERT_REPAIR_RET(exp, ret)                                 \
  if(!::eXl::TestAssertion(exp, #exp, "Assertion failed", __FILE__, __LINE__, true)) \
  {                                                                     \
    return ret; \
  }

#define eXl_ASSERT_MSG_REPAIR_RET(exp, msg, ret)                                 \
  if(!::eXl::TestAssertion(exp, #exp, msg, __FILE__, __LINE__, true)) \
  {                                                                     \
    return ret; \
  }

#define eXl_ASSERT_MSG_REPAIR_BEGIN(exp,msg)                            \
  if(!::eXl::TestAssertion(exp, #exp, msg,__FILE__,__LINE__,true))                                              

#ifdef MSVC_COMPILER
#define FUN_STR __FUNCTION__
#else
#define FUN_STR __func__
#endif

#define eXl_ALLOC(bytes) (::eXl::MemoryManager::Allocate(bytes,__FILE__,__LINE__,FUN_STR))
#define eXl_FREE(ptr) ::eXl::MemoryManager::Free(ptr,false);

//Memory manager compliant allocator calls.
#if defined(_DEBUG) && defined(TRACE_LEAKS)

#define eXl_NEW new(__FILE__,__LINE__,FUN_STR)
#define eXl_NEW_DATA(Type) new(::eXl::MemoryManager::Allocate(sizeof(Type),__FILE__,__LINE__,FUN_STR)) Type

#define eXl_NEW_DATA_ARRAY(Type,Num) (::eXl::MemoryManager::ConstructN<Type>(::eXl::MemoryManager::Allocate((Num)*sizeof(Type),__FILE__,__LINE__,FUN_STR,Num),Num))
#define eXl_NEW_ARRAY(Class,Num) eXl_NEW_DATA_ARRAY(Class,Num)

#define eXl_DELETE delete
#define eXl_DELETE_DATA(Type,Obj) do{if(Obj)Obj->~Type();::eXl::MemoryManager::Free(Obj,__FILE__,__LINE__,FUN_STR,false);}while(false)

#define eXl_DELETE_DATA_ARRAY(Type,Obj) FreeArrayT(Type,Obj,__FILE__,__LINE__,FUN_STR)
#define eXl_DELETE_ARRAY(Class,Obj) eXl_DELETE_DATA_ARRAY(Class,Obj)

#else

#define eXl_NEW new
#define eXl_NEW_DATA(Type) new Type

#define eXl_NEW_DATA_ARRAY(Type,Num) (new Type[Num])
#define eXl_NEW_ARRAY(Class,Num) (new Class[Num])

#define eXl_DELETE delete
#define eXl_DELETE_DATA(Type,Obj) (delete Obj)

#define eXl_DELETE_ARRAY(Class,Obj) (delete [] Obj)
#define eXl_DELETE_DATA_ARRAY(Type,Obj) (delete [] Obj)

#endif

namespace eXl
{
  template<class T>
  class ManagedDelete
  {
  public:
    void operator()(T* iPtr){eXl_DELETE iPtr;}
  };

  template<class T>
  class ManagedDeleteData
  {
  public:
    void operator()(T* iPtr){eXl_DELETE_DATA(T,iPtr);}
  };

  template<typename T, size_t Size>
  constexpr size_t ArrayLength(T(&)[Size]) { return Size; }

  template<typename T, size_t Size>
  constexpr T* ArrayEnd(T(&iArray)[Size]) { return iArray + ArrayLength(iArray); }

  template <typename T>
  using UniquePtr = std::unique_ptr<T>;

  template <typename T>
  using Optional = std::optional<T>;
}

#include <core/intrusiveptr.hpp>
#include <core/containers.hpp>

