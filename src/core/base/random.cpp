/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/random.hpp>

#include <core/string.hpp>
#include <boost/random.hpp>
#include <boost/uuid/random_generator.hpp>
#include <sstream>

namespace eXl
{
  class BoostRNG : public Random
  {
  public:
    BoostRNG(unsigned int iSeed) : m_Gen(iSeed) {}

    unsigned int Generate() override
    {
      return m_Gen();
    }

    String Save() override
    {
      std::stringstream outStr;
      outStr << m_Gen;

      return StringUtil::FromASCII(outStr.str().c_str());
    }

    void Restore(String const& iStr)
    {
      std::stringstream inStr(StringUtil::ToASCII(iStr).c_str());
      inStr >> m_Gen;
    }

  protected:
    boost::random::mt19937 m_Gen;
  };

  Random* Random::CreateDefaultRNG(unsigned int iSeed)
  {
    return new BoostRNG(iSeed);
  }

  uint64_t Random::AllocateUUID()
  {
    static thread_local boost::uuids::random_generator s_UUIDGen;
    boost::uuids::uuid newuuid = s_UUIDGen();
    uint32_t* dwords = reinterpret_cast<uint32_t*>(newuuid.data);

    static_assert (sizeof(size_t) == sizeof(uint64_t), "");
    {
      uint64_t objId[2] = { dwords[0], dwords[2] };
      objId[0] <<= 32;
      objId[0] |= dwords[1];
      objId[1] <<= 32;
      objId[1] |= dwords[3];
      boost::hash_combine(objId[0], objId[1]);
      return objId[0];
    }
  }
}