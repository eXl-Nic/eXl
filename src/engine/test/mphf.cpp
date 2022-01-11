
#include <gtest/gtest.h>
#include <core/coredef.hpp>
#include <core/random.hpp>
#include <core/clock.hpp>
#include <core/utils/mphf.hpp>
#include <core/name.hpp>

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif


using namespace eXl;

class StringMPH : public MPHF_Base<String, StringMPH>
{
public:

  template <typename Iter>
  void Build(Iter const& iBegin, Iter const& iEnd)
  {
    std::unique_ptr<Random> rand(Random::CreateDefaultRNG(__rdtsc()));
    m_Seed1 = rand->Generate();
    m_Seed2 = rand->Generate();
    m_Seed3 = rand->Generate();
    for (uint32_t i = 0; i < 64; ++i)
    {
      if (_Build(iBegin, iEnd))
      {
        return;
      }
      m_Seed1 = rand->Generate();
      m_Seed2 = rand->Generate();
      m_Seed3 = rand->Generate();
    }

    eXl_FAIL_MSG("Failed to build a perfect hashing function in 64 tries");
  }

  void Hash(String const& iStr, uint32_t (&oHashes)[3]) const
  {
    size_t value = boost::hash_value(iStr);

    size_t hash1 = m_Seed1;
    boost::hash_combine(hash1, value);

    size_t hash2 = m_Seed2;
    boost::hash_combine(hash2, value);

    size_t hash3 = m_Seed3;
    //boost::hash_combine(hash3, value);
    hash3 ^= std::hash<String>()(iStr);

    oHashes[0] = hash1;//(hash1 & 0xFFFF);
    oHashes[1] = hash2;//((hash1 >> 16) & 0xFFFF);
    oHashes[2] = hash3;//(hash2 & 0xFFFF);
  }
private:
  uint32_t m_Seed1;
  uint32_t m_Seed2;
  uint32_t m_Seed3;
};

class NameMPHF : public MPHF_Base<Name, NameMPHF>
{
public:
  template <typename Iter>
  void Build(Iter const& iBegin, Iter const& iEnd)
  {
    std::unique_ptr<Random> rand(Random::CreateDefaultRNG(__rdtsc()));
    m_Seed1 = rand->Generate();
    m_Seed2 = rand->Generate();
    m_Seed3 = rand->Generate();
    for (uint32_t i = 0; i < 64; ++i)
    {
      if (_Build(iBegin, iEnd))
      {
        return;
      }
      m_Seed1 = rand->Generate();
      m_Seed2 = rand->Generate();
      m_Seed3 = rand->Generate();
    }

    eXl_FAIL_MSG("Failed to build a perfect hashing function in 64 tries");
  }

  void Hash(Name const& iName, uint32_t(&oHashes)[3]) const
  {
    //size_t value = boost::hash_value(iName.get());
    //
    //size_t hash1 = m_Seed1;
    //boost::hash_combine(hash1, value);
    //
    //size_t hash2 = m_Seed2;
    //boost::hash_combine(hash2, value);
    //
    //size_t hash3 = m_Seed3;
    //boost::hash_combine(hash3, value);
    //
    //oHashes[0] = hash1;
    //oHashes[1] = hash2;
    //oHashes[2] = hash3;
    ptrdiff_t ptr = (ptrdiff_t)iName.get().c_str();
    ptr = ((ptr >> 16) ^ ptr) * 0x45d9f3b45d9f3b;
    ptr = ((ptr >> 16) ^ ptr) * 0x45d9f3b45d9f3b;
    ptr = (ptr >> 16) ^ ptr;
    
    oHashes[0] = ptr & (1 << 22) - 1;
    oHashes[1] = (ptr >> 21) & (1 << 22) - 1;
    oHashes[2] = (ptr >> 42) & (1 << 22) - 1;
  }
protected:
  uint32_t m_Seed1;
  uint32_t m_Seed2;
  uint32_t m_Seed3;
};

TEST(Core, MPHF)
{
  uint32_t const numKeys = 10000;

  std::unique_ptr<Random> rand(Random::CreateDefaultRNG(0));
  UnorderedSet<String> keysDict;
  Vector<String> keys;
  for (uint32_t i = 0; i < numKeys; ++i)
  {
    char buffer[16];
    do
    {
      for (uint32_t i = 0; i < 16; ++i)
      {
        buffer[i] = (rand->Generate() % 26) + 'a';
      }
      String newKey(buffer, buffer + 16);
      if (keysDict.insert(newKey).second)
      {
        keys.push_back(newKey);
        break;
      }
    } while (true);
  }

  uint32_t const numQueries = 25000000;
  if(1)
  {
    UnorderedMap<String, uint32_t> classicMap;
    for (auto const& s : keys)
    {
      classicMap.insert(std::make_pair(s, classicMap.size()));
    }

    Clock timer;
    timer.GetTime();

    for (uint32_t i = 0; i < numQueries; ++i)
    {
      classicMap.find(keys[rand->Generate() % keys.size()]);
    }

    printf("Time classic : %f\n", timer.GetTime());

    StringMPH testStruct;
    testStruct.Build(keys.begin(), keys.end());

    Vector<bool> assigned(numKeys, false);
    for (uint32_t i = 0; i < numKeys; ++i)
    {
      uint32_t idx = testStruct.Compute(keys[i]);
      eXl_ASSERT(idx >= 0);
      eXl_ASSERT(idx < numKeys);
      eXl_ASSERT(!assigned[idx]);
      assigned[idx] = true;
    }

    printf("Time MPHF Build : %f\n", timer.GetTime());

    for (uint32_t i = 0; i < numQueries; ++i)
    {
      String const& s = keys[rand->Generate() % keys.size()];
      testStruct.Compute(s);
    }

    printf("Time MPHFStruct : %f\n", timer.GetTime());
  }

  {
    Clock timer;
    timer.GetTime();
    UnorderedMap<Name, uint32_t> classicNameMap;
    Vector<Name> names;
    for (auto const& s : keys)
    {
      names.push_back(Name(s));
    }

    NameMPHF namesHash;
    namesHash.Build(names.begin(), names.end());

    for (uint32_t i = 0; i < numQueries; ++i)
    {
      classicNameMap.find(names[rand->Generate() % names.size()]);
    }

    printf("Time classic names : %f\n", timer.GetTime());

    Vector<bool> assigned(numKeys, false);
    for (uint32_t i = 0; i < numKeys; ++i)
    {
      uint32_t idx = namesHash.Compute(names[i]);
      eXl_ASSERT(idx >= 0);
      eXl_ASSERT(idx < numKeys);
      eXl_ASSERT(!assigned[idx]);
      assigned[idx] = true;
    }

    printf("Time MPHF names Build : %f\n", timer.GetTime());

    for (uint32_t i = 0; i < numQueries; ++i)
    {
      Name const& s = names[rand->Generate() % names.size()];
      namesHash.Compute(s);
    }

    printf("Time MPHFStruct names : %f\n", timer.GetTime());
  }
}