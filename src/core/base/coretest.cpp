/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <map>
#include <core/coretest.hpp>
#include <cassert>

#ifdef MSVC_COMPILER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


typedef void (*_se_translator_function)(unsigned int, struct _EXCEPTION_POINTERS* );

static const eXl::Char* ErrorTrace[] = 
{" EXCEPTION_ACCESS_VIOLATION : Memory access violation",
 " EXCEPTION_ARRAY_BOUNDS_EXCEEDED : Array bounds exceeded",
 " EXCEPTION_BREAKPOINT : Breakpoint (?)",
 " EXCEPTION_DATATYPE_MISALIGNMENT : Alignement issue",
 " EXCEPTION_FLT_DENORMAL_OPERAND : Float denormal (don't care)",
 " EXCEPTION_FLT_DIVIDE_BY_ZERO : Float division by zero",
 " EXCEPTION_FLT_INEXACT_RESULT : Float inexact result (don't care)",
 " EXCEPTION_FLT_INVALID_OPERATION : Float invalid operation (don't care)",
 " EXCEPTION_FLT_OVERFLOW : Float overflow (don't care)",
 " EXCEPTION_FLT_STACK_CHECK : Float op caused stack overflow/underflow",
 " EXCEPTION_FLT_UNDERFLOW : Float underflow (don't care)",
 " EXCEPTION_GUARD_PAGE : Reached PAGE_GUARD in memory",
 " EXCEPTION_ILLEGAL_INSTRUCTION : Illegal instruction",
 " EXCEPTION_IN_PAGE_ERROR : Page access error",
 " EXCEPTION_INT_DIVIDE_BY_ZERO : Integer division by zero",
 " EXCEPTION_INT_OVERFLOW : Integer overflow (don't care)",
 " EXCEPTION_INVALID_DISPOSITION : Invalid disposition (Should NEVER happen according to msdn)",
 " EXCEPTION_INVALID_HANDLE : Invalid window handle",
 " EXCEPTION_NONCONTINUABLE_EXCEPTION : Thread attempted to resume execution after throwing a non-continuable exception",
 " EXCEPTION_PRIV_INSTRUCTION : Instruction not allowed in current computer mode",
 " EXCEPTION_SINGLE_STEP : A single instruction was executed (don't care)",
 " EXCEPTION_STACK_OVERFLOW : Stack overflow",
 " STATUS_UNWIND_CONSOLIDATE : Frame consolidation executed (?)",
 " Unknown error code"
};

class SE_Exception : public eXl::Exception
{

public:
  SE_Exception():eXl::Exception(eXl::Exception::UnexpectedType)
  {
    
  }
  SE_Exception( unsigned int n ) :eXl::Exception(eXl::Exception::UnexpectedType), nSE( n ) 
  {
    switch(nSE)
    {
    case EXCEPTION_ACCESS_VIOLATION :
      msg+=ErrorTrace[0];
      break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      msg+= ErrorTrace[1];
      break;
    case EXCEPTION_BREAKPOINT:
      msg+= ErrorTrace[2];
      break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      msg+= ErrorTrace[3];
      break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      msg+= ErrorTrace[4];
      break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      msg+= ErrorTrace[5];
      break;
    case EXCEPTION_FLT_INEXACT_RESULT:
      msg+= ErrorTrace[6];
      break;
    case EXCEPTION_FLT_INVALID_OPERATION:
      msg+= ErrorTrace[7];
      break;
    case EXCEPTION_FLT_OVERFLOW:
      msg+= ErrorTrace[8];
      break;
    case EXCEPTION_FLT_STACK_CHECK:
      msg+= ErrorTrace[9];
      break;
    case EXCEPTION_FLT_UNDERFLOW:
      msg+= ErrorTrace[10];
      break;
    case EXCEPTION_GUARD_PAGE:
      msg+= ErrorTrace[11];
      break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      msg+= ErrorTrace[12];
      break;
    case EXCEPTION_IN_PAGE_ERROR:
      msg+= ErrorTrace[13];
      break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      msg+= ErrorTrace[14];
      break;
    case EXCEPTION_INT_OVERFLOW:
      msg+= ErrorTrace[15];
      break;
    case EXCEPTION_INVALID_DISPOSITION:
      msg+= ErrorTrace[16];
      break;
    case EXCEPTION_INVALID_HANDLE:
      msg+= ErrorTrace[17];
      break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      msg+= ErrorTrace[18];
      break;
    case EXCEPTION_PRIV_INSTRUCTION:
      msg+= ErrorTrace[19];
      break;
    case EXCEPTION_SINGLE_STEP:
      msg+= ErrorTrace[20];
      break;
    case EXCEPTION_STACK_OVERFLOW:
      msg+= ErrorTrace[21];
      break;
      /*  case STATUS_UNWIND_CONSOLIDATE:
          return ErrorTrace[22];
          break;*/
    default:
      msg+= ErrorTrace[23];
      break;
    }
  }
  ~SE_Exception() {}

