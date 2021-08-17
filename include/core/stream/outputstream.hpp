#pragma once

#include <core/corelibexp.hpp>
#include <core/heapobject.hpp>
#include <core/containers.hpp>

#include <ostream>
#include <streambuf>

namespace eXl
{
  class OutputStream : public HeapObject
  {
  public:
    virtual size_t Write(size_t iSize, void* oData) = 0;
    virtual size_t GetWrittenSize() = 0;
    virtual ~OutputStream() {}
    //inline StorageInfo const* GetInfo() const { return m_Info.get(); }
  protected:
    OutputStream();
    //OutputStream(StorageInfo const* iInfo) :m_Info(iInfo) {}
    //IntrusivePtr<StorageInfo const> m_Info;
  private:
    OutputStream& operator=(const OutputStream&);
    OutputStream(const OutputStream&);
  };

  class OutputStream_ostream : public std::ostream
  {
    class OutputStream_streambuf : public std::streambuf
    {
    public:
      EXL_CORE_API OutputStream_streambuf(OutputStream* iStream, size_t iBuffSize = 256);
    private:
      int_type overflow(int ch);
      int sync();
      OutputStream* m_Stream;
      size_t m_WriteOffset;
      Vector<char> m_Buffer;
    };
    unsigned char m_StreamBuff[sizeof(OutputStream_streambuf)];
  public:
    inline OutputStream_ostream(OutputStream* iStream)
      :std::ostream(new(m_StreamBuff) OutputStream_streambuf(iStream)) {}
    inline ~OutputStream_ostream()
    {
      flush();
    }
  };
}