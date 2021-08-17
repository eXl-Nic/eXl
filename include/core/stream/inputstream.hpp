#pragma once

#include <core/corelibexp.hpp>
#include <core/heapobject.hpp>
#include <core/containers.hpp>

#include <istream>
#include <streambuf>

namespace eXl
{
  class EXL_CORE_API InputStream : public HeapObject
  {
  public:
    virtual size_t Read(size_t iOffset, size_t iSize, void* oData) = 0;
    virtual size_t GetSize() = 0;
    virtual ~InputStream() {}
  protected:
    InputStream();
  private:
    InputStream& operator=(const InputStream&);
    InputStream(const InputStream&);
  };

  class InputStream_istream : public std::istream
  {
    class InputStream_streambuf : public std::streambuf
    {
    public:
      EXL_CORE_API InputStream_streambuf(InputStream* iStream, size_t iBuffSize = 256, size_t iPutBack = 8);
    private:
      int_type underflow();
      std::streampos seekpos(std::streampos sp, std::ios_base::openmode which);
      std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
      InputStream* m_Stream;
      size_t m_ReadOffset;
      size_t m_Size;
      const std::size_t m_PutBack;
      Vector<char> m_Buffer;
    };
    unsigned char m_StreamBuff[sizeof(InputStream_streambuf)];
  public:
    inline InputStream_istream(InputStream* iStream)
      :std::istream(new(m_StreamBuff) InputStream_streambuf(iStream)) {}
  };
}

#include <core/stream/textreader.hpp>

namespace eXl
{
  class EXL_CORE_API InputStreamTextReader : public TextReader
  {
  public:

    InputStreamTextReader(String const& iIdent, std::unique_ptr<InputStream> iStream);

    void reset() override;
    void clear() override;
    Char get() override;
    Char peek() override;
    size_t getPos() override;
    void setPos(size_t) override;

  protected:

    void EnsureBuffer(size_t iDesiredOffset);

    std::unique_ptr<InputStream> m_Stream;
    size_t m_CurOffset = 0;
    size_t m_BufferedOffset = -1;
    static constexpr size_t s_BufferSize = 256;
    Char m_Buffer[s_BufferSize];
  };
}