  unsigned int getSeNumber() { return nSE; }
    
private:
  unsigned int nSE;  
};




#endif

#include <core/log.hpp>

namespace eXl
{

  TestUnit::TestUnit(const String& iName):m_Name(iName),m_Msg(""),status(0),numTest(0)
  {
    
  }

  void TestUnit::AddTest()
  {
    numTest++;
  }

  void TestUnit::Failed(const String& iMsg)
  {
    m_Msg = iMsg;
    status = 1;
  }

  String TestUnit::Report()const
  {
    String OMsg;
    if(status == 0)
      OMsg = String("Test unit ") + m_Name + " successfully completed " + StringUtil::FromInt(numTest) + " tests. \n";
    else
    {
      OMsg = String("Test unit ") + m_Name + " failed at test " + StringUtil::FromInt(numTest) + " because : " + m_Msg;
    }
    return OMsg;
  }

  const unsigned int Exception::AssertType=1;
  const unsigned int Exception::UnexpectedType=2;
  const unsigned int Exception::TestingFailureType=3;
  const unsigned int Exception::BadCast=4;
  
  Exception::Exception(unsigned int type):flags(type),msg(""),file(""),line(0)
  {
    if(IsAssert())
      msg = "Assertion error";
    else if(IsUnexpected())
      msg = "Unexpected Err";
    else if(IsBadCast())
      msg = "Bad Dynamic Cast";
    else
      msg = "Testing failure";
  }  
  
  Exception::Exception(unsigned int type,const String& iMsg):flags(type),msg(iMsg),file(""),line(0)
  {
    if(msg=="")
    {
      if(IsAssert())
        msg = "Assertion error";
      else if(IsUnexpected())
        msg = "Unexpected Err";
      else if(IsBadCast())
        msg = "Bad Dynamic Cast";
      else
        msg = "Testing failure";
    }
  }  

  Exception::Exception(unsigned int type,const String& iMsg,const char * iFile,unsigned int iLine):flags(type),msg(iMsg),file(StringUtil::FromASCII(iFile)),line(iLine)
  {
    if(msg=="")
    {
      if(IsAssert())
        msg = "Assertion error";
      else if(IsUnexpected())
        msg = "Unexpected Err";
      else if(IsBadCast())
        msg = "Bad Dynamic Cast";
      else
        msg = "Testing failure";
  
      if(file!="")
        msg = msg + " in " + file + " at " + StringUtil::FromInt(line);
    }
    else
    {
      if(file!="")
        msg = msg + " in " + file + " at " + StringUtil::FromInt(line);
    }
  }  

  const char* Exception::what() const throw()
  {
#ifdef EXL_CHAR_TYPE_IS_CHAR
    return msg.c_str();
#else
  #ifdef EXL_CHAR_TYPE_IS_WCHAR
    return "";
  #else
  #error
  #endif
#endif
  }


  static thread_local bool reportingDone;
  static thread_local unsigned int currentErroErrCode;

  static unsigned int errorFlags = 0;

#ifdef MSVC_COMPILER
  void trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
  {
    char adressBuffer[64];
    sprintf(adressBuffer,"0x%p",pExp->ExceptionRecord->ExceptionAddress);

    Unexpected(adressBuffer,__FILE__,__LINE__,true);
  }
#endif
  
  void ClearException()
  {
    currentErroErrCode = 0;
    reportingDone = 0;
  }

  void SetErrorHandling(unsigned int iLevel)
  {
    errorFlags=iLevel;
  }

  void InstallThreadHandler()
  {
    currentErroErrCode=0;
    reportingDone=false;

#ifdef MSVC_COMPILER
    //_set_se_translator(trans_func);
#endif
  }

