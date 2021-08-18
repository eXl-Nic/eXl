/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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