#include <core/stream/inputstream.hpp>

namespace eXl
{
  InputStream::InputStream () = default;

  InputStream_istream::InputStream_streambuf::InputStream_streambuf(InputStream* iStream, size_t iBuffSize, size_t iPutBack)
    : m_Stream(iStream)
    , m_PutBack(iPutBack)
    , m_Buffer(iBuffSize)
    , m_ReadOffset(0)
    , m_Size(iStream->GetSize())
  {
    char *end = &m_Buffer.front() + m_Buffer.size();
    setg(end, end, end);
  }

  std::streampos InputStream_istream::InputStream_streambuf::seekpos(std::streampos sp, std::ios_base::openmode which)
  {
    if (which & std::ios_base::in)
    {
      int posInFile = static_cast<int>(m_ReadOffset - (egptr() - gptr()));
      int dist = static_cast<int>(sp) - posInFile;
      if ((gptr() + dist) < eback() || (gptr() + dist) >= egptr())
      {
        m_ReadOffset = sp > m_Size ? std::streampos(m_Size) : sp;
        char *end = &m_Buffer.front() + m_Buffer.size();
        setg(end, end, end);
      }
      else
      {
        gbump(dist);
      }
      return sp;
    }
    return -1;
  }

  std::streampos InputStream_istream::InputStream_streambuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which)
  {
    if (which & std::ios_base::in)
    {
      unsigned int pos;
      switch (way)
      {
      case std::ios_base::beg:
        pos = off;
        break;
      case std::ios_base::cur:
        //m_ReadOffset == egptr()
        pos = (m_ReadOffset - (egptr() - gptr())) + off;
        if (off == 0)
          return pos;
        break;
      case std::ios_base::end:
        pos = (m_Size + off) > m_Size ? m_Size : m_Size + off;
        break;
      }
      seekpos(pos, which);
      return pos;
    }
    return -1;
  }

  std::streambuf::int_type InputStream_istream::InputStream_streambuf::underflow()
  {
    if (gptr() < egptr()) // buffer not exhausted
      return traits_type::to_int_type(*gptr());

    char *base = &m_Buffer.front();
    char *start = base;

    if (m_Size == m_ReadOffset)
      return traits_type::eof();

    if (eback() == base) // true when this isn't the first fill
    {
      // Make arrangements for putback characters
      std::memmove(base, egptr() - m_PutBack, m_PutBack);
      start += m_PutBack;
    }

    // start is now the start of the buffer, proper.
    // Read from fptr_ in to the provided buffer

    size_t toRead = (m_Buffer.size() - (start - base));
    if (m_Size - m_ReadOffset < toRead)
    {
      toRead = m_Size - m_ReadOffset;
    }

    if (toRead == 0)
      return traits_type::eof();

    //size_t n = std::fread(start, 1, buffer_.size() - (start - base), fptr_);
    size_t read = m_Stream->Read(m_ReadOffset, toRead, start);
    if (read == 0)
      return traits_type::eof();

    m_ReadOffset += read;

    // Set buffer pointers
    setg(base, start, start + read);

    return traits_type::to_int_type(*gptr());
  }

  InputStreamTextReader::InputStreamTextReader(String const& iIdent, std::unique_ptr<InputStream> iStream)
    : TextReader(iIdent)
    , m_Stream(std::move(iStream))

  {
    reset();
  }
  
  void InputStreamTextReader::reset()
  {
    m_CurOffset = 0;
    m_BufferedOffset = -1;
    m_isGood = m_Stream && m_Stream->GetSize() > 0;
    m_IsEof = m_Stream->GetSize() == 0;
  }

  void InputStreamTextReader::clear()
  {
    m_isGood = m_Stream && m_Stream->GetSize() > 0;
  }

  void InputStreamTextReader::EnsureBuffer(size_t iDesiredOffset)
  {
    if (m_BufferedOffset <= iDesiredOffset && iDesiredOffset < m_BufferedOffset + 256)
    {
      return;
    }

    m_BufferedOffset = iDesiredOffset / s_BufferSize;
    m_BufferedOffset *= s_BufferSize;

    size_t remainingSize = m_Stream->GetSize() - m_BufferedOffset;
    size_t readSize = remainingSize < s_BufferSize ? remainingSize : s_BufferSize;

    m_Stream->Read(m_BufferedOffset, readSize, m_Buffer);
  }

  Char InputStreamTextReader::get()
  {
    if (!m_isGood || m_IsEof || m_CurOffset >= m_Stream->GetSize())
    {
      if (m_IsEof)
      {
        m_isGood = false;
      }
      return 0;
    }

    EnsureBuffer(m_CurOffset);

    Char ret = m_Buffer[m_CurOffset - m_BufferedOffset];
    ++m_CurOffset;

    if (m_CurOffset == m_Stream->GetSize())
    {
      m_IsEof = true;
    }

    return ret;
  }

  Char InputStreamTextReader::peek()
  {
    if (!m_isGood || m_IsEof || m_CurOffset >= m_Stream->GetSize())
    {
      if (m_IsEof)
      {
        m_isGood = false;
      }
      return 0;
    }

    EnsureBuffer(m_CurOffset);

    return m_Buffer[m_CurOffset - m_BufferedOffset];
  }

  size_t InputStreamTextReader::getPos()
  {
    return m_CurOffset;
  }

  void InputStreamTextReader::setPos(size_t iOffset)
  {
    if (m_Stream == nullptr || iOffset >= m_Stream->GetSize())
    {
      return ;
    }

    m_CurOffset = iOffset;
    m_IsEof = false;
  }
}