  void DebugBreak()
  {
#ifdef MSVC_COMPILER
    __debugbreak();
#else
    assert(false);
#endif
  }

  void AssertionError(const Char* iExpression, const Char* iMsg, const char* file, unsigned int line,bool repair)
  {
    String msg;
    if (iExpression != nullptr)
    {
      if (iMsg != nullptr)
      {
        msg = String("Test : ") + iExpression + " -> " + iMsg;
      }
      else
      {
        msg = String("Test : ") + iExpression + " failed";
      }
    }
    else if (iMsg != nullptr)
    {
      msg = String(iMsg);
    }
    else
    {
      msg = "<unknown failure>";
    }

    if(!(repair && (errorFlags & ALLOW_ASSERT_REPAIR)))
    {
#ifdef MSVC_COMPILER
      if(!(errorFlags & NOASK_ASSERT))
      {
        int res =0;
      
#ifdef _DEBUG
        String errStr = "Assertion error in " + StringUtil::FromASCII(file)+ " at " + StringUtil::FromInt(line) + " : \n " + msg;
#ifdef EXL_CHAR_TYPE_IS_CHAR
        res = MessageBoxA(nullptr,errStr.c_str(),"Assertion Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#ifdef EXL_CHAR_TYPE_IS_WCHAR
        res = MessageBoxW(nullptr,errStr.c_str(),"Assertion Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#error
#endif
#endif
        if(res==IDRETRY)
        {
          DebugBreak();
          return;
        }
        if(res==IDABORT)
        {
          if(errorFlags & THROW_ASSERT)
            throw Exception(Exception::AssertType);
          else
            abort();
        }
#else
        String errStr = ("Assertion error in " + StringUtil::FromASCII(file)+ " at " + StringUtil::FromInt(line) + " : \n " + msg).c_str();

#ifdef EXL_CHAR_TYPE_IS_CHAR
        res = MessageBoxA(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
  #ifdef EXL_CHAR_TYPE_IS_WCHAR
          res = MessageBoxW(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
  #else
  #error
  #endif
#endif
        if(res==IDCANCEL)
        {
          if(errorFlags & THROW_ASSERT)
            throw Exception(Exception::AssertType);
          else
            abort();
        }
#endif
      
        if(res!=IDIGNORE && !(res==IDRETRY))
          throw Exception(Exception::AssertType);
      }
      else
#endif
      {
        if(!((errorFlags& SILENT_ASSERT)==SILENT_ASSERT))
        {
          if(iMsg!=nullptr)
          {
            RAW_LOG_ERROR<<"Assertion Err in "<<file<<" at "<<line<<" : "<<iMsg<<"\n";
          }
          else
          {
            RAW_LOG_ERROR<<"Assertion Err in "<<file<<" at "<<line<<"\n";
          }
        }
        if(errorFlags & THROW_ASSERT)
        {
          if(iMsg!=nullptr)
          {
            throw Exception(Exception::AssertType,iMsg,file,line);
          }
          else
          {
            throw Exception(Exception::AssertType,"",file,line);
          }
        }
        if(errorFlags & CRASH_ASSERT)
        {
#ifdef _DEBUG
          DebugBreak();return;
#else
          abort();
#endif
        }
        DebugBreak();
      }
    }
    else
    {
      if(!((errorFlags& SILENT_ASSERT)==SILENT_ASSERT))
      {
        if(iMsg!=nullptr)
        {
          RAW_LOG_WARNING<<"Assertion Err in "<<file<<" at "<<line<<" : "<<iMsg<<"\n";
        }
        else
        {
          RAW_LOG_WARNING<<"Assertion Err in "<<file<<" at "<<line<<"\n";
        }
      }
    }
  }
  
  void Unexpected(const Char* iMsg,const char* file, unsigned int line,bool hardware)
  {
    if(hardware)
    {
      int res =0;
#ifdef MSVC_COMPILER
      SE_Exception theEx(currentErroErrCode);
#else
      Exception theEx(Exception::UnexpectedType);
#endif
      String msg = StringUtil::FromASCII(theEx.what());
#ifdef MSVC_COMPILER
      if(!(errorFlags & NOASK_UNEX))
      {
        if(!reportingDone)
        {
#ifdef _DEBUG
          String errMsg = String("Unexpected error at ") + iMsg + " : \n " + msg;

#ifdef EXL_CHAR_TYPE_IS_CHAR
          res = MessageBoxA(nullptr,errMsg.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#ifdef EXL_CHAR_TYPE_IS_WCHAR
          res = MessageBoxW(nullptr,errMsg.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#error
#endif
#endif
          
          if(res==IDRETRY)
            DebugBreak();
          else if(res==IDABORT)
          {
            if(errorFlags & THROW_UNEX)
              throw theEx;
            else
              abort();
          }
#else
          String errMsg(String("Unexpected error at ") + iMsg + " : \n " + msg);
#ifdef EXL_CHAR_TYPE_IS_CHAR
          res = MessageBoxA(nullptr,errMsg.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#ifdef EXL_CHAR_TYPE_IS_WCHAR
          res = MessageBoxW(nullptr,errMsg.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#error
#endif
#endif
          if(res==IDCANCEL)
          {
            if(errorFlags & THROW_UNEX)
              throw theEx;
            else
              abort();
          }
#endif
          reportingDone=true;
        }
      
        if(res!=IDIGNORE && !(res==IDRETRY))
          throw theEx;
      }
      else
#endif
      {
        if(!((errorFlags& SILENT_UNEX)==SILENT_UNEX))
        {

          if(!reportingDone)
          {
            RAW_LOG_ERROR<<"Unexpected Err at "<<iMsg<<" : "<<msg<<"\n";
            reportingDone=true;
          }
        }
        if(errorFlags & THROW_UNEX)
        {
          throw theEx;
        }
        if(errorFlags & CRASH_UNEX)
        {
          abort();
        }
      }
    }
    else
    {
#ifdef MSVC_COMPILER
      if(!(errorFlags & NOASK_UNEX))
      {
        int res =0;
      
        String msg;
        if(iMsg!=nullptr)
          msg=String(iMsg);
      
#ifdef _DEBUG
        String errStr = String("Unexpected error in ") + StringUtil::FromASCII(file)+ " at " + StringUtil::FromInt(line) + " : \n " + msg;
#ifdef EXL_CHAR_TYPE_IS_CHAR
        res = MessageBoxA(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#ifdef EXL_CHAR_TYPE_IS_WCHAR
        res = MessageBoxW(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#error
#endif
#endif  
        if(res==IDRETRY)
        {DebugBreak();return;}
        if(res==IDABORT)
          abort();
#else
        String errStr = String("Unexpected error in ") + StringUtil::FromASCII(file)+ " at " + StringUtil::FromInt(line) + " : \n " + msg;
#ifdef EXL_CHAR_TYPE_IS_CHAR
        res = MessageBoxA(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#ifdef EXL_CHAR_TYPE_IS_WCHAR
        res = MessageBoxW(nullptr,errStr.c_str(),"Unexpected Err.",MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL);
#else
#error
#endif
#endif
        if(res==IDCANCEL)
          abort();
#endif
      
        if(res!=IDIGNORE)
          throw Exception(Exception::UnexpectedType);
      }
      else
#endif
      {
        if(!((errorFlags& SILENT_UNEX)==SILENT_UNEX))
        {
          if(iMsg!=nullptr)
          {
            RAW_LOG_ERROR<<"Assertion Err in "<<file<<" at "<<line<<" : "<<iMsg<<"\n";
          }
          else
          {
            RAW_LOG_ERROR<<"Assertion Err in "<<file<<" at "<<line<<"\n";
          }
        }
        if(errorFlags & THROW_UNEX)
        {
          throw Exception(Exception::UnexpectedType);
        }
        if(errorFlags & CRASH_UNEX)
        {
          abort();
        }
      
      }
    }
  }
  
  void Undefined(const char* iMsg,const char* file, unsigned int line)
  {
    if(iMsg!=nullptr)
    {
      RAW_LOG_ERROR<<"Undefined call in "<<file<<" at "<<line<<" : "<<iMsg<<"\n";
    }
    else
    {
      RAW_LOG_ERROR<<"Undefined call in "<<file<<" at "<<line<<"\n";
    }
      
#ifdef _DEBUG
#ifdef MSVC_COMPILER
    DebugBreak();
#endif
#endif
  }
    
}
