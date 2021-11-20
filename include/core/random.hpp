/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "coredef.hpp"

namespace eXl
{
  class EXL_CORE_API Random
  {
  public:

    virtual ~Random()
    {}

    // Create a deterministic RNG
    static Random* CreateDefaultRNG(unsigned int iSeed);

    // Allocate a random uint64 from a UUID source
    static uint64_t AllocateUUID();

    virtual unsigned int Generate() = 0;

    virtual String Save() = 0;

    virtual void Restore(String const& iStr) = 0;

    inline unsigned int operator()()
    {
      return Generate();
    }
  };

  struct RandomWrapper
  {
    typedef unsigned int result_type;
    unsigned int min() { return 0; }
    unsigned int max() { return 0xFFFFFFFF; }
    RandomWrapper(Random* iRand) : m_Rand(iRand) {}

    inline unsigned int operator()(){ return m_Rand->Generate();}

    Random* m_Rand;
  };
}