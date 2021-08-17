/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const int tolog)
{
  LM.write(StringUtil::FromInt(tolog).c_str());
  return LM;
}

inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const unsigned int tolog)
{
  LM.write(StringUtil::FromInt(tolog).c_str());
  return LM;
}
#if defined(_WIN64) || defined (__LP64__)
inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const size_t tolog)
{
  LM.write(StringUtil::FromSizeT(tolog).c_str());
  return LM;
}
#endif

inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const void* tolog)
{
  LM.write(StringUtil::FromPtr(tolog).c_str());
  return LM;
}

inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const float tolog)
{
  LM.write(StringUtil::FromFloat(tolog).c_str());
  return LM;
}

//inline const Log_Manager::LogObject& operator <<(const Log_Manager::LogObject& LM,const WString& tolog)
//{
//  LM.write(tolog);
//  return LM;
//}
#ifdef EXL_SHARED_LIBRARY
inline const Log_Manager::LogObject& operator <<(const Log_Manager::LogObject& LM, const std::string& tolog)
{
  LM.write(tolog.c_str());
  return LM;
}
#endif

inline const Log_Manager::LogObject& operator <<(const Log_Manager::LogObject& LM,const AString& tolog)
{
  LM.write(tolog);
  return LM;
}

inline const Log_Manager::LogObject& operator <<(const Log_Manager::LogObject& LM, const KString& tolog)
{
  LM.write(tolog);
  return LM;
}

inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const char* tolog)
{
  LM.write(tolog);
  return LM;
}

//inline const Log_Manager::LogObject& operator << (const Log_Manager::LogObject& LM,const wchar_t* tolog)
//{
//  LM.write(tolog);
//  return LM;
//}

inline void Log_Manager::LogStream::write(WString const&  tolog)const
{
  if (enabled)
    Log_Manager::write(tolog, streamNum);
}
inline void Log_Manager::LogStream::write(AString const&  tolog)const
{
  if (enabled)
    Log_Manager::write(tolog, streamNum);
}
inline void Log_Manager::LogStream::write(const Char* tolog)const
{
  if (enabled)
    Log_Manager::write(tolog, streamNum);
}
inline Log_Manager::LogStream const& Log_Manager::LogStream::Prefix() const
{
  if (enabled)
  {
    Log_Manager::write(m_Prefix.data(), streamNum);
    //Log_Manager::Inst->write(Maintenant(),streamNum);
  }
  return *this;
}
