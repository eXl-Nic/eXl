/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/coredef.hpp>
#include <core/random.hpp>

namespace eXl
{
  /*
   * Inspired by the paper "Simple and Space-Efficient Minimal Perfect Hash Functions"
   */
  struct EXL_CORE_API MPHF_Data
  {
    uint32_t m_HashLen;
    uint32_t m_Mask;

    Vector<uint64_t> m_AssignmentTable;
    Vector<uint32_t> m_RankTable;

    Err Build(uint32_t const* iHashValues, uint32_t iNumKeys);
    void Clear();

    uint32_t CountBits(uint64_t iValue) const
    {
      // 0, 1, 2 mean the value has been assigned a key.
      // 3 means unassigned.
      // The following bit manipulation counts the number of pairs of bits not set to 3.
      uint64_t const maskEvenBits = 0x5555555555555555; //01010101.....
      uint64_t const maskedValue = ~((iValue & maskEvenBits) & ((iValue >> 1) & maskEvenBits));
#ifdef _MSC_VER
      return __popcnt64(maskedValue) - 32;
#else
      return __builtin_popcountll(maskedValue) - 32;
#endif
    }

    uint8_t GetLabel(uint32_t iValue) const
    {
      uint32_t bucket = iValue / 32;
      uint32_t bits = iValue % 32;

      return (m_AssignmentTable[bucket] >> (2 * bits)) & 3;
    }

    uint32_t Compute(uint32_t const* iHashValues) const
    {
      uint32_t ortho_hashes[] = { iHashValues[0] & m_Mask ,
      (iHashValues[1] & m_Mask) + (m_Mask + 1),
      (iHashValues[2] & m_Mask) + (m_Mask + 1) * 2 };

      // Get which hash of the 3 to select for the given key's hashes
      uint32_t hashIdx = (GetLabel(ortho_hashes[0]) + GetLabel(ortho_hashes[1]) + GetLabel(ortho_hashes[2])) % 3;
      uint32_t const& hash = ortho_hashes[hashIdx];

      uint32_t bucket = hash / 32;
      uint32_t bits = hash % 32;
      uint64_t maskValue = ~((1ull << 2 * bits) - 1);

      return m_RankTable[bucket] + CountBits(m_AssignmentTable[bucket] | maskValue);
    }
  };

  template <typename T, typename Derived>
  class MPHF_Base
  {
  public:
  
    void _Hash(T const& iValue, uint32_t(&oHashes)[3]) const
    {
      static_cast<Derived const*>(this)->Hash(iValue, oHashes);
    }

    uint32_t Compute(T const& iValue) const
    {
      if (m_Data.m_AssignmentTable.empty())
      {
        return 0;
      }
      uint32_t hashes[3];
      _Hash(iValue, hashes);
      return m_Data.Compute(hashes);
    }

    MPHF_Data const& GetData() const
    {
      return m_Data;
    }

    MPHF_Data& GetData()
    {
      return m_Data;
    }

  protected:
    template <typename Iter>
    Err _Build(Iter const& iBegin, Iter const& iEnd)
    {
      m_Data.Clear();
      uint32_t const numKeys = std::distance(iBegin, iEnd);
      if (numKeys == 0)
      {
        return Err::Success;
      }
      Vector<uint32_t> allHashes;
      allHashes.resize(3 * numKeys);
      uint32_t* hashesPtr = allHashes.data();
      for (Iter it = iBegin; it != iEnd; ++it, hashesPtr += 3)
      {
        _Hash(*it, reinterpret_cast<uint32_t (&)[3]>(*hashesPtr));
      }

      return m_Data.Build(allHashes.data(), numKeys);
    }
  private:
    MPHF_Data m_Data;
  };

  class EXL_CORE_API StringMPH : public MPHF_Base<KString, StringMPH>
  {
  public:

    template <typename Iter>
    Err Build(Iter const& iBegin, Iter const& iEnd)
    {
      std::unique_ptr<Random> rand(Random::CreateDefaultRNG(RandomInit()));
      UpdateSeeds(*rand);
      for (uint32_t i = 0; i < 64; ++i)
      {
        if (_Build(iBegin, iEnd))
        {
          return Err::Success;
        }
        UpdateSeeds(*rand);
      }

      eXl_FAIL_MSG_RET("Failed to build a perfect hashing function in 64 tries", Err::Failure);
    }

    void Hash(KString iStr, uint32_t(&oHashes)[3]) const;

    void GetSeeds(uint32_t (&oSeeds)[3]) const
    {
      std::copy(m_Seeds, ArrayEnd(m_Seeds), oSeeds);
    }

    void SetSeeds(uint32_t const(&oSeeds)[3])
    {
      std::copy(oSeeds, ArrayEnd(oSeeds), m_Seeds);
    }

  private:

    uint32_t RandomInit();
    void UpdateSeeds(Random& iRand);

    uint32_t m_Seeds[3];
  };
}