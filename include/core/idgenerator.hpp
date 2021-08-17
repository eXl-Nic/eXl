/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <boost/icl/interval_set.hpp>

namespace eXl
{
  /**
     The class IdGenerator is used to manage a collection of Id, like an allocator.
     Cannot handle contiguous ids.
  **********************************************************************/
  class EXL_CORE_API IdGenerator
  {
  public:
    /**
       Generator CTor.
       @param Size maximum of id that the generator can allocate (0 <==> UINT_MAX).
    **********************************************************************/
    IdGenerator(unsigned int Size = 0);
    /**
       Get the next id to be used.
       @return an unsigned int that can be used
    **********************************************************************/
    unsigned int Get();

    /**
       Peek the next Id
       Cannot be used.
    **********************************************************************/
    unsigned int Peek() const;

    /**
       Reserve a precise ID in the generator.
    **********************************************************************/
    void Reserve(unsigned int iId);
    /**
       Return an id in the heap.
       @param id the id to return in the heap.
    **********************************************************************/
    void Return(unsigned int iId);

    inline unsigned int GetCount()const{return count;}

    inline unsigned int GetSize()const{return m_Size;}

    //If no alloc, returns size;
    unsigned int GetMaxId();

    void Grow(uint32_t iNum);

  private:
    boost::icl::interval_set<unsigned int> m_Set;
    uint32_t m_Size;
    uint32_t count;
  };
}

