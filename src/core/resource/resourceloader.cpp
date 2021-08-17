#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/stream/jsonstreamer.hpp>
#include <core/stream/jsonunstreamer.hpp>
#include <core/stream/writer.hpp>
#include <core/stream/textreader.hpp>
#include <sstream>

namespace eXl
{
  Err ResourceLoader::Save(Resource* iRsc, Writer& iWriter) const
  {
    if (auto stdWriter = StdOutWriter::DynamicCast(&iWriter))
    {
      std::stringstream sstream;
      JSONStreamer streamer(&sstream);
      Err res = streamer.Begin();
      if (res)
      {
        res = iRsc->Stream(streamer);
      }
      if (res)
      {
        res = streamer.End();
      }

      if (!res)
      {
        LOG_ERROR << "Error saving asset " << iRsc->GetName() << "\n";
      }

      {
        String str(sstream.str().c_str()); // <= replace with view when available.
        StringViewReader reader(iRsc->GetName(), str.data(), str.data() + str.size());
        JSONUnstreamer unstreamer(&reader);
        if (!unstreamer.Begin())
        {
          LOG_ERROR << "Error while checking that temp saved resource can be read : " << iRsc->GetName() << "\n";
          return Err::Failure;
        }
      }

      {
        stdWriter->m_Stream << sstream.str();
      }

      return res;
    }
    return Err::Failure;
  }

  Resource* ResourceLoader::Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Reader& iReader) const
  {
    if (auto textReader = TextReader::DynamicCast(&iReader))
    {
      Resource* rsc = Create_Impl(iMetaData);
      JSONUnstreamer unstreamer(textReader);
      Err res = unstreamer.Begin();
      if (res)
      {
        res = rsc->Unstream(unstreamer);
      }
      if (res)
      {
        unstreamer.End();
      }
      if (!res)
      {
        eXl_DELETE rsc;
        return nullptr;
      }
      return rsc;
    }

    return nullptr;
  }
}
