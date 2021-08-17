#pragma once

#include <core/coredef.hpp>

namespace eXl
{
  class Random;

  /**
    Walks through a set of numbers, from 0 to iSetSize-1, visiting each elements only once.
  */
  class EXL_CORE_API RandomSetWalk
  {
  public:

    RandomSetWalk(unsigned int iSetSize, Random& iRand);

    bool Next(unsigned int& oNum);

    inline unsigned int GetCurrent() const {return m_Current;}

    void Reset(Random& iRand);

  protected:
    unsigned int m_Size;
    unsigned int m_Prime;
    unsigned int m_Skip;
    unsigned int m_Step;
    unsigned int m_Current;
  };
}