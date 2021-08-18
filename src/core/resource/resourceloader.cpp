/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
