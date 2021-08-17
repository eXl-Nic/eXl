#include <core/randomsetwalk.hpp>
#include <core/random.hpp>

#include <boost/container/flat_set.hpp>

namespace eXl
{
  struct PrimeNumbers
  {
    PrimeNumbers()
    {
      m_Primes.insert(2);
      m_Primes.reserve(1000);
      AddUpTo(1000);
    }

    void AddUpTo(unsigned int iMin)
    {
      unsigned int start = *m_Primes.rbegin();
      unsigned int curMaxPrime = start;
      ++start;
      while (curMaxPrime <= iMin)
      {
        unsigned int limit = sqrt(double(start));
        boost::container::flat_set<unsigned int>::iterator iter = m_Primes.begin();
        boost::container::flat_set<unsigned int>::iterator iterEnd = m_Primes.end();
        for (; iter != iterEnd; ++iter)
        {
          if (*iter > limit)
          {
            iter = iterEnd;
            break;
          }
          if (start % *iter == 0)
          {
            break;
          }
        }
        if (iter == iterEnd)
        {
          //printf("%i\n", start);
          curMaxPrime = start;
          m_Primes.insert(start);
        }
        ++start;
      }
    }

    unsigned int Get(unsigned int iValue)
    {
      if (iValue >= *m_Primes.rbegin())
      {
        AddUpTo(iValue);
        return *m_Primes.rbegin();
      }
      else
      {
        return *m_Primes.upper_bound(iValue);
      }
    }

    boost::container::flat_set<unsigned int> m_Primes;
  };

  static PrimeNumbers s_Prime;

  RandomSetWalk::RandomSetWalk(unsigned int iSetSize, Random& iRand)
  {
    m_Size = iSetSize;
    m_Prime = s_Prime.Get(iSetSize);
    Reset(iRand);
  }

  bool RandomSetWalk::Next(unsigned int& oNum)
  {
    if (m_Step < m_Size)
    {
      oNum = m_Current;
      do
      {
        m_Current += m_Skip;
        m_Current %= m_Prime;
      }
      while(m_Current >= m_Size);
      ++m_Step;
      return true;
    }
    return false;
  }

  void RandomSetWalk::Reset(Random& iRand)
  {
    do
    {
      m_Skip = ((iRand.Generate() % (m_Prime - 1)) + 1) * m_Size * m_Size + ((iRand.Generate() % (m_Prime - 1)) + 1) * m_Size + ((iRand.Generate() % (m_Prime - 1)) + 1);
    }while(m_Skip % m_Prime == 0);
    m_Current = iRand.Generate() % m_Size;
    m_Step = 0;
  }
}