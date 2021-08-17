#include <core/stream/outputstream.hpp>

namespace eXl
{
  OutputStream::OutputStream() = default;

  OutputStream_ostream::OutputStream_streambuf::OutputStream_streambuf(OutputStream* iStream, size_t iBuffSize)
    :m_Stream(iStream)
    , m_Buffer(iBuffSize + 1)
    , m_WriteOffset(0)
  {
    char *base = &m_Buffer.front();
    setp(base, base + m_Buffer.size() - 1);
  }

  std::streambuf::int_type OutputStream_ostream::OutputStream_streambuf::overflow(int ch)
  {
    if (ch != traits_type::eof())
    {
      eXl_ASSERT(std::less_equal<char *>()(pptr(), epptr()));
      *pptr() = ch;
      pbump(1);
      if (sync() == 0)
        return ch;
    }

    return traits_type::eof();
  }

  int OutputStream_ostream::OutputStream_streambuf::sync()
  {
    std::ptrdiff_t n = pptr() - pbase();

    pbump(-n);

    size_t writtenSize = m_Stream->Write(n, pbase());
    m_WriteOffset += writtenSize;
    return writtenSize == n ? 0 : -1;
  }